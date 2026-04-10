/******************************************************************************************************************
This is a little more convoluted than I'd like it to be, but it's the best way I could think of to make
registering config for all my mods easy and modular for all the various config styles I use across all my mods.

The workflow is: inherit from this class (ZenConfigBase) and then override the relevant settings.

To define the "getter" for the config, follow the pattern below (replace "YourConfigType").

You can optionally include the "modded class ZenConfigRegister" part to make my core mod automatically pre-load 
your config when the game or server starts up so you don't have to call GetYourConfigType() on game init.

To apply a customized data sync payload, check the BuildSyncPayload and ApplySyncPayload examples in my own mods.
-------------------------------------------------------------------------------------------------------------------
*******************************************************************************************************************

ref YourConfigType g_YourConfigType;

static YourConfigType GetYourConfigType()
{
	if (!g_YourConfigType) GetZenConfigRegister().RegisterConfig(YourConfigType);
	return g_YourConfigType;
}

modded class ZenConfigRegister
{
	override void RegisterPreload()
	{
		super.RegisterPreload(); 
		RegisterType(YourConfigType);
	}
}

class YourConfigType : ZenConfigBase
{
	// -------------------------
	// CONFIG SETTINGS
	// -------------------------
	override void OnRegistered()
	{
		g_YourConfigType = this;
	}

	override string 	GetRootFolder()       		{ return super.GetRootFolder() }
	override string 	GetFolderName()       		{ return super.GetFolderName(); }
	override string 	GetFileName()         		{ return super.GetFileName(); }
	override string    	GetCurrentVersion()   		{ return "1.29.1"; }
	override bool		ShouldLoadOnServer() 		{ return false; }
	override bool		ShouldLoadOnClient() 		{ return false; }
	override bool		ShouldSyncToClient()		{ return false; }
	override bool 		ShouldSaveOnShutdown() 		{ return false; }
	override bool		IsClientOnlyConfig()		{ return false; }
	override bool 		IsServerOnlyConfig()		{ return false; }
	
	override bool ReadJson(string path, out string err)
	{
		return JsonFileLoader<YourConfigType>.LoadFile(path, this, err);
	}

	override bool WriteJson(string path, out string err)
	{
		return JsonFileLoader<YourConfigType>.SaveFile(path, this, err);
	}
	
	override protected bool BuildSyncPayload(out string payload, out string err)
	{
		return JsonFileLoader<YourConfigType>.MakeData(this, payload, err, false);
	}

	override protected bool ApplySyncPayload(string payload, out string err)
	{
		return JsonFileLoader<YourConfigType>.LoadData(payload, this, err);
	}
}

*/

class ZenConfigBase
{
	string ConfigVersion;

	[NonSerialized()]
	static const int RPC_MAX_CHUNK_CHARS = 1000;

	// -------------------------
	// CONFIG SETTINGS
	// -------------------------
	string GetRootFolder()       		
	{ 
		#ifdef SERVER
		return ZenConstants.GetProfilesFolder(); 
		#else 
		return ZenConstants.GetClientProfilesFolder();
		#endif
	}
	
	string 			GetFolderName()       		{ return ""; } // "" = no subfolder
	string 			GetFileName()         		{ return ClassName() + ".json"; }
	string    		GetCurrentVersion()   		{ return "1.29.1"; }
	string 			GetSyncVersion() 			{ return GetCurrentVersion(); }
	bool 			ShouldLoadConfig()			{ return true; }
	bool			ShouldLoadOnServer() 		{ return false; }
	bool			ShouldLoadOnClient() 		{ return false; }
	bool			ShouldSyncToClient()		{ return false; }
	bool 			ShouldSaveOnShutdown() 		{ return false; }
	bool 			ShouldDebugPrint()			{ return true; }
	bool			HasReceivedSync()			{ return GetZenConfigRegister().HasReceivedSync(this); }
	
	// Due to the nature of config registration, all registered configs will load on both client & server
	// If a config is intended to be client or server-side only, override the relevant bool to prevent it being saved in memory.
	bool			IsClientOnlyConfig()		{ return false; }
	bool 			IsServerOnlyConfig()		{ return false; }

	void 			SetDefaults() {}
	void 			Migrate(string fromVersion, string toVersion) {}

	void ZenConfigBase()
	{
		if (ShouldDebugPrint())
			ZMPrint("[ZenConfig] Created: " + ClassName());
	}
	
	void ~ZenConfigBase()
	{
		if (ShouldDebugPrint())
			ZMPrint("[ZenConfig] Destroyed: " + ClassName());
		
		if (g_ZenConfigRegister)
		{
			g_ZenConfigRegister.UnregisterConfig(this);
		}
	}
	
	void OnRegistered() { Error("Not implemented! Set the global var ref here! Eg. g_MyConfigClass = this;"); }
	
	// -------------------------
	// PATH
	// -------------------------
	string GetFolderPath()
	{
		string root = GetRootFolder();
		root.Replace("/", "\\");

		// ensure trailing slash
		if (root.Length() > 0 && root.Get(root.Length() - 1) != "\\")
		{
			root += "\\";
		}

		string p = root;

		string folder = GetFolderName();
		if (folder != "")
		{
			p += folder + "\\";
		}

		return p;
	}

	string GetFilePath()
	{
		return GetFolderPath() + GetFileName();
	}

	void BackupFile(string suffix)
	{
		string src = GetFilePath();
		
		if (FileExist(src))
		{
			CopyFile(src, src + suffix);
		}
	}

	void EnsureFolderExists()
	{
		ZenGameFunctions.EnsureDirectoriesExist(GetFolderPath());
	}

	// -------------------------
	// TYPE-SPECIFIC JSON IO HOOKS
	// -------------------------
	// Child MUST override these two so serialization happens with the concrete type.
	protected bool ReadJson(string path, out string err)  { err = "ReadJson not implemented";  return false; }
	protected bool WriteJson(string path, out string err) { err = "WriteJson not implemented"; return false; }

	// -------------------------
	// LOAD / SAVE
	// -------------------------
	void Load()
	{
		if (!ShouldLoadConfig())
			return;
		
		#ifdef SERVER
		if (!ShouldLoadOnServer())
			return;
		#else
		if (!ShouldLoadOnClient())
			return;
		#endif

		string path = GetFilePath();

		if (FileExist(path))
		{
			if (ShouldDebugPrint())
				ZMPrint("[ZenConfig] Loading file: " + GetFilePath());
			
			string err;
			if (!ReadJson(path, err))
			{
				Error(err);
				BackupFile(".bad");
				ConfigVersion = GetCurrentVersion();
				Save();
				return;
			}

			string cur = GetCurrentVersion();
			if (ConfigVersion != cur)
			{
				string oldVersion = ConfigVersion;
				if (oldVersion == "")
					oldVersion = "OLD";
				
				BackupFile(string.Format(".v%1", oldVersion));
				Migrate(ConfigVersion, cur);
				ConfigVersion = cur;
				Save();
			}
			else 
			{
				ZMPrint("[ZenConfig] File loaded: " + GetFileName() + " version=" + ConfigVersion);
			}
		}
		else
		{
			if (ShouldDebugPrint())
				ZMPrint("[ZenConfig] Creating file: " + GetFileName());
			
			ConfigVersion = GetCurrentVersion();
			Save(false);
		}

		DoDebugPrint("LOAD");
		AfterLoad();
	}

	void Save(bool printLog = true)
	{
		#ifdef SERVER
		if (!ShouldLoadOnServer())
			return;
		#else
		if (!ShouldLoadOnClient())
			return;
		#endif

		if (printLog)
		{
			if (ShouldDebugPrint())
				ZMPrint("[" + ClassName() + "] Saving to file: " + GetFilePath());
		}
		
		EnsureFolderExists();

		string err;
		if (!WriteJson(GetFilePath(), err))
		{
			ErrorEx(string.Format("[%1] Failed to save config file: %2", GetFileName(), err));
		}
		
		DoDebugPrint("SAVE");
		AfterSave();
	}
	
	// Config decides WHAT is synced via build/apply funcs - override these to make specific config changes
	bool BuildSyncPayload(out string payload, out string err) 	{ payload = ""; err = "BuildSyncPayload not implemented"; return false; }
	bool ApplySyncPayload(string payload, out string err)      	{ err = "ApplySyncPayload not implemented"; return false; }

	void SyncToClient(PlayerIdentity identity = null)
	{
		string payload, err;
		if (!BuildSyncPayload(payload, err))
		{
			ErrorEx("[ZenConfig] BuildSyncPayload failed for " + Type().ToString() + ": " + err);
			return;
		}

		string typeName = Type().ToString();
		int totalLen = payload.Length();

		// Small enough -> single RPC
		if (totalLen <= RPC_MAX_CHUNK_CHARS)
		{
			GetRPCManager().SendRPC(ZEN_RPC, ZenConfigRegister.RPC_CLIENT_RECEIVE, new Param2<string, string>(typeName, payload), true, identity);
			return;
		}

		// Chunked send
		int totalChunks = totalLen / RPC_MAX_CHUNK_CHARS;
		if ((totalLen % RPC_MAX_CHUNK_CHARS) != 0)
			totalChunks = totalChunks + 1;

		if (ShouldDebugPrint())
			ZMPrint("[ZenConfig] Chunking " + typeName + " payloadLen=" + totalLen + " chunks=" + totalChunks);

		for (int i = 0; i < totalChunks; i++)
		{
			int start = i * RPC_MAX_CHUNK_CHARS;
			int remain = totalLen - start;

			int take = RPC_MAX_CHUNK_CHARS;
			if (remain < take)
				take = remain;

			string chunk = payload.Substring(start, take);

			GetRPCManager().SendRPC(ZEN_RPC, ZenConfigRegister.RPC_CLIENT_RECEIVE_CHUNK, new Param4<string, int, int, string>(typeName, i, totalChunks, chunk), true, identity);
		}
	}

	void AfterLoad();
	void AfterSave();
	
	void AfterConfigReceived()
	{
		if (ShouldLoadOnClient())
		{
			Save();
		}
		
		AfterLoad();
		DoDebugPrint("SYNC");
	}
	
	void DoDebugPrint(string trigger = "");
}