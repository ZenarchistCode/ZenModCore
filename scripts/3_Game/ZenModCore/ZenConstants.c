class ZenConstants
{
	static const string PROFILES_FOLDER 	= "$profile:Zenarchist\\";
	static const string DB_FOLDER 			= "$mission:storage_%1\\Zenarchist\\";
	
	static string GetProfilesFolder(string extraFolders = "")
	{
		return PROFILES_FOLDER + extraFolders;
	}
	
	static string GetClientProfilesFolder(string extraFolders = "")
	{
		string serverPath;
		GetCLIParam("connect", serverPath);
		serverPath.Replace(":", "");
		serverPath.Replace(".", "");
		return PROFILES_FOLDER + serverPath + "\\" + extraFolders;
	}
	
	static string GetDbFolder(string extraFolders = "")
	{
		if (!g_Game)
			return string.Format(DB_FOLDER, "1") + extraFolders;
		
		return string.Format(DB_FOLDER, g_Game.ServerConfigGetInt("instanceId")) + extraFolders;
	}
	
	static string GetLogFolder(string extraFolders = "")
	{
		string folder;
		
		#ifdef SERVER
		folder = GetProfilesFolder();
		#else 
		folder = GetClientProfilesFolder();
		#endif

		return folder + "Logs\\" + extraFolders;
	}
}

static const string ZEN_RPC = "ZenMod_RPC";