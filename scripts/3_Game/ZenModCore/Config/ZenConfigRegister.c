class ZenConfigRegister
{
	static const string RPC_CLIENT_RECEIVE 					= "RPC_ReceiveConfigOnClient";
	static const string RPC_SERVER_RECEIVE_SYNC_REQUEST 	= "RPC_RequestConfigIfOutdated";
	static const string RPC_CLIENT_RECEIVE_CHUNK			= "RPC_ReceiveConfigChunkOnClient";
	
	protected ref map<typename, ref ZenConfigBase> m_Configs = new map<typename, ref ZenConfigBase>;
	protected ref map<string, typename> m_TypesByName = new map<string, typename>;
	protected ref map<string, string> m_PendingPayloads = new map<string, string>;
	protected ref array<typename> m_AllTypes = new array<typename>;
	protected ref map<string, ref map<int, string>> m_PendingChunks = new map<string, ref map<int, string>>;
	protected ref map<string, int> m_PendingTotals = new map<string, int>;
	protected ref map<string, bool> m_HasReceivedServerSync = new map<string, bool>;
	protected static bool s_Registered;
	
	// 3_game cannot talk up>scope - it can't talk to 4_world or 5_mission, but 4_world/5_mission can talk down<scope
	// A ScriptInvoker is similair to a "signal" in GoDot engine and other game engines where we can "fire" a signal onto this SI
	// and any class which "subscribes" to it receives the signal. So we can subscribe to this in 5_mission, and "do something"
	// whenever config is received here in 3_game scope. Pretty cool stuff, at least for retards like me.
	static ref ScriptInvoker AfterSyncReceivedSI = new ScriptInvoker();
	
	bool HasReceivedSync(ZenConfigBase cfg)
	{
		if (g_Game && g_Game.IsDedicatedServer())
			return true;
		
		bool received = false;
		if (m_HasReceivedServerSync.Find(cfg.ClassName(), received))
			return received;
		
		return false;
	}
	
	void DestroyConfig(typename cfgType)
	{
		ZenConfigBase cfg;
		if (m_Configs.Find(cfgType, cfg))
		{
			m_Configs.Remove(cfgType);
			m_TypesByName.Remove(cfgType.ToString());
			m_AllTypes.RemoveItem(cfgType);
			delete cfg;
		}
	}
	
	void RegisterPreload();
	
	void RegisterRPC()
	{
		if (s_Registered)
		{
			//Error("Do not register RPCs more than once! Remove this second RegisterRPC() call from your code.");
			return;
		}
		
		ZMPrint("[ZenModCore] Registering ZenConfigRegister RPCs.");
		GetRPCManager().AddRPC(ZEN_RPC, RPC_CLIENT_RECEIVE, this, SingleplayerExecutionType.Both);
		GetRPCManager().AddRPC(ZEN_RPC, RPC_SERVER_RECEIVE_SYNC_REQUEST, this, SingleplayerExecutionType.Both);
		GetRPCManager().AddRPC(ZEN_RPC, RPC_CLIENT_RECEIVE_CHUNK, this, SingleplayerExecutionType.Both);
		
		s_Registered = true;
	}
	
	void PrepareConfigs()
	{
		for (int i = 0; i < m_AllTypes.Count(); i++)
		{
			RegisterConfig(m_AllTypes.Get(i));
		}
	}
	
	void RegisterType(typename type)
	{
		if (!type)
			return;
	
		string typeName = type.ToString();
	
		typename tmp;
		if (!m_TypesByName.Find(typeName, tmp))
			m_TypesByName.Insert(typeName, type);
	
		if (m_AllTypes.Find(type) == -1)
			m_AllTypes.Insert(type);
	
		// If a payload arrived BEFORE this type was registered, apply it now.
		string pending;
		if (m_PendingPayloads.Find(typeName, pending))
		{
			m_PendingPayloads.Remove(typeName);
			ApplyPayloadToConfig(typeName, pending);
		}
	}

	ZenConfigBase RegisterConfig(typename type)
	{
		if (!type)
			return null;
	
		// Always register mapping (important for RPC receiver)
		RegisterType(type);
	
		// Already created?
		ZenConfigBase existing;
		if (m_Configs.Find(type, existing) && existing)
		{
			existing.OnRegistered(); // rebind globals if needed
			return existing;
		}
	
		ZenConfigBase cfg = ZenConfigBase.Cast(type.Spawn());
		if (!cfg)
		{
			ErrorEx("[ZenConfigRegister] Failed to Spawn() config type: " + type.ToString());
			return null;
		}
	
		#ifdef SERVER
		if (cfg.IsClientOnlyConfig())
		{
			ZMPrint("[ZenConfigRegister] Deleting client-side only config on server: " + cfg.ClassName());
			delete cfg;
			return null;
		}
		#else
		if (cfg.IsServerOnlyConfig())
		{
			ZMPrint("[ZenConfigRegister] Deleting server-side only config on client: " + cfg.ClassName());
			delete cfg;
			return null;
		}
		#endif
	
		cfg.SetDefaults();
		cfg.Load();
	
		m_Configs.Insert(type, cfg);
	
		// Bind globals / static instance pointers here
		cfg.OnRegistered();
	
		return cfg;
	}
	
	bool UnregisterConfig(ZenConfigBase cfg)
	{
		bool removedCfg = false;
		
		if (cfg && m_Configs.Contains(cfg.Type()))
		{
			string cfgName = cfg.ClassName();
			m_Configs.Remove(cfg.Type());
			m_TypesByName.Remove(cfg.Type().ToString());
			m_AllTypes.RemoveItem(cfg.Type());
			removedCfg = true;
		}
		
		if (removedCfg)
			ZMPrint("[ZenConfigRegister] Unregistered " + cfgName);
		
		return removedCfg;
	}
	
	void RequestConfigIfOutdated(typename t)
	{
		if (!g_Game || !g_Game.IsClient())
			return;
	
		ZenConfigBase cfg;
		if (!m_Configs.Find(t, cfg) || !cfg)
			return;
	
		// Send (typeName, clientSyncVersion) to server
		GetRPCManager().SendRPC(ZEN_RPC, RPC_SERVER_RECEIVE_SYNC_REQUEST, new Param2<string, string>(t.ToString(), cfg.GetSyncVersion()), true, null);
	}
	
	void RPC_RequestConfigIfOutdated(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
			return;
	
		Param2<string, string> msg;
		if (!ctx.Read(msg))
			return;
	
		string typeName = msg.param1;
		string clientVer = msg.param2;
	
		typename t;
		if (!m_TypesByName.Find(typeName, t))
		{
			ErrorEx("[ZenConfig] Unknown config type: " + typeName);
			return;
		}
	
		ZenConfigBase cfg;
		if (!m_Configs.Find(t, cfg) || !cfg)
			return;
	
		string serverVer = cfg.GetSyncVersion();
	
		if (clientVer != serverVer)
		{
			cfg.SyncToClient(sender);
			Print("[ZenConfig] " + typeName + " mismatch client=" + clientVer + " server=" + serverVer + " -> sent update to " + sender.GetId());
		}
	}

	void SaveAllConfigs()
	{
		foreach (typename type, ZenConfigBase cfg : m_Configs)
		{
			if (cfg && cfg.ShouldSaveOnShutdown())
			{
				cfg.Save();
			}
		}
	}
	
	ZenConfigBase Get(string type)
	{
		return Get(m_TypesByName.Get(type));
	}

	ZenConfigBase Get(typename type)
	{
		ZenConfigBase cfg;
		m_Configs.Find(type, cfg);
		return cfg;
	}
	
	map<typename, ref ZenConfigBase> GetConfigs()
	{
		return m_Configs;
	}
	
	void SyncToClient(PlayerIdentity identity)
	{
		int cfgCount = 0;
		
		foreach (typename type, ZenConfigBase cfg : m_Configs)
		{
			if (cfg && cfg.ShouldSyncToClient())
			{
				cfgCount++;
				cfg.SyncToClient(identity);
			}
		}
		
		Print("[ZenConfigRegister] Sent " + cfgCount + " config files to id=" + identity.GetId());
	}
	
	private void ApplyPayloadToConfig(string typeName, string payload)
	{
		typename t;
		if (!m_TypesByName.Find(typeName, t))
		{
			// We don't know this typename yet (getter not called / type not registered).
			// Cache it and apply later when RegisterType() is called.
			m_PendingPayloads.Set(typeName, payload);
			Error("[ZenConfig] Type not registered yet, caching payload: " + typeName);
			return;
		}
	
		ZenConfigBase cfg;
		if (!m_Configs.Find(t, cfg) || !cfg)
		{
			cfg = ZenConfigBase.Cast(t.Spawn());
			if (!cfg)
			{
				Error("[ZenConfig] Failed to create config: " + t.ToString());
				return;
			}
	
			#ifdef SERVER
			if (cfg.IsClientOnlyConfig())
			{
				delete cfg;
				return;
			}
			#else
			if (cfg.IsServerOnlyConfig())
			{
				delete cfg;
				return;
			}
			#endif
	
			// IMPORTANT: init refs before LoadData/ApplySyncPayload touches them
			cfg.SetDefaults();
	
			m_Configs.Set(t, cfg);
	
			// Bind globals / static instance pointers
			cfg.OnRegistered();
		}
	
		string err;
		if (!cfg.ApplySyncPayload(payload, err))
		{
			Error("[ZenConfig] ApplySyncPayload failed for " + typeName + ": " + err);
			return;
		}
	
		string className = cfg.ClassName();
		
		ZMPrint("[" + className + "] Received config from server.");
		
		m_HasReceivedServerSync.Set(className, true);
		cfg.AfterConfigReceived();
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DeferredInvokeAfterSync, 0, false, cfg);
	}
	
	void DeferredInvokeAfterSync(ZenConfigBase cfg)
	{
	    AfterSyncReceivedSI.Invoke(cfg);
	}
	
	// CLIENT receiver (single handler for all configs)
	void RPC_ReceiveConfigOnClient(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;
	
		Param2<string, string> msg;
		if (!ctx.Read(msg))
		{
			Error("[ZenConfig] RPC_ReceiveConfigOnClient: ctx.Read failed");
			return;
		}
	
		ApplyPayloadToConfig(msg.param1, msg.param2);
	}
	
	void RPC_ReceiveConfigChunkOnClient(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;
	
		Param4<string, int, int, string> msg;
		if (!ctx.Read(msg))
		{
			Error("[ZenConfig] RPC_ReceiveConfigChunkOnClient: ctx.Read failed");
			return;
		}
	
		string typeName = msg.param1;
		int idx = msg.param2;
		int total = msg.param3;
		string chunk = msg.param4;
	
		map<int, string> chunks;
		if (!m_PendingChunks.Find(typeName, chunks) || !chunks)
		{
			chunks = new map<int, string>;
			m_PendingChunks.Set(typeName, chunks);
		}
	
		int existingTotal;
		if (m_PendingTotals.Find(typeName, existingTotal))
		{
			// If a new transfer starts with a different total, reset.
			if (existingTotal != total)
			{
				chunks.Clear();
				m_PendingTotals.Set(typeName, total);
			}
		}
		else
		{
			m_PendingTotals.Set(typeName, total);
		}
	
		chunks.Set(idx, chunk);
	
		// Not complete yet
		if (chunks.Count() < total)
			return;
	
		// Reassemble in order
		string payload = "";
		for (int i = 0; i < total; i++)
		{
			string part;
			if (!chunks.Find(i, part))
				return; // still missing something
	
			payload += part;
		}
	
		// Cleanup buffers
		m_PendingChunks.Remove(typeName);
		m_PendingTotals.Remove(typeName);
	
		ApplyPayloadToConfig(typeName, payload);
	}
}

ref ZenConfigRegister g_ZenConfigRegister;

static ZenConfigRegister GetZenConfigRegister()
{
	if (!g_ZenConfigRegister)
	{
		g_ZenConfigRegister = new ZenConfigRegister();
	}
	
	return g_ZenConfigRegister;
}