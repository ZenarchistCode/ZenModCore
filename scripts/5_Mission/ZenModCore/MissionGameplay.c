// This class code only executes on the client.
modded class MissionGameplay
{
	private bool m_ZenMissionGameplayInitialized;
	
	override void OnInit()
	{
		super.OnInit();
		
		if (m_ZenMissionGameplayInitialized)
		{
			return;
		}
		
		m_ZenMissionGameplayInitialized = true;
		
		OnZenModInit();
	}

	override void TickScheduler(float timeslice)
	{
		super.TickScheduler(timeslice);
		
		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		ZenTickScheduler(timeslice, player);
	}
	
	void ZenTickScheduler(float timeslice, PlayerBase clientPlayer)
	{
	}
	
	void OnZenModInit()
	{
		RegisterZenClientOptions();
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
		// Always call super if overridden - use this to apply settings etc on launch.
	}
	
	//! RPC STUFF
	
	override void RegisterZenRPC()
	{
		super.RegisterZenRPC();
		
		GetRPCManager().AddRPC(ZEN_RPC, "RPC_ReceiveZenAdminCommandFailed", this, SingeplayerExecutionType.Client);
	}
	
	void RPC_ReceiveZenAdminCommandFailed(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Client && g_Game.IsClient())
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