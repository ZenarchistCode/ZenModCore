// This class code only executes on the client.
modded class MissionGameplay
{
	private bool m_ZenMissionGameplayInitialized;
	private float m_ZenVoipTimeNoAmplitude;
	
	override void OnInit()
	{
		super.OnInit();
		
		if (m_ZenMissionGameplayInitialized)
		{
			return;
		}
		
		m_ZenMissionGameplayInitialized = true;
		m_ZenVoipTimeNoAmplitude = 0;
		
		OnZenModInit();
	}

	override void TickScheduler(float timeslice)
	{
		super.TickScheduler(timeslice);
		
		ZenTickScheduler(timeslice);
	}
	
	void ZenTickScheduler(float timeslice)
	{
		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		bool isVoipActive = g_Game.GetMission().IsVoNActive();
		bool isVoipPush = VONManager.GetInstance().IsVonToggled() == false;
		
		if (isVoipActive && isVoipPush)
		{
			float voipAmplitude = player.IsPlayerSpeaking();
			
			if (voipAmplitude == 0)
			{
				bool enabled = ZenOptions.GetBool("ZEN.MicCheck", true);
				if (enabled)
				{
					// Mic is being pressed but amplitude is zero - trigger mic notification
					m_ZenVoipTimeNoAmplitude += timeslice;
					
					if (m_ZenVoipTimeNoAmplitude > 5)
					{
						// Warn player!
						m_ZenVoipTimeNoAmplitude = 0;
						NotificationSystem.AddNotificationExtended(5, "#STR_ZenMicCheck0", "#STR_ZenMicCheck2", "set:dayz_gui image:mic");
					}
				}
			}
			else 
			{
				m_ZenVoipTimeNoAmplitude = 0;
			}
		}
	}
	
	void OnZenModInit()
	{
		RegisterZenClientOptions();
		CheckZenCleanFiles();
	}
	
	// All config which is required ONLY on the client should be init here.
	override void LoadZenConfig()
	{
		super.LoadZenConfig();
	}
	
	bool IsZenMissionGameplayInitialized()
	{
		return m_ZenMissionGameplayInitialized;
	}
	
	// Example options registration and debug code. AWLAYS CALL SUPER IF YOU OVERRIDE!
	void RegisterZenClientOptions()
	{
		ZenOptions.RegisterCategory("ZEN", "#STR_ZenarchistCoreModSettings", "#STR_ZenarchistCoreModSettings1");
		ZenOptions.AddSetting("ZEN", "LogLifetime", "#STR_ZenarchistCoreModSettings2", { "#menu_disabled", "#STR_ZenarchistCoreModSettings4", "#STR_ZenarchistCoreModSettings5", "#STR_ZenarchistCoreModSettings6", "#STR_ZenarchistCoreModSettings7" }, 0, "#STR_ZenarchistCoreModSettings8");
		ZenOptions.AddBoolSetting("ZEN", "MicCheck", "#STR_ZenMicCheck0", false, "#STR_ZenMicCheck1");
		
		/*
		ZenOptions.RegisterCategory("ZEN", "Zenarchist's Mods", "Client-side settings for Zenarchist's mods.");
		ZenOptions.AddBoolSetting("ZEN", "EnableCoolStuff",	"Enable Cool Stuff", true, "Turns the cool stuff on/off.");
		ZenOptions.AddSetting("ZEN", "Intensity", "Intensity", { "Off", "Low", "Medium", "High" }, 2, "How strong the effect should be.");
		
		bool enabled = ZenOptions.GetBool("ZEN.EnableCoolStuff", true);
		int intensityIdx = ZenOptions.GetIndex("ZEN.Intensity", 2);
		string intensityText = ZenOptions.GetValue("ZEN.Intensity", "Medium");
		
		ZMPrint("[OptionsTest1] Enabled=" + enabled + " intensityIdx=" + intensityIdx + " intensityText=" + intensityText);
		*/
		
		AfterRegisterZenClientOptions();
	}
	
	void AfterRegisterZenClientOptions()
	{
	}
	
	void CheckZenCleanFiles()
	{
		int deleteDaysIndex = ZenOptions.GetIndex("ZEN.LogLifetime", 0);
		
		if (deleteDaysIndex == 0)
			return;
		
		array<int> daysList = { -1, 1, 7, 14, 30 };
		int dayCount = daysList.Get(deleteDaysIndex);
		
		if (dayCount == -1)
			return;
		
		int deleted = ZenFileCleaner.CleanupVanillaProfileFiles(dayCount);
		
		if (deleted > 0)
		{
			ZMPrint("[ZenFileCleaner] Deleted old vanilla profile files older than " + dayCount + " days: " + deleted);
		} else 
		{
			ZMPrint("[ZenFileCleaner] Any log files older than " + dayCount + " days will be deleted.");
		}
	}
	
	//! RPC STUFF
	
	override void RegisterZenRPC()
	{
		super.RegisterZenRPC();
		
		GetRPCManager().AddRPC(ZEN_RPC, "RPC_ReceiveZenAdminCommandFailed", this, SingeplayerExecutionType.Client);
	}
	
	void RPC_ReceiveZenAdminCommandFailed(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Client && GetGame().IsClient())
        {
            Param1<string> data;
            if (!ctx.Read(data))
            {
                Error("Error receiving data - RPC_ReceiveZenAdminCommandFailed");
                return;
            }

            if (data.param1 && g_Game)
            {
                g_Game.ChatPlayer(data.param1);
            }
        }
    }
}