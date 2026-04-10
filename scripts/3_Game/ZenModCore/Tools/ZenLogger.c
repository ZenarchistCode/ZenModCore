// Buffered writer logger - designed to have minimum impact on server performance by only printing to file periodically instead
// of with every single Log() call to minimize file I/O calls.
class ZenLogger
{
	static bool LogsEnabled = true;
	static string FileSuffix = ".log";
	static const int FLUSH_INTERVAL_MS = 5000;   // flush every ~5s
	static const int MAX_BUFFER_LINES  = 50;     // or when >= 50 lines queued

	protected static ref map<string, ref TStringArray> s_Buffers = new map<string, ref TStringArray>();
	protected static ref map<string, bool> s_EnsuredFolders = new map<string, bool>();
	protected static bool s_FlushQueued;

	// Public: write one log line
	// subFolder: e.g. "Chat"
	// baseFileName:
	//   - "chat" => chatFileSuffix (or chat_2025-12-30FileSuffix if perDay=true)
	//   - ""     => defaults to 2025-12-30FileSuffix if perDay=true
	static void Log(string subFolder, string baseFileName, string text, bool perDay = true)
	{
		if (!LogsEnabled)
			return;
		
		if (GetZenCoreConfig().ZenCore_LogConfig.LogFolderExcludeList.Find(subFolder) != -1)
			return;

		string path = GetLogFilePath(subFolder, baseFileName, perDay);

		// sanitize newlines so one message = one line in file
		text.Replace("\r", "");
		text.Replace("\n", "\\n");

		BufferLine(path, text);
	}

	// Call this on shutdown to ensure nothing is lost
	static void FlushAll()
	{
		s_FlushQueued = false;

		foreach (string path, TStringArray buf : s_Buffers)
		{
			if (buf && buf.Count() > 0)
				FlushPath(path);
		}
	}
	
	// Deletes FileSuffix files whose filename contains a date older than keepDays.
	// Returns number of deleted files.
	static int CleanupOldLogs(int keepDays)
	{
		// If logs folder doesn't exist, nothing to do
		if (!FileExist(ZenConstants.GetLogFolder()))
			return 0;

		if (keepDays < 0)
			keepDays = 0;

		int y, m, d;
		GetYearMonthDay(y, m, d);
		int todayDays = ZenGameFunctions.DaysFromCivil(y, m, d);

		ZMPrint("[ZenLogger] Cleaning up files which are " + keepDays + "+ days old.");
		return CleanupFolderRecursive(ZenConstants.GetLogFolder(), keepDays, todayDays);
	}

	// -------------------------
	// Internals
	// -------------------------
	private static string GetLogFilePath(string subFolder, string baseFileName, bool perDay)
	{
		string folder = ZenConstants.GetLogFolder();
		if (subFolder != "")
			folder += subFolder + "\\";

		EnsureFolderOnce(folder);

		string date = ZMGetDateStamp(); // YYYY-MM-DD

		string file = baseFileName;
		if (perDay)
		{
			if (file == "")
				file = date;                 // "2025-12-30"
			else
				file = file + "_" + date;    // "chat_2025-12-30"
		}

		if (file == "")
			file = "log_" + date;

		if (file.IndexOf(FileSuffix) == -1)
			file += FileSuffix;

		return folder + file;
	}

	private static void EnsureFolderOnce(string folder)
	{
		bool done;
		if (s_EnsuredFolders.Find(folder, done) && done)
			return;

		ZenGameFunctions.EnsureDirectoriesExist(folder);
		s_EnsuredFolders.Set(folder, true);
	}

	private static void BufferLine(string path, string line)
	{
		TStringArray buf;
		if (!s_Buffers.Find(path, buf) || !buf)
		{
			buf = new TStringArray;
			s_Buffers.Set(path, buf);
		}

		buf.Insert("[" + ZMGetDate() + "] " + line);

		// If we have a lot queued, flush immediately
		if (buf.Count() >= MAX_BUFFER_LINES)
			FlushPath(path);

		QueueFlush();
	}

	private static void QueueFlush()
	{
		if (s_FlushQueued)
			return;

		s_FlushQueued = true;

		// flush later (avoids disk IO on every chat message)
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(FlushAll, FLUSH_INTERVAL_MS, false);
	}

	private static void FlushPath(string path)
	{
		TStringArray buf;
		if (!s_Buffers.Find(path, buf) || !buf || buf.Count() == 0)
			return;

		// Append if possible
		FileHandle fh = OpenFile(path, FileMode.APPEND);
		if (fh == 0)
			fh = OpenFile(path, FileMode.WRITE);

		if (fh == 0)
			return; // keep buffer, try next flush

		foreach (string l : buf)
			FPrint(fh, l + "\n");

		CloseFile(fh);
		buf.Clear();
	}

	// Returns true if file ends with ".FileSuffix"
	private static bool IsTxtFile(string fileName)
	{
		int len = fileName.Length();
		if (len < FileSuffix.Length())
			return false;

		return fileName.Substring(len - FileSuffix.Length(), FileSuffix.Length()) == FileSuffix;
	}

	// Tries to parse a date from a filename token.
	// Accepts:
	//   2025-12-30FileSuffix
	//   Chat_2025-12-30FileSuffix
	private static bool ParseDateFromFileName(string fileName, out int y, out int m, out int d)
	{
		y = 0; m = 0; d = 0;

		// Strip extension
		int dot = fileName.LastIndexOf(".");
		string baseName = fileName;
		if (dot > 0) 
			baseName = fileName.Substring(0, dot);

		// Split by '_' and try tokens from the end (most common: prefix_date)
		TStringArray tokens = new TStringArray;
		baseName.Split("_", tokens);

		for (int t = tokens.Count() - 1; t >= 0; t--)
		{
			string candidate = tokens.Get(t);

			TStringArray bits = new TStringArray;
			candidate.Split("-", bits);
			if (bits.Count() != 3)
				continue;

			// YYYY-MM-DD only
			if (bits.Get(0).Length() != 4)
				continue;
			
			y = bits.Get(0).ToInt();
			m = bits.Get(1).ToInt();
			d = bits.Get(2).ToInt();

			// Basic sanity
			if (y >= 1970 && m >= 1 && m <= 12 && d >= 1 && d <= 31)
				return true;
		}

		return false;
	}

	// Recursively walk folders starting at folderPath and delete old FileSuffix logs.
	private static int CleanupFolderRecursive(string folderPath, int keepDays, int todayDays)
	{
		int deleted = 0;

		folderPath.Replace("/", "\\");
		if (folderPath.Length() > 0 && folderPath.Get(folderPath.Length() - 1) != "\\")
			folderPath += "\\";

		string fileName;
		FileAttr attr;

		// Enumerate everything in this folder
		FindFileHandle h = FindFile(folderPath + "*", fileName, attr, 0);
		if (!h)
			return 0;

		while (true)
		{
			if (fileName != "" && fileName != "." && fileName != "..")
			{
				if (attr == FileAttr.DIRECTORY)
				{
					// Recurse into subfolder
					deleted += CleanupFolderRecursive(folderPath + fileName + "\\", keepDays, todayDays);
				}
				else
				{
					// Only touch FileSuffix logs
					if (IsTxtFile(fileName))
					{
						int y, m, d;
						if (ParseDateFromFileName(fileName, y, m, d))
						{
							int fileDays = ZenGameFunctions.DaysFromCivil(y, m, d);
							int ageDays = todayDays - fileDays;

							// Delete if strictly older than keepDays
							if (ageDays > keepDays)
							{
								string fullPath = folderPath + fileName;
								if (DeleteFile(fullPath)) // works in $profile: :contentReference[oaicite:2]{index=2}
									deleted++;
							}
						}
						
						// If it has no parseable date, we leave it alone.
					}
				}
			}

			if (!FindNextFile(h, fileName, attr))
				break;
		}

		CloseFindFile(h);
		return deleted;
	}
}
