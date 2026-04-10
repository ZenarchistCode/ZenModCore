class ZenFileCleaner
{
	// Deletes timestamped vanilla files in the ROOT of $profile: (no subfolders except for my ZenCrashReporterLogs folder)
	// Matches: *_YYYY-MM-DD_*.(log|rpt|adm)  (also supports YYYY_MM_DD)
	// Returns number deleted.
	static int CleanupVanillaProfileFiles(int keepDays)
	{
		if (!g_Game) // allow server or client
			return 0;

		if (keepDays < 0)
			keepDays = 0;

		int y, m, d;
		GetYearMonthDay(y, m, d);
		int todayDays = ZenGameFunctions.DaysFromCivil(y, m, d);

		int deleted = 0;

		// 1) Root of profiles
		deleted += CleanupTimestampedFilesInFolder("$profile:", keepDays, todayDays, true);

		// 2) profiles/ZenCrashReporterLogs/  (if it exists)
		deleted += CleanupTimestampedFilesInFolder("$profile:ZenCrashReporterLogs\\", keepDays, todayDays, false);

		return deleted;
	}
	
	// Scans ONE folder (non-recursive). If folder doesn't exist, returns 0.
	// If vanillaExts=true uses IsVanillaProfileExt (log/rpt/adm/mdmp); otherwise only ".log" (tweak as needed).
	private static int CleanupTimestampedFilesInFolder(string folder, int keepDays, int todayDays, bool vanillaExts)
	{
		// Make sure non-alias folders end with a slash
		if (!ZenGameFunctions.EndsWith(folder, ":") && !ZenGameFunctions.EndsWith(folder, "\\") && !ZenGameFunctions.EndsWith(folder, "/"))
			folder += "\\";

		string fileName;
		FileAttr attr;

		FindFileHandle h = FindFile(folder + "*", fileName, attr, 0);
		if (!h)
			return 0; // folder missing / not readable

		int deleted = 0;

		while (true)
		{
			if (fileName != "" && fileName != "." && fileName != "..")
			{
				if (attr != FileAttr.DIRECTORY)
				{
					bool extOk;
					if (vanillaExts)
					{
						extOk = IsVanillaProfileExt(fileName);
					}
					else
					{
						// ZenCrashReporterLogs: "logs" only (change if you also want .mdmp etc)
						string lower = fileName;
						lower.ToLower();
						extOk = ZenGameFunctions.EndsWith(lower, ".log");
					}

					if (extOk)
					{
						int fy, fm, fd;
						if (ZenGameFunctions.ParseDateFromVanillaProfileName(fileName, fy, fm, fd))
						{
							int fileDays = ZenGameFunctions.DaysFromCivil(fy, fm, fd);
							int ageDays = todayDays - fileDays;

							// Ignore future-dated files (just in case)
							if (ageDays > keepDays)
							{
								if (DeleteFile(folder + fileName))
									deleted++;
							}
						}
					}
				}
			}

			if (!FindNextFile(h, fileName, attr))
				break;
		}

		CloseFindFile(h);
		return deleted;
	}
	
	private static bool IsVanillaProfileExt(string fileName)
	{
		string lower = fileName;
		lower.ToLower();
	
		return ZenGameFunctions.EndsWith(lower, ".log") || ZenGameFunctions.EndsWith(lower, ".rpt") || ZenGameFunctions.EndsWith(lower, ".adm") || ZenGameFunctions.EndsWith(lower, ".mdmp");
	}
}