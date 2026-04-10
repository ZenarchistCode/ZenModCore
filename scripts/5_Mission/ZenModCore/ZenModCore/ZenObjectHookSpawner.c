ref ZenObjectHookSpawner g_ZenObjectHookSpawner;

static ZenObjectHookSpawner GetZenObjectHookSpawner()
{
	if (!g_ZenObjectHookSpawner) g_ZenObjectHookSpawner = new ZenObjectHookSpawner();
	return g_ZenObjectHookSpawner;
}

class ZenObjectHookSpawner
{
	protected ref Timer m_Timer;
	protected ref array<ref ZenObjectHookConfigBase> m_HookConfigs;

	void Init()
	{
		ZMPrint("[ZenObjectHookSpawner] Init.");
		ZenObjectHookConfigBase.AfterHookLoaded.Insert(OnHookConfigLoaded);
	}
	
	void SpawnOrDump()
	{
		if (!m_Timer)
			m_Timer = new Timer();
		
		m_Timer.Run(30, this, "DumpObjects");
	}
	
	void OnHookConfigLoaded(ZenObjectHookConfigBase cfg)
	{
		if (!m_HookConfigs)
			m_HookConfigs = new array<ref ZenObjectHookConfigBase>;
		
		if (m_HookConfigs.Find(cfg) == -1)
		{
			ZMPrint("[ZenObjectHookSpawner] Added hook config to scan list: " + cfg.ClassName());
			m_HookConfigs.Insert(cfg);
		}
	}
	
	void GetObjectsToFind(out array<string> objectsToFind)
	{
		if (!objectsToFind)
			objectsToFind = new array<string>;
		
		foreach (ZenObjectHookConfigBase cfg : m_HookConfigs)
		{
			foreach (ZenObjectHookDef def : cfg.SpawnHooks)
			{
				if (!cfg.DumpAllObjects || objectsToFind.Find(def.TargetObjectType) != -1)
					continue;
				
				objectsToFind.Insert(def.TargetObjectType);
			}
		}
	}
	
	void DumpObjects()
	{
		if (!m_HookConfigs)
		{
			ZMPrint("[ZenObjectHookSpawner] No hook configs to dump positions of.");
			return;
		}
		
		for (int i = m_HookConfigs.Count() - 1; i >= 0; i--)
		{
			if (!m_HookConfigs.Get(i).ShouldSpawn())
			{
				ZMPrint("[ZenObjectHookSpawner] " + m_HookConfigs.Get(i).ClassName() + ": Don't spawn.");
				m_HookConfigs.Remove(i);
			}
		}
		
		if (m_HookConfigs.Count() == 0)
		{
			ZMPrint("[ZenObjectHookSpawner] No hook configs set to spawn.");
			return;
		}
	
		// Load the list of objects we need to find.
		array<string> objectsToFind;
		GetObjectsToFind(objectsToFind);
	
		// Nothing needs dumping -> just spawn from existing DBs.
		if (!objectsToFind || objectsToFind.Count() == 0)
		{
			ZMPrint("[ZenObjectHookSpawner] No objects require a map-wide dump: start spawning of cached object DB locations.");
			SpawnObjects();
			return;
		}
	
		int startTime = g_Game.GetTime();
		int duration;
	
		ZMPrint("[ZenObjectHookSpawner] Start object dump - searching for:");
		foreach (string objToFindDump : objectsToFind)
		{
			objToFindDump.ToLower();
			ZMPrint("[ZenObjectHookSpawner]   '" + objToFindDump + "'");
		}
	
		// World-wide object scan.
		vector centerPos = ZenGameFunctions.GetWorldCenterPosition();
	
		array<Object> objectsOnMap = new array<Object>;
		g_Game.GetObjectsAtPosition3D(centerPos, 30000, objectsOnMap, null);
	
		int objCount = 0;
	
		// posStr -> TRUE world object classname found at that pos
		map<string, string> positions = new map<string, string>;
		map<string, vector> orientationsByPos = new map<string, vector>;
	
		foreach (Object obj : objectsOnMap)
		{
			if (!obj)
				continue;
	
			string className = ZenGameFunctions.GetTrueObjectName(obj);
			if (className == "")
				continue;
			
			className.ToLower();
	
			foreach (string lookForObj : objectsToFind)
			{
				lookForObj.ToLower();
				
				if (className.Contains(lookForObj))
				{
					string posKey = obj.GetPosition().ToString(false);
					positions.Set(posKey, className);
					orientationsByPos.Set(posKey, obj.GetOrientation());
					objCount++;
					break;
				}
			}
		}
	
		// If scan found nothing, don't wipe DB/config; just spawn from whatever DB already exists.
		if (!positions || positions.Count() == 0)
		{
			duration = g_Game.GetTime() - startTime;
			ZMPrint("[ZenObjectHookSpawner] Finish object dump - no objects found (duration=" + duration + "ms - total objects scanned=" + objectsOnMap.Count() + "). Spawning from existing DB.");
			m_Timer.Stop();
			m_Timer.Run(1, this, "SpawnObjects");
			return;
		}
	
		// 1) Clear DBs ONCE per config that requested a dump
		foreach (ZenObjectHookConfigBase cfgClear : m_HookConfigs)
		{
			if (!cfgClear || !cfgClear.DumpAllObjects)
				continue;
	
			ZenObjectHookDbBase dbClear = cfgClear.GetDB();
			if (!dbClear)
				continue;
	
			// Safety: ensure map exists before writing
			if (!dbClear.DumpedPositions)
				dbClear.DumpedPositions = new map<string, string>;
	
			dbClear.Clear();
		}
	
		// 2) Fill DBs (ONLY with positions relevant to each config)
		foreach (string vectorPos, string objName : positions)
		{
			objName.ToLower();
			
			foreach (ZenObjectHookConfigBase cfg : m_HookConfigs)
			{
				if (!cfg || !cfg.DumpAllObjects)
					continue;
	
				ZenObjectHookDbBase db = cfg.GetDB();
				if (!db)
					continue;
	
				if (!db.DumpedPositions)
					db.DumpedPositions = new map<string, string>;
	
				foreach (ZenObjectHookDef def : cfg.SpawnHooks)
				{
					string targetName = def.TargetObjectType;
					targetName.ToLower();
					
					// objName is the real classname; TargetObjectType is usually a substring token
					if (objName.Contains(targetName))
					{
						db.DumpedPositions.Set(vectorPos, objName);
						
						vector parentPos = vectorPos.ToVector();
						vector parentOri = orientationsByPos.Get(vectorPos); // Yaw,Pitch,Roll
						OnDump(parentPos, parentOri, cfg, def, objName);
						
						break; // only store once per cfg for this object
					}
				}
			}
		}
	
		// 3) Save ONCE per config + flip DumpAllObjects off (so it never dumps again unless admin re-enables)
		foreach (ZenObjectHookConfigBase cfgSave : m_HookConfigs)
		{
			if (!cfgSave || !cfgSave.DumpAllObjects)
				continue;
	
			ZenObjectHookDbBase dbSave = cfgSave.GetDB();
			if (dbSave)
				dbSave.Save();
	
			cfgSave.DumpAllObjects = !cfgSave.ShouldOnlyDumpOnce();
			cfgSave.SpawnObjects = !cfgSave.ShouldOnlySpawnOnce();
			cfgSave.Save();
		}
	
		duration = g_Game.GetTime() - startTime;
		ZMPrint("[ZenObjectHookSpawner] Finish object dump - total objects found=" + objCount + "/" + objectsOnMap.Count() + " scanned | Duration=" + duration + "ms.");
	
		m_Timer.Stop();
		m_Timer.Run(1, this, "SpawnObjects");
	}
	
	void SpawnObjects()
	{
		if (!m_HookConfigs || m_HookConfigs.Count() == 0)
			return;
		
		ZMPrint("[ZenObjectHookSpawner] Spawning objects.");
		int startTime = g_Game.GetTime();
		int objectCount = 0;
		
		// 1. Loop through all configs
		foreach (ZenObjectHookConfigBase cfg : m_HookConfigs)
		{
			// 2. Get all their dump positions
			foreach (string posStr, string spawnType : cfg.GetDB().DumpedPositions)
			{
				spawnType.ToLower();
				
				// 3. Get their spawn def for the spawnType
				ZenObjectHookDef def = GetHookDef(cfg, spawnType);
				if (!def)
					continue;
				
				if (Math.RandomFloat01() > def.ChanceOfSpawn)
					continue;
				
				// 4. Create the spawn type on the target hook object.
				array<Object> objectsNearPosition = new array<Object>;
				g_Game.GetObjectsAtPosition3D(posStr.ToVector(), 1, objectsNearPosition, null);
				
				foreach (Object obj : objectsNearPosition)
				{
					string className = ZenGameFunctions.GetTrueObjectName(obj);
					if (className == "")
						continue;
					
					className.ToLower();
					
					if (className.Contains(spawnType))
					{
						Object newObj = CreateObject(obj, def);
						if (!newObj)
							continue;
						
						OnSpawn(newObj, cfg, className);
						objectCount++;
					}
				}
			}
		}
		
		int duration = g_Game.GetTime() - startTime;
		ZMPrint("[ZenObjectHookSpawner] Object spawn completed - spawned " + objectCount + " objects in " + duration + " ms.");
		
		Cleanup();
	}
	
	static Object CreateObject(Object parentObj, ZenObjectHookDef def)
	{
		if (!parentObj)
			return null;
		
		Object newObj = ZenGameFunctions.SpawnObjectRelative(
			parentObj,
			def.SpawnObjectType,
			def.OffsetPosition,
			def.OffsetOrientation,
			def.SpawnObjectScale);
		
		if (!newObj)
		{
			Error("Failed to spawn object type: " + def.SpawnObjectType);
			return null;
		}
		
		return newObj;
	}
	
	ZenObjectHookDef GetHookDef(ZenObjectHookConfigBase cfg, string className)
	{
		className.ToLower();
		
		foreach (ZenObjectHookDef def : cfg.SpawnHooks)
		{
			string targetName = def.TargetObjectType;
			targetName.ToLower();
			
			if (className.Contains(targetName))
			{
				return def;
			}
		}
		
		return null;
	}
	
	float GetSpawnChance(ZenObjectHookConfigBase cfg, string className)
	{
		if (!cfg.SpawnObjects)
			return 0;
		
		foreach (ZenObjectHookDef def : cfg.SpawnHooks)
		{
			if (def.TargetObjectType == className)
			{
				return def.ChanceOfSpawn;
			}
		}
		
		return 1.0;
	}
	
	private static bool s_MapGroupPosStarted;

	void OnDump(vector parentPos, vector parentOri, ZenObjectHookConfigBase cfg, ZenObjectHookDef def, string parentClassName)
	{
		if (!cfg.SpawnMapGroupPosXML)
			return;

		// Clear the output file ONCE per dump run (so you don't append duplicates on restart)
		if (!s_MapGroupPosStarted)
		{
			s_MapGroupPosStarted = true;
			ZenMapGroupPosWriter.Clear();
		}

		// IMPORTANT: export the *spawned* object's transform, not the parent’s:
		vector spawnPos = parentPos + def.OffsetPosition;
		vector spawnOri = parentOri + def.OffsetOrientation; // (Yaw,Pitch,Roll)

		string line = ZenMapGroupPosMath.BuildGroupLine(def.SpawnObjectType, spawnPos, spawnOri);
		ZenMapGroupPosWriter.Append(line);
	}
	
	void OnSpawn(Object object, ZenObjectHookConfigBase cfg, string className)
	{
	}
	
	void Cleanup()
	{
		ZMPrint("[ZenObjectHookSpawner] Cleaning up unneeded DB files from memory.");
		
		// Once the objects are spawned we have no further need for the DB positions to be in memory.
		foreach (ZenObjectHookConfigBase cfg : m_HookConfigs)
		{
			delete cfg.GetDB();
		}
	}
}
