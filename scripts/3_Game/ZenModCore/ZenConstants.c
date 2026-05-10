class ZenConstants
{
	static int SERVER_INSTANCE_ID = -1;
	
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
		if (!g_Game)
		{
			return "ZENARCHIST_ERROR";
		}
		
		string folderPath = "$mission:storage_%1\\" + modName + "\\";
		
		if (SERVER_INSTANCE_ID == -1)
			SERVER_INSTANCE_ID = g_Game.ServerConfigGetInt("instanceId");
		
		return string.Format(folderPath, SERVER_INSTANCE_ID) + extraFolders;
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