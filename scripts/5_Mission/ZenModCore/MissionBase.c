// This class code executes on both client & server.
modded class MissionBase
{
	private ref ZenAdminCommandHandler m_ZenAdminCommandHandler;
	private bool m_ZenMissionBaseInitialized;
	private bool m_ZenShutdownComplete;
	
	void MissionBase()
	{
		ZMPrint("[ZenModCore] MissionBase()");
		ZenGameFunctions.PrintMods();
	}

	void ~MissionBase()
	{
		ZMPrint("~MissionBase()");
		
		ZenConfigRegister.AfterSyncReceivedSI.Remove(OnZenCfgSynced);
		ZenLogger.FlushAll();
		
		if (g_Game)
		{
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(ZenDeferredInit);
		}
	}

	override void OnInit()
	{
		super.OnInit();
		
		if (m_ZenMissionBaseInitialized)
			return;
		
		ZMPrint("[ZenModCore] OnInit.");
		
		ZenConfigRegister.AfterSyncReceivedSI.Remove(OnZenCfgSynced);
    	ZenConfigRegister.AfterSyncReceivedSI.Insert(OnZenCfgSynced);
		
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ZenDeferredInit, 30000, false);
		OnZenInit();
	}
	
	// NOTE: This is triggered in 3_game. It passes the ZenConfigBase param which was received on the CLIENT to 3_game's script invoker.
	// The script invoker then triggers any class which "subscribes" to this trigger via ZenConfigRegister.AfterSyncReceived.Insert()
	// OnZenCfgSynced() is triggered by ZenModCore - and any mods which override this function can perform post-received cfg adjustments.
	void OnZenCfgSynced(notnull ZenConfigBase cfg)
	{
		ZMPrint("Config sync received from ScriptInvoker: " + cfg.ClassName());
	}
	
	ZenAdminCommandHandler GetZenCommandHandler()
	{
		return m_ZenAdminCommandHandler;
	}
	
	void ZenDeferredInit() {};
	
	bool IsZenMissionBaseInitialized()
	{
		return m_ZenMissionBaseInitialized;
	}
	
	void OnZenInit()
	{
		ZMPrint("MissionBase::OnInit");
		
		ZenGameFunctions.Init();

		RegisterZenRPC();
		LoadZenConfig();
		
		m_ZenAdminCommandHandler = new ZenAdminCommandHandler();
		
		m_ZenShutdownComplete = false;
		m_ZenMissionBaseInitialized = true;
	}
	
	void RegisterZenRPC()
	{
		GetZenConfigRegister().RegisterRPC();
	}
	
	// All config which is required on both server & client should be init here.
	protected void LoadZenConfig()
	{
		ZMPrint("[ZenConfig] Loading config files for map: " + g_Game.GetWorldName());
		
		GetZenConfigRegister().RegisterPreload();
		GetZenConfigRegister().PrepareConfigs();
		
		#ifndef SERVER
		ZenOptions.Get();
		#endif
		
		AfterLoadZenConfig();
	}
	
	void AfterLoadZenConfig()
	{
	}
	
	protected void OnZenMissionFinish()
	{
		if (m_ZenShutdownComplete)
			return;
		
		ZMPrint("Server shutdown detected.");
		m_ZenShutdownComplete = true;
		
		OnZenFinish();
	}
	
	void OnZenFinish()
	{
		ZMPrint("Saving all DBs.");
		
		ZenLogger.FlushAll();
		GetZenConfigRegister().SaveAllConfigs();
	}
	
	override void OnMissionStart()
	{
		super.OnMissionStart();
		
		ZMPrint("MissionBase::OnMissionStart");
	}
	
	override void OnMissionFinish()
	{
		super.OnMissionFinish();
		
		ZMPrint("MissionBase::OnMissionFinish");
	}
}