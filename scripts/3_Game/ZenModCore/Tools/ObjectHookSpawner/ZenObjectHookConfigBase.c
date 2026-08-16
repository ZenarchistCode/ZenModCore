class ZenObjectHookConfigBase: ZenConfigBase
{
	// -------------------------
	// CONFIG SETTINGS
	// -------------------------
	static ref ScriptInvoker AfterHookLoaded = new ScriptInvoker();
	
	override string    	GetCurrentVersion()   		{ return "1.29.1"; }
	override bool		ShouldLoadOnServer() 		{ return true; }
	override bool 		IsServerOnlyConfig()		{ return true; }
	
	override bool ReadJson(string path, out string err)
	{
		return JsonFileLoader<ZenObjectHookConfigBase>.LoadFile(path, this, err);
	}

	override bool WriteJson(string path, out string err)
	{
		return JsonFileLoader<ZenObjectHookConfigBase>.SaveFile(path, this, err);
	}
	
	// Vars 
	bool DumpAllObjects;
	bool SpawnMapGroupPosXML;
	bool SpawnObjects;
	ref array<ref ZenObjectHookDef> SpawnHooks;
	
	ZenObjectHookDbBase GetDB()
	{
		Error("Not implemented!");
		return null;
	}
	
	bool ShouldSpawn()
	{
		return SpawnObjects || SpawnMapGroupPosXML;
	}
	
	bool ShouldOnlyDumpOnce()
	{
		return true;
	}
	
	bool ShouldOnlySpawnOnce()
	{
		return false;
	}
	
	override void SetDefaults()
	{
		DumpAllObjects = true;
		SpawnMapGroupPosXML = false;
		SpawnObjects = true;
		SpawnHooks = new array<ref ZenObjectHookDef>;
	}
	
	override void AfterLoad()
	{
		super.AfterLoad();

		if (g_Game.IsClient())
			return;

		ZenObjectHookDbBase db = GetDB();

		if (!SpawnMapGroupPosXML && (!db || !db.DumpedPositions || db.DumpedPositions.Count() == 0))
		{
			ZMPrint("[" + ClassName() + "] DB entries do not exist - fresh object dump and spawn enabled.");

			DumpAllObjects = true;
			SpawnObjects = true;
		}
		else
		{
			ZMPrint("[" + ClassName() + "] DB file already exists: " + db.DumpedPositions.Count() + " entries.");
		}

		AfterHookLoaded.Invoke(this);
	}
}

class ZenObjectHookDef
{
	string TargetObjectType;
	string SpawnObjectType;
	float SpawnObjectScale;
	vector OffsetPosition;
	vector OffsetOrientation;
	float ChanceOfSpawn;
	
	void ZenObjectHookDef(string target, string spawntype, vector offsetPos = vector.Zero, vector offsetOri = vector.Zero, float scale = 1.0, float chance = 1.0)
	{
		TargetObjectType = target;
		SpawnObjectType = spawntype;
		SpawnObjectScale = scale;
		OffsetPosition = offsetPos;
		OffsetOrientation = offsetOri;
		ChanceOfSpawn = chance;
	}
}