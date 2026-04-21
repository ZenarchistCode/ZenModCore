class ZenConstants
{
	static string GetProfilesFolder(string modName = "Zenarchist", string extraFolders = "")
	{
		return "$profile:" + modName + "\\" + extraFolders;
	}
	
	static string GetClientProfilesFolder(string modName = "Zenarchist", string extraFolders = "")
	{
		string serverPath;
		GetCLIParam("connect", serverPath);
		serverPath.Replace(":", "");
		serverPath.Replace(".", "");
		return "$profile:" + modName + "\\" + serverPath + "\\" + extraFolders;
	}
	
	static string GetDbFolder(string modName = "Zenarchist", string extraFolders = "")
	{
		string folderPath = "$mission:storage_%1\\" + modName + "\\";
		if (!g_Game)
			return string.Format(folderPath, "1") + extraFolders;
		
		return string.Format(folderPath, g_Game.ServerConfigGetInt("instanceId")) + extraFolders;
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