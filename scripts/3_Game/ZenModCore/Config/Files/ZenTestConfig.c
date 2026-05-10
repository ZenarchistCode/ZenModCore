/*
This is purely for test purposes to test all the functionality of the config structure, file I/O and network syncing.

The use is as follows:
1. Inherit from ZenConfigBase
2. Specify your config setting overrides (filename, folder, version, sync, save on shutdown etc)
3. Make sure to explicitly define the class type in the JSON file save/load section (ReadJson/WriteJson)
4. Make sure to explicitly define the class type in the RPC serialization section (BuildSyncPayload/ApplySyncPayload)
	NOTE: If you have complex data types which do not sync properly

This config sync method uses data types converted into -> JSON string to apply settings. Large files will be chunked
and applied in chunks to avoid super large strings corrupting the data. This allows you to sync specific config variables
and ignore others by building a "payload" class which contains an identical variable name to what you're trying to sync -
there's an example below. It's a bit complicated to understand at first but it's actually quite a simple method which is
nice and robust and modular for all config types once you get used to the structure of things.
*/

ref ZenTestConfig g_ZenTestConfig;

static ZenTestConfig GetZenTestConfig()
{
	if (!g_ZenTestConfig) GetZenConfigRegister().RegisterConfig(ZenTestConfig);
	return g_ZenTestConfig;
}

modded class ZenConfigRegister
{
	override void RegisterPreload()
	{
		super.RegisterPreload(); 
		//RegisterType(ZenTestConfig); // This will force the game to create instance & auto-load this config type
	}
}

class ZenTestClass
{
	float SomeData = Math.RandomFloat01();
}

class ZenTestConfig_SyncPayload
{
	ref map<string, ref ZenTestClass> TestMap;

	void ZenTestConfig_SyncPayload()
	{
		TestMap = new map<string, ref ZenTestClass>;
	}
}

class ZenTestConfig : ZenConfigBase
{
	// -------------------------
	// CONFIG SETTINGS
	// -------------------------
	override void OnRegistered()
	{
		g_ZenTestConfig = this;
	}
	
	override string 	GetRootFolder()       		{ return ZenConstants.GetClientProfilesFolder(); }
	//override string 	GetFolderName()       		{ return ""; }
	//override string 	GetFileName()         		{ return "config.json"; }
	override string    	GetCurrentVersion()   		{ return "1.29.1"; }
	override bool		ShouldLoadOnServer() 		{ return true; }
	//override bool		ShouldLoadOnClient() 		{ return true; }
	override bool		ShouldSyncToClient()		{ return true; }
	override bool 		ShouldSaveOnShutdown() 		{ return true; }
	
	override bool ReadJson(string path, out string err)
	{
		return JsonFileLoader<ZenTestConfig>.LoadFile(path, this, err);
	}

	override bool WriteJson(string path, out string err)
	{
		return JsonFileLoader<ZenTestConfig>.SaveFile(path, this, err);
	}
	
	override protected bool BuildSyncPayload(out string payload, out string err)
	{
		return JsonFileLoader<ZenTestConfig>.MakeData(this, payload, err, false);
	}

	override protected bool ApplySyncPayload(string payload, out string err)
	{
		return JsonFileLoader<ZenTestConfig>.LoadData(payload, this, err);
	}
	
	// -------------------------
	// CONFIG VARIABLES
	// -------------------------
	bool EnablePumps;
	int  PumpCapacity;
	string NewVar;
	ref array<string> AllowedPumpTypes;
	ref map<string, ref ZenTestClass> TestMap;
	
	// -------------------------
	// CONFIG FUNCTIONS
	// -------------------------
	void ZenTestConfig();
	void ~ZenTestConfig();

	// -------------------------
	// OVERRIDE FUNCTIONS
	// -------------------------
	override void SetDefaults()
	{
		EnablePumps  = true;
		PumpCapacity = 100;

		if (!AllowedPumpTypes)
		{
			AllowedPumpTypes = new array<string>;
		}

		// don't Clear() unless you want to wipe user edits on every boot
		if (AllowedPumpTypes.Count() == 0)
		{
			AllowedPumpTypes.Insert("Land_WaterPump_DZ");
		}
		
		if (!TestMap)
		{
			TestMap = new map<string, ref ZenTestClass>;
		}
		
		if (TestMap.Count() == 0)
		{
			TestMap.Set("test", new ZenTestClass());
			TestMap.Set("test2", new ZenTestClass());
		}
		
		NewVar = "TEST!";
	}

	/*
	
	// -------------------------
	// FULL CONFIG CLASS SYNC (SIMPLE & EASY)
	// This approach copies the entire config class vars and replicates. You must explicitly define <ClassType> in JsonFileLoader.
	// This method works great with simple data types, but if you have complex maps/arrays/data structures
	// with custom classes in them (eg. map<MyClassKey, MyClassElement>) you might need to do the below method.
	// -------------------------
	override protected bool BuildSyncPayload(out string payload, out string err)
	{
		return JsonFileLoader<ZenTestConfig>.MakeData(this, payload, err, false);
	}

	override protected bool ApplySyncPayload(string payload, out string err)
	{
		return JsonFileLoader<ZenTestConfig>.LoadData(payload, this, err);
	}
	
	// -------------------------
	// SELECTIVE VARIABLES SYNC (COMPLEX)
	// If you only need to sync some data but not all data, split your config into 2 files if necessary 
	// and turn sync off on one of them. But otherwise if you want to keep all server config in one file
	// and still sync the data - or you're using complex data structures - this method may be your only option.
	// -------------------------
	override bool BuildSyncPayload(out string payload, out string err)
	{
		ZenTestConfig_SyncPayload snap = new ZenTestConfig_SyncPayload();

		// Copy into payload (so you never accidentally mutate your config during serialization)
		if (TestMap)
		{
			foreach (string k, ZenTestClass v : TestMap)
			{
				snap.TestMap.Set(k, v);
			}
		}

		// Serialize payload only (NOT the whole config)
		return JsonFileLoader<ZenTestConfig_SyncPayload>.MakeData(snap, payload, err, false);
	}

	override bool ApplySyncPayload(string payload, out string err)
	{
		// Ensure any containers exist before load (prevents null refs if loader behaves oddly)
		if (!TestMap)
		{
			TestMap = new map<string, ref ZenTestClass>;
		}

		ZenTestConfig_SyncPayload snap = new ZenTestConfig_SyncPayload();
		if (!JsonFileLoader<ZenTestConfig_SyncPayload>.LoadData(payload, snap, err))
		{
			return false;
		}

		// Robust: keep the same TestMap instance (in case other code holds a reference to it)
		TestMap.Clear();
		if (snap.TestMap)
		{
			foreach (string k, ZenTestClass v : snap.TestMap)
			{
				TestMap.Set(k, v);
			}
		}

		return true;
	}
	
	*/
	
	override void DoDebugPrint(string trigger = "")
	{
		ZMPrint("Debug print triggered by: " + trigger);
		ZMPrint("EnablePumps=" + EnablePumps + " NewVar=" + NewVar + " AllowedPumpTypesCount=" + AllowedPumpTypes.Count());
		
		foreach (string testStr : AllowedPumpTypes)
		{
			ZMPrint("testStr=" + testStr);
		}
		
		foreach (string key, ZenTestClass element : TestMap)
		{
			ZMPrint("key=" + key + " element=" + element.SomeData);
		}
	}
}