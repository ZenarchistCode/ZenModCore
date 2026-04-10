class ZenObjectHookDbBase: ZenConfigBase
{
	// -------------------------
	// CONFIG SETTINGS
	// -------------------------
	override string 	GetRootFolder() 			{ return ZenConstants.GetDbFolder("hookspawnerdb"); }
	override string    	GetCurrentVersion()   		{ return "1.29.1"; }
	override bool		ShouldLoadOnServer() 		{ return true; }
	override bool 		IsServerOnlyConfig()		{ return true; }
	
	override bool ReadJson(string path, out string err)
	{
		return JsonFileLoader<ZenObjectHookDbBase>.LoadFile(path, this, err);
	}

	override bool WriteJson(string path, out string err)
	{
		return JsonFileLoader<ZenObjectHookDbBase>.SaveFile(path, this, err);
	}

	// -------------------------
	// CONFIG VARS
	// -------------------------
	
	// Vector pos, object name
	ref map<string, string> DumpedPositions;
	
	override void SetDefaults()
	{
		DumpedPositions = new map<string, string>;
	}
	
	void Clear()
	{
		if (DumpedPositions)
			DumpedPositions.Clear();
	}
}