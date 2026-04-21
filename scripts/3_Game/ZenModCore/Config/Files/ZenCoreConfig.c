ref ZenCoreConfig g_ZenCoreConfig;

static ZenCoreConfig GetZenCoreConfig()
{
	if (!g_ZenCoreConfig) GetZenConfigRegister().RegisterConfig(ZenCoreConfig);
	return g_ZenCoreConfig;
}

modded class ZenConfigRegister
{
	override void RegisterPreload()
	{
		super.RegisterPreload(); 
		RegisterType(ZenCoreConfig);
	}
}

class ZenCoreConfig_SyncPayload
{
	ref ZenAdminConfig ZenCore_AdminConfig;
	ref ZenLoggerConfig ZenCore_LogConfig;
	ref ZenGeneralConfig ZenCore_GeneralConfig;
	
	void ZenCoreConfig_SyncPayload()
	{
		ZenCore_AdminConfig = new ZenAdminConfig();
		ZenCore_LogConfig = new ZenLoggerConfig();
		ZenCore_GeneralConfig = new ZenGeneralConfig();
	}
}

class ZenCoreConfig : ZenConfigBase
{
	// -------------------------
	// CONFIG SETTINGS
	// -------------------------
	override void OnRegistered()
	{
		g_ZenCoreConfig = this;
	}
	
	override string 	GetFolderName()       		{ return "Core"; }
	override string    	GetCurrentVersion()   		{ return "1.29.2"; }
	override bool 		ShouldLoadOnClient()		{ return true; }
	override bool		ShouldLoadOnServer() 		{ return true; }
	override bool		ShouldSyncToClient()		{ return true; }
	
	override bool ReadJson(string path, out string err)
	{
		return JsonFileLoader<ZenCoreConfig>.LoadFile(path, this, err);
	}

	override bool WriteJson(string path, out string err)
	{
		return JsonFileLoader<ZenCoreConfig>.SaveFile(path, this, err);
	}

	override bool BuildSyncPayload(out string payload, out string err)
	{
		ZenCoreConfig_SyncPayload snap = new ZenCoreConfig_SyncPayload();

		snap.ZenCore_AdminConfig 	= ZenCore_AdminConfig;
		snap.ZenCore_LogConfig 		= ZenCore_LogConfig;
		snap.ZenCore_GeneralConfig 	= ZenCore_GeneralConfig;

		// Serialize payload only (NOT the whole config)
		return JsonFileLoader<ZenCoreConfig_SyncPayload>.MakeData(snap, payload, err, false);
	}

	override bool ApplySyncPayload(string payload, out string err)
	{
		// Create default containers in case an error happens on read
		ZenCore_AdminConfig 		= new ZenAdminConfig();
		ZenCore_GeneralConfig 		= new ZenGeneralConfig();
		ZenCore_LogConfig 			= new ZenLoggerConfig();

		ZenCoreConfig_SyncPayload snap = new ZenCoreConfig_SyncPayload();
		if (!JsonFileLoader<ZenCoreConfig_SyncPayload>.LoadData(payload, snap, err))
		{
			return false;
		}
		
		ZenCore_AdminConfig 		= snap.ZenCore_AdminConfig;
		ZenCore_GeneralConfig 		= snap.ZenCore_GeneralConfig;
		ZenCore_LogConfig 			= snap.ZenCore_LogConfig;

		return true;
	}
	
	// -------------------------
	// CONFIG VARIABLES
	// -------------------------
	string MapIzurviveURL;
	ref ZenAdminConfig ZenCore_AdminConfig;
	ref ZenGeneralConfig ZenCore_GeneralConfig;
	ref ZenLoggerConfig ZenCore_LogConfig;
	ref ZenDiscordConfig ZenCore_DiscordConfig;
	
	// -------------------------
	// CONFIG FUNCTIONS
	// -------------------------
	void ZenCoreConfig();
	void ~ZenCoreConfig();
	
	bool IsAdmin(string uid)
	{
		return ZenCore_AdminConfig.AdminIDs.Find(uid) != -1;
	}
	
	bool IsAdmin(PlayerIdentity id)
	{
		if (!id)
			return false;
		
		if (IsAdmin(id.GetId()))
			return true;
		
		if (IsAdmin(id.GetPlainId()))
			return true;
		
		return false;
	}
	
	bool IsModerator(string uid)
	{
		return ZenCore_AdminConfig.AdminIDs.Find(uid) != -1 || ZenCore_AdminConfig.ModeratorIDs.Find(uid) != -1;
	}

	// -------------------------
	// OVERRIDE FUNCTIONS
	// -------------------------
	override void SetDefaults()
	{
		MapIzurviveURL = "";
		ZenCore_AdminConfig = new ZenAdminConfig();
		ZenCore_GeneralConfig = new ZenGeneralConfig();
		ZenCore_LogConfig = new ZenLoggerConfig();
		ZenCore_DiscordConfig = new ZenDiscordConfig();
	}
	
	override void AfterLoad()
	{
		if (ZenCore_AdminConfig)
		{
			if (ZenCore_AdminConfig.CommandPrefix.Length() > 1)
			{
				ZenCore_AdminConfig.CommandPrefix = ZenCore_AdminConfig.CommandPrefix.Substring(0, 1);
			}
		}
		
		ZenLogger.LogsEnabled = ZenCore_LogConfig.EnableLogs;
		ZenLogger.FileSuffix = ZenCore_LogConfig.FileSuffix;
		
		#ifndef SERVER
		ZenLogger.CleanupOldLogs(ZenCore_LogConfig.KeepLogsDaysCount);
		#endif
	}
	
	// Wipe admin ids from client cache file, but keep them in memory so we can still check if we're an admin on the client.
	override void AfterConfigReceived() 
	{
		array<string> AdminIDs_Temp = new array<string>;
		array<string> ModeratorIDs_Temp = new array<string>;
		
		AdminIDs_Temp.Copy(ZenCore_AdminConfig.AdminIDs);
		ModeratorIDs_Temp.Copy(ZenCore_AdminConfig.ModeratorIDs);
		
		ZenCore_AdminConfig.AdminIDs.Clear();
		ZenCore_AdminConfig.ModeratorIDs.Clear();
		
		// Save to file with no admin/mod ids in text
		Save();
		
		ZenCore_AdminConfig.AdminIDs.Copy(AdminIDs_Temp);
		ZenCore_AdminConfig.ModeratorIDs.Copy(ModeratorIDs_Temp);
	}
	
	override void Migrate(string fromVersion, string toVersion) 
	{
		if (toVersion == "1.29.2")
		{
			if (ZenCore_AdminConfig)
			{
				ZenCore_AdminConfig.EnableCommands = false;
			}
			
			if (ZenCore_GeneralConfig)
			{
				ZenCore_GeneralConfig.PreventHologramPlacementAt000 = false;
				ZenCore_GeneralConfig.DeleteObjectsAt000 = false;
			}
			
			if (ZenCore_LogConfig)
			{
				ZenCore_LogConfig.EnableLogs = false;
			}
			
			if (ZenCore_DiscordConfig)
			{
				ZenCore_DiscordConfig.EnableDiscord = false;
				if (ZenCore_DiscordConfig.KillfeedConfig)
				{
					ZenCore_DiscordConfig.KillfeedConfig.EnableKillfeed = false;
				}
			}
		}
	}
}

// -------------------------
// ADMIN CONFIG
// -------------------------
class ZenAdminConfig
{
	bool EnableCommands;
	string CommandPrefix;
	ref array<string> AdminIDs;
	ref array<string> ModeratorIDs;
	ref map<string, string> CommandURLs;

	void ZenAdminConfig()
	{
		EnableCommands = false;
		CommandPrefix = "!";
		AdminIDs = new array<string>;
		AdminIDs.Insert("4s_12UDE-PKYemc7adlZyKGVSrwzIMW0T32Q69_YOURUID");
		ModeratorIDs = new array<string>;
		ModeratorIDs.Insert("4s_12UDE-PKYemc7adlZyKGVSrwzIMW0T32Q69_YOURUID");
		CommandURLs = new map<string, string>;
		CommandURLs.Set("zenarchist", "https://zenarchist.com");
	}
}

// -------------------------
// GENERAL CONFIG
// -------------------------
class ZenGeneralConfig
{
	bool PreventHologramPlacementAt000;
	bool DeleteObjectsAt000;
	
	void ZenGeneralConfig()
	{
		PreventHologramPlacementAt000 = false;
		DeleteObjectsAt000 = false;
	}
}

// -------------------------
// LOG CONFIG
// -------------------------
class ZenLoggerConfig
{
	bool EnableLogs;
	string FileSuffix;
	int KeepLogsDaysCount;
	ref array<string> LogFolderExcludeList;
	
	void ZenLoggerConfig()
	{
		EnableLogs = false;
		FileSuffix = ".log";
		KeepLogsDaysCount = 14;
		LogFolderExcludeList = new array<string>;
		LogFolderExcludeList.Insert("DontPrintToThisFolder");
	}
}

// -------------------------
// DISCORD CONFIG
// -------------------------
class ZenDiscordConfig
{
	bool EnableDiscord;
	string ServerName;
	string PingAdminCommand;
	ref array<string> AdminWebhooks;
	ref ZenKillfeedConfig KillfeedConfig;
	ref map<string, string> PlayerWatchlist;

	void ZenDiscordConfig()
	{
		EnableDiscord = false;
		ServerName = ""; //g_Game.GetWorldName(); // g_Game is not available upon cfg load unfortunately. Set it on MissionInit.
		PingAdminCommand = "admin";
		AdminWebhooks = new array<string>;
		AdminWebhooks.Insert("Insert admin channel webhooks here");
		PlayerWatchlist = new map<string, string>;
		PlayerWatchlist.Set("4s_12UDE-PKYemc7adlZyKGVSrwzIMW0T32Q69_YOURUID", "Reason they're on this watchlist.");
		KillfeedConfig = new ZenKillfeedConfig();
	}
}

// -------------------------
// DISCORD CONFIG
// -------------------------
class ZenKillfeedConfig
{
	bool EnableKillfeed;
	ref array<string> KillfeedWebhooks;
	bool DisplayPlayerSteamID;
	bool DisplayKillLocation;
	bool DisplayKillsByAI;
	
	void ZenKillfeedConfig()
	{
		EnableKillfeed = false;
		KillfeedWebhooks = new array<string>;
		KillfeedWebhooks.Insert("Insert killfeed channel webhooks here");
		DisplayPlayerSteamID = false;
		DisplayKillLocation = false;
		DisplayKillsByAI = false;
	}
}