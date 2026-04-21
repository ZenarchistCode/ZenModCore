class ZenGameFunctions
{
	static bool HasInit;
	static int ServerSessionCount = -1;
	
	static void Init()
	{
		#ifdef SERVER
		EnsureDirectoriesExist(ZenConstants.GetDbFolder());
		EnsureDirectoriesExist(ZenConstants.GetProfilesFolder());
		UpdateServerSessionCount();
		#else
		EnsureDirectoriesExist(ZenConstants.GetProfilesFolder());
		EnsureDirectoriesExist(ZenConstants.GetClientProfilesFolder());
		#endif
		
		HasInit = true;
	}
	
	static void UpdateServerSessionCount()
	{
		int count = 0;

		// If we already have a stored count, load it and increment for this new boot
		if (FileExist(SESSION_COUNT_FILE))
		{
			FileSerializer r = new FileSerializer();
			if (r.Open(SESSION_COUNT_FILE, FileMode.READ))
			{
				// File contains exactly 1 int
				r.Read(count);
				r.Close();
			}

			count = count + 1;
		}
		else
		{
			// First ever boot after wipe (or file removed) => session 0
			count = 0;
		}

		ServerSessionCount = count;

		// Persist (create or overwrite)
		FileSerializer w = new FileSerializer();
		if (w.Open(SESSION_COUNT_FILE, FileMode.WRITE))
		{
			w.Write(count);
			w.Close();
		}
	}
	
	static bool IsFreshWipe()
	{
		if (!HasInit)
		{
			Error("Called IsFreshWipe() before server has initialized properly!");
		}
		
		return ServerSessionCount == 0;
	}
	
	static bool IsWinter(bool countLateOrEarlyWinter = true)
	{
		if (IsDeepWinter())
			return true;

		if (countLateOrEarlyWinter)
			return IsLateOrEarlyWinter();

		return false;
	}

	static bool IsDeepWinter()
	{
		#ifdef WinterChernarusV2
		return true;
		#endif

		#ifdef WinterLivoniaWorld
		return true;
		#endif

		return false;
	}

	static bool IsLateOrEarlyWinter()
	{
		#ifdef earlywinter_scripts
		return true;
		#endif 

		#ifdef earlywinter_livonia_scripts
		return true;
		#endif

		return false;
	}

	// For event-related auto-code.
	static bool IsChristmas()
	{
		int year, month, day;
		GetYearMonthDay(year, month, day);
		return month == 12 && day >= 24 && day <= 25;
	}

	static bool IsEaster()
	{
		int year, month, day;
		GetYearMonthDay(year, month, day);
		return month == 4 && day >= 5 && day <= 6;
	}

	static bool IsHalloween()
	{
		int year, month, day;
		GetYearMonthDay(year, month, day);
		return month == 10 && day >= 30 && day <= 31;
	}

	static bool IsAustraliaDay()
	{
		int year, month, day;
		GetYearMonthDay(year, month, day);
		return month == 1 && day >= 25 && day <= 27;
	}
	
	static Object SpawnObject(string type, vector position, vector orientation = "0 0 0", float scale = 1.0, int flags = ECE_SETUP, bool allowDuplicate = true)
	{
		if (!allowDuplicate)
		{
			// Get objects within 0.1 meter of the spawn
		    array<Object> objectsNearby = new array<Object>;
		    g_Game.GetObjectsAtPosition3D(position, 0.1, objectsNearby, null);
			
			// Delete any existing objects
		    foreach (Object z_obj : objectsNearby)
		    {
		        if (z_obj.GetType() == type && z_obj.GetPosition() == position && z_obj.GetOrientation() == orientation)
		        {
		            g_Game.ObjectDelete(z_obj);
					Error("DUPLICATE OBJECT FOUND @ " + position + " - " + type);
					return z_obj;
		        }
		    }
		}
	    
		Object obj = g_Game.CreateObjectEx(type, position, flags, RF_IGNORE);
	    if (!obj) 
	    {
	        Error("Failed to create object " + type);
	        return NULL;
	    }
	
	    obj.SetPosition(position);
	    obj.SetOrientation(orientation);
	    obj.SetScale(scale);
	    obj.Update();
	    obj.SetAffectPathgraph(true, false);
	    
	    if (obj.CanAffectPathgraph()) 
	    {
	        g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(g_Game.UpdatePathgraphRegionByObject, 100, false, obj);
	    }
	    
	    return obj;
	}
	
	static Object SpawnObjectRelative(
		Object parent,
		string type,
		vector localOffsetPos,
		vector localOffsetOri = "0 0 0",
		float scale = 1.0,
		int createFlags = ECE_SETUP,
		int createRotationFlags = RF_IGNORE)
	{
		if (!parent) return null;
	
		// 1) Parent world transform
		vector parentTM[4];
		parent.GetTransform(parentTM);
	
		// 2) Local offset transform (rotation + translation)
		vector offRot[3];
		Math3D.YawPitchRollMatrix(localOffsetOri, offRot);
	
		vector offTM[4];
		offTM[0] = offRot[0];
		offTM[1] = offRot[1];
		offTM[2] = offRot[2];
		offTM[3] = localOffsetPos; // [right, up, forward]
	
		// 3) World = parent * offset
		vector worldTM[4];
		Math3D.MatrixMultiply4(parentTM, offTM, worldTM);
	
		// Spawn at the computed world position (worldTM[3])
		Object obj = g_Game.CreateObjectEx(type, worldTM[3], createFlags, createRotationFlags);
		if (!obj) return null;
	
		// Apply full transform in one go (more consistent than SetPosition+SetOrientation)
		obj.SetTransform(worldTM);
	
		// Scale (see notes below)
		if (scale != 1.0)
		{
			obj.SetScale(scale);
		}
	
		// You do NOT want pathgraph updates for a ghost proxy
		obj.SetAffectPathgraph(false, false);
	
		return obj;
	}
	
	//! Aligns given object to underlying terrain
	static void AlignToTerrain(Object obj)
	{
		vector transform[4];
		obj.GetTransform(transform);
		vector ground_position, ground_dir; 
		int component;
		DayZPhysics.RaycastRV(transform[3], transform[3] + transform[1] * -1000, ground_position, ground_dir, component, null, null, null, false, true);
		vector surface_normal = g_Game.SurfaceGetNormal(ground_position[0], ground_position[2]);
		vector local_ori = obj.GetDirection();
		transform[0] = surface_normal * local_ori;
		transform[1] = surface_normal;
		transform[2] = surface_normal * (local_ori * vector.Up);
		obj.SetTransform(transform);
		obj.Update();
	}

	private static bool HAS_PRINTED_MODS;
	static void PrintMods() 
	{
		if (HAS_PRINTED_MODS)
			return;

		int mod_count = g_Game.ConfigGetChildrenCount("CfgMods");
		string mod_name;

		Print("[ZenModCore] Listing CfgMods:");

		for (int i = 0; i < mod_count; i++)
		{
			g_Game.ConfigGetChildName("CfgMods", i, mod_name);
			Print("#ifdef " + mod_name);
		}
		
		HAS_PRINTED_MODS = true;
	}

	// Creates ALL folders in a path, one-by-one (mkdir -p)
	// Works with $profile: style paths too.
	static void EnsureDirectoriesExist(string folderPath)
	{
		if (folderPath == "")
			return;
		
		if (folderPath.Contains("storage_"))
			folderPath.ToLower();
	
		// Normalize separators
		folderPath.Replace("/", "\\");
	
		// Trim trailing slashes so Split doesn't end with ""
		while (folderPath.Length() > 0 && folderPath.Get(folderPath.Length() - 1) == "\\")
		{
			folderPath = folderPath.Substring(0, folderPath.Length() - 1);
		}
	
		TStringArray parts = new TStringArray;
		folderPath.Split("\\", parts);
	
		string current = "";
		for (int i = 0; i < parts.Count(); i++)
		{
			string part = parts.Get(i);
			if (part == "")
				continue;
	
			if (current == "")
				current = part;
			else
				current = current + "\\" + part;
	
			// Skip creating the prefix itself ("$profile:" / "$mission:" etc.)
			if (current.Length() > 0 && current.Get(current.Length() - 1) == ":")
				continue;
	
			if (!FileExist(current))
				MakeDirectory(current);
		}
	}
	
	// Gregorian date -> days since 1970-01-01
	static int DaysFromCivil(int y, int m, int d)
	{
		// y -= (m <= 2);
		if (m <= 2)
			y -= 1;
	
		int era;
		if (y >= 0)
			era = y / 400;
		else
			era = (y - 399) / 400;
	
		int yoe = y - era * 400; // [0, 399]
	
		// int mp = m + ((m > 2) ? -3 : 9);
		int mp;
		if (m > 2)
			mp = m - 3;   // Mar=0..Dec=9
		else
			mp = m + 9;   // Jan=10, Feb=11
	
		int doy = (153 * mp + 2) / 5 + d - 1;            // [0, 365]
		int doe = yoe * 365 + (yoe / 4) - (yoe / 100) + doy; // [0, 146096]
	
		return era * 146097 + doe - 719468; // 1970-01-01 -> 0
	}
	
	// Finds YYYY-MM-DD anywhere in the filename (before extension).
	// Works for: script_2026-01-01_19-04-57.log
	//            DayZ_x64_2026-01-01_19-26-47.rpt
	//            workbenchApp_2026-01-01_16-03-26.mdmp ETC
	static bool ParseDateFromVanillaProfileName(string fileName, out int y, out int m, out int d)
	{
		y = 0; m = 0; d = 0;
	
		// Strip extension
		int dot = fileName.LastIndexOf(".");
		string baseName = fileName;
		if (dot > 0)
			baseName = fileName.Substring(0, dot);
	
		// Normalize separators so we only deal with '-'
		baseName.Replace("_", "-");
	
		TStringArray parts = new TStringArray;
		baseName.Split("-", parts);
	
		// Look for YYYY-MM-DD sequence
		for (int i = 0; i < parts.Count() - 2; i++)
		{
			string sy = parts.Get(i);
			string sm = parts.Get(i + 1);
			string sd = parts.Get(i + 2);
	
			// Fast shape check
			if (sy.Length() != 4 || sm.Length() != 2 || sd.Length() != 2)
				continue;
	
			int yy = sy.ToInt();
			int mm = sm.ToInt();
			int dd = sd.ToInt();
	
			// Sanity
			if (yy >= 1970 && mm >= 1 && mm <= 12 && dd >= 1 && dd <= 31)
			{
				y = yy;
				m = mm;
				d = dd;
				return true;
			}
		}
	
		return false;
	}
	
	// Returns true if the given string ends with the given suffix
	static bool EndsWith(string s, string suffix)
	{
		int len = s.Length();
		int slen = suffix.Length();
	
		if (len < slen)
			return false;
	
		return s.Substring(len - slen, slen) == suffix;
	}
	
	// Returns true if the given key code is a number or backspace/caret mover key
	static bool IsKeyNumber(int keyCode, bool allowBackspaceEtc = true)
	{
		switch (keyCode)
		{
			case KeyCode.KC_0:
			case KeyCode.KC_1:
			case KeyCode.KC_2:
			case KeyCode.KC_3:
			case KeyCode.KC_4:
			case KeyCode.KC_5:
			case KeyCode.KC_6:
			case KeyCode.KC_7:
			case KeyCode.KC_8:
			case KeyCode.KC_9:

			case KeyCode.KC_NUMPAD0:
			case KeyCode.KC_NUMPAD1:
			case KeyCode.KC_NUMPAD2:
			case KeyCode.KC_NUMPAD3:
			case KeyCode.KC_NUMPAD4:
			case KeyCode.KC_NUMPAD5:
			case KeyCode.KC_NUMPAD6:
			case KeyCode.KC_NUMPAD7:
			case KeyCode.KC_NUMPAD8:
			case KeyCode.KC_NUMPAD9:
				return true;
		}
		
		if (allowBackspaceEtc)
		{
			switch (keyCode)
			{
				case KeyCode.KC_BACK:
				case KeyCode.KC_DELETE:
				case KeyCode.KC_LEFT:
				case KeyCode.KC_RIGHT:
					return true;
			}
		}
		
		return false;
	}
	
	// Is the given character a digit?
	static bool IsNumberChar(string c)
	{
		if (c.Length() != 1)
			return false;
	
		return "0123456789".IndexOf(c) != -1;
	}
	
	// Is the given string a number?
	static bool IsAllNumbers(string s)
	{
		if (s == "")
			return false;
	
		for (int i = 0; i < s.Length(); i++)
		{
			if (!IsNumberChar(s.Substring(i, 1)))
				return false;
		}
	
		return true;
	}
	
	// Counts how many times the given needle appears in the given 's' string
	static int CountInString(string s, string needle)
	{
		if (s == "" || needle == "")
			return 0;
	
		int count = 0;
		int pos = 0;
		int len = needle.Length();
	
		while (true)
		{
			pos = s.IndexOfFrom(pos, needle);
			if (pos == -1)
				break;
	
			count++;
			pos = pos + len; // move past this match
		}
	
		return count;
	}
	
	// Returns a clickable izurvive map link for the given position
	static string GetMapLinkPosition(vector pos, string mapName = "")
	{
		// get izurvive link
		string MapURL = GetZenCoreConfig().MapIzurviveURL;
		string mapLink = "";

		if (MapURL == "")
		{
			if (mapName != "")
			{
				mapLink = mapName + " @ " + pos[0] + " / " + pos[2];
			}
			else 
			{
				mapLink = "@ " + pos[0] + " / " + pos[2];
			}
		}
		else 
		{
			if (mapName != "")
			{
				"[" + mapName + " @ " + pos[0] + " / " + pos[2] + "](" + MapURL + "#location=" + pos[0] + ";" + pos[2] + ")";
			}
			else 
			{
				mapLink = "[@ " + pos[0] + " / " + pos[2] + "](" + MapURL + "#location=" + pos[0] + ";" + pos[2] + ")";
			}
		}

		return mapLink;
	}
	
	// Get readable date formatted
	static string GetDate(int p_dateFormat = 6, int day = 1, int month = 1, int year = 1)
	{
		// 0 = no date.
		if (p_dateFormat == 0)
		{
			return "";
		}

		// Prepare date strings
		string date;
		string dayNumber;
		string monthNumber;
		string dayStr;
		string monthStr;

		// Get day number (eg. convert 07 -> 7)
		dayNumber = day.ToStringLen(2);
		if (day <= 9)
			dayNumber = day.ToStringLen(1);

		// Get month number (eg. convert 07 -> 7)
		monthNumber = month.ToStringLen(2);
		if (month <= 9)
			monthNumber = month.ToStringLen(1);

		if (p_dateFormat == 1) // eg. 23rd September, 2022
		{
			// Get formatted date 
			dayStr = GetDateSuffix(day);
			monthStr = GetMonth(month);
			date = dayNumber + dayStr + " " + monthStr + ", " + year.ToStringLen(4);
		}
		else
		if (p_dateFormat == 2) // eg. 23/09/2022
		{
			date = day.ToStringLen(2) + "/" + month.ToStringLen(2) + "/" + year.ToStringLen(4);
		}
		else
		if (p_dateFormat == 3) // eg. 3/6/2022
		{
			date = dayNumber + "/" + monthNumber + "/" + year.ToStringLen(4);
		}
		else
		if (p_dateFormat == 4) // eg. 09/23/22
		{
			date = month.ToStringLen(2) + "/" + day.ToStringLen(2) + "/" + year.ToStringLen(4);
		}
		else
		if (p_dateFormat == 5) // eg. 12/9/22
		{
			date = monthNumber + "/" + dayNumber + "/" + year.ToStringLen(4);
		}
		else
		if (p_dateFormat == 6) // eg. 23rd September
		{
			// Get formatted date 
			dayStr = GetDateSuffix(day);
			monthStr = GetMonth(month);
			date = dayNumber + dayStr + " " + monthStr;
		}

		return date;
	}

	// Returns the suffix of a date number
	static string GetDateSuffix(int number)
	{
		switch (number)
		{
			case 1:
			case 21:
			case 31:
				return "st"; // 1st
			case 2:
			case 22:
				return "nd"; // 2nd
			case 3:
			case 23:
				return "rd"; // 3rd
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
			case 11:
			case 12:
			case 13:
			case 14:
			case 15:
			case 16:
			case 17:
			case 18:
			case 19:
			case 20:
			case 24:
			case 25:
			case 26:
			case 27:
			case 28:
			case 29:
			case 30:
				return "th"; // 4th
		}

		return "";
	}

	// Gets the month as a string
	static string GetMonth(int month)
	{
		if (month < 1 || month > 12)
		{
			int year, monthToday, day;
			GetYearMonthDay(year, monthToday, day);
			return MONTH_NAME[monthToday - 1];
		}

		return MONTH_NAME[month - 1];
	}
	
	static const ref array<string> MONTH_NAME =
	{
		"#STR_ZenMonth_January",
		"#STR_ZenMonth_February",
		"#STR_ZenMonth_March",
		"#STR_ZenMonth_April",
		"#STR_ZenMonth_May",
		"#STR_ZenMonth_June",
		"#STR_ZenMonth_July",
		"#STR_ZenMonth_August",
		"#STR_ZenMonth_September",
		"#STR_ZenMonth_October",
		"#STR_ZenMonth_November",
		"#STR_ZenMonth_December"
	};
	
	static string GetTrueObjectName(Object objTarget)
	{
		if (!objTarget)
			return "null object";
		
		string objType = objTarget.GetType();
		if (objType != "")
			return objType;

		objType = objTarget.GetDebugName();
		if (objType != "")
			return objType;
		
		objType = objTarget.GetDebugNameNative();
		if (objType != "")
			return objType;
		
		objType = objTarget.GetModelName();
		if (objType != "")
			return objType;
		
		objType = objTarget.ClassName();
		if (objType != "")
			return objType;
		
		return "unknown entity";
	}
	
	static vector m_WorldCenterPosition = vector.Zero;
	
	// In true DayZ fashion, the vector given by this vanilla centerPosition var has its xyz in the wrong order
	static vector GetWorldCenterPosition()
	{
		if (m_WorldCenterPosition == vector.Zero)
		{
			string path = "CfgWorlds " + g_Game.GetWorldName();
			vector temp = g_Game.ConfigGetVector( path + " centerPosition" );
			m_WorldCenterPosition = Vector(temp[0], temp[2], temp[1]);
		}
		
		return m_WorldCenterPosition;
	}
	
	// Random constants we don't need to worry about editing or checking ever
	private static const string SESSION_COUNT_FILE = ZenConstants.GetDbFolder() + "zen_session_count.bin";
}

// Only use this for printlogs where the timestamp matters for debugging.
// Don't use it for frivilous prints as it may be expensive if spammed, not 100% sure.
static void ZMPrint(Object obj)
{
	ZMPrint(obj.ToString());
}

static void ZMPrint(string s)
{
	Print("[ZEN|" + ZMGetDate() + "] " + s);
}

static void ZMLog(string subFolder, string fileName, string text, bool perDay = true)
{
	ZenLogger.Log(subFolder, fileName, text, perDay);
}

// YYYY-MM-DD-TIME
static string ZMGetDate(bool fileFriendly = false)
{
	int year, month, day, hour, minute, second;

	GetYearMonthDay(year, month, day);
	GetHourMinuteSecond(hour, minute, second);
	
	string date = year.ToStringLen(4) + "." + month.ToStringLen(2) + "." + day.ToStringLen(2) + " " + hour.ToStringLen(2) + ":" + minute.ToStringLen(2) + ":" + second.ToStringLen(2);

	if (fileFriendly)
	{
		date.Replace(" ", "_");
		date.Replace(".", "-");
		date.Replace(":", "-");
	}

	return date;
}

// YYYY-MM-DD
static string ZMGetDateStamp()
{
	int y, m, d;
	GetYearMonthDay(y, m, d);

	return y.ToStringLen(4) + "-" + m.ToStringLen(2) + "-" + d.ToStringLen(2);
}

class ZenSoundEmitter 
{
	static bool Spawn(string type, vector position)
	{
		if (!g_Game.ConfigIsExisting("CfgVehicles " + type + " zenSoundset"))
		{
			Error("Failed to spawn sound emitter - emitter is invalid: " + type + " @ " + position);
			return false;
		}
	
		ZenGameFunctions.SpawnObject(type, position);
		return true;
	}
}