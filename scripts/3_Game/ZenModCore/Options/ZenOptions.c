class ZenOptionsSaveEntry
{
	string Id;
	int Index;
}

// ---------------------------------------------
// Persistent - saved to disk in unique server path (so client settings are unique per server)
// ---------------------------------------------
class ZenOptionsConfig : ZenConfigBase
{
	ref array<ref ZenOptionsSaveEntry> Entries;

	override string GetRootFolder()
	{
		return ZenConstants.GetClientProfilesFolder();
	}

	override string GetFolderName()
	{
		return "";
	}

	override string GetFileName()
	{
		return "ZenOptions.json";
	}

	override string GetCurrentVersion()
	{
		return "1.29.1";
	}

	override bool ShouldLoadOnServer()
	{
		return false;
	}

	override bool ShouldLoadOnClient()
	{
		return true;
	}

	override bool ShouldSyncToClient()
	{
		return false;
	}

	override bool ShouldSaveOnShutdown()
	{
		return false;
	}

	override void SetDefaults()
	{
		ConfigVersion = GetCurrentVersion();

		if (!Entries)
			Entries = new array<ref ZenOptionsSaveEntry>;
		else
			Entries.Clear();
	}

	protected static ref JsonSerializer s_Serializer = new JsonSerializer;

	override bool ReadJson(string path, out string err)
	{
		return JsonFileLoader<ZenOptionsConfig>.LoadFile(path, this, err);
	}

	override bool WriteJson(string path, out string err)
	{
		return JsonFileLoader<ZenOptionsConfig>.SaveFile(path, this, err);
	}

	override void AfterLoad()
	{
		if (!Entries)
			Entries = new array<ref ZenOptionsSaveEntry>;
	}

	override void AfterSave()
	{
	}

	override void AfterConfigReceived()
	{
	}

	override void DoDebugPrint(string trigger = "")
	{
	}
}

// ---------------------------------------------
// Runtime setting model
// ---------------------------------------------
class ZenModSetting
{
	string Id;
	string CategoryId;

	string Title;
	string Description;

	ref array<string> Options;

	int DefaultIndex;
	int SavedIndex;
	int PendingIndex;

	void ZenModSetting(string id, string categoryId, string title, array<string> options, int defaultIndex = 0, string description = "")
	{
		Id = id;
		CategoryId = categoryId;
		Title = title;
		Description = description;

		Options = new array<string>;
		if (options)
		{
			for (int i = 0; i < options.Count(); i++)
				Options.Insert(options[i]);
		}

		DefaultIndex = ClampIndex(defaultIndex);
		SavedIndex = DefaultIndex;
		PendingIndex = SavedIndex;
	}

	int ClampIndex(int idx)
	{
		int n = 0;

		if (Options)
			n = Options.Count();

		if (n <= 0)
			return 0;

		if (idx < 0)
			return 0;

		if (idx >= n)
			return n - 1;

		return idx;
	}

	string GetSavedValue()
	{
		if (!Options)
			return "";

		if (Options.Count() == 0)
			return "";

		return Options[SavedIndex];
	}

	string GetPendingValue()
	{
		if (!Options)
			return "";

		if (Options.Count() == 0)
			return "";

		return Options[PendingIndex];
	}

	void SetOptions(array<string> newOptions, int newDefaultIndex = 0)
	{
		string savedValue = GetSavedValue();
		string pendingValue = GetPendingValue();

		Options.Clear();
		if (newOptions)
		{
			for (int i = 0; i < newOptions.Count(); i++)
				Options.Insert(newOptions[i]);
		}

		DefaultIndex = ClampIndex(newDefaultIndex);

		int newSaved = Options.Find(savedValue);
		if (newSaved == -1)
			newSaved = ClampIndex(SavedIndex);
		SavedIndex = newSaved;

		int newPending = Options.Find(pendingValue);
		if (newPending == -1)
			newPending = ClampIndex(PendingIndex);
		PendingIndex = newPending;
	}

	void SetSavedIndex(int idx)
	{
		SavedIndex = ClampIndex(idx);
		PendingIndex = SavedIndex;
	}

	void SetPendingIndex(int idx)
	{
		PendingIndex = ClampIndex(idx);
	}

	bool IsChanged()
	{
		if (PendingIndex != SavedIndex)
			return true;

		return false;
	}

	void ApplyPending()
	{
		SavedIndex = PendingIndex;
	}

	void RevertPending()
	{
		PendingIndex = SavedIndex;
	}

	void SetPendingToDefault()
	{
		PendingIndex = DefaultIndex;
	}

	bool GetSavedBool(bool fallback = false)
	{
		if (!Options)
			return fallback;

		if (Options.Count() < 2)
			return fallback;

		if (SavedIndex == 0)
			return false;

		return true;
	}
}

// ---------------------------------------------
// Defines a unique mod category
// ---------------------------------------------
class ZenOptionsCategory
{
	string Id;
	string DisplayName;
	string Description;

	ref array<ref ZenModSetting> Settings;

	void ZenOptionsCategory(string id, string displayName, string description = "")
	{
		Id = id;
		DisplayName = displayName;
		Description = description;

		Settings = new array<ref ZenModSetting>;
	}
}

// ---------------------------------------------
// Registry + integration
// ---------------------------------------------
class ZenOptions
{
	private static ref ZenOptions s_Instance;

	private ref array<ref ZenOptionsCategory> m_CategoryList;
	private ref map<string, ref ZenOptionsCategory> m_CategoryMap;
	private ref map<string, ref ZenModSetting> m_SettingMap;

	private ref map<string, int> m_LoadedValues;

	private ref ZenOptionsConfig m_Config;

	ref ScriptInvoker OnApplied;
	
	// -------- Public functions --------

	static ZenOptions Get()
	{
		if (!s_Instance)
			s_Instance = new ZenOptions();

		return s_Instance;
	}

	void ZenOptions()
	{
		m_CategoryList = new array<ref ZenOptionsCategory>;
		m_CategoryMap = new map<string, ref ZenOptionsCategory>;
		m_SettingMap = new map<string, ref ZenModSetting>;
		m_LoadedValues = new map<string, int>;

		OnApplied = new ScriptInvoker();

		m_Config = new ZenOptionsConfig();
		m_Config.Load();

		CacheLoadedValues();
	}

	static void RegisterCategory(string categoryId, string displayName, string description = "")
	{
		Get().GetOrCreateCategory(categoryId, "#STR_ZenarchistCoreModPrefix " + displayName, description);
	}

	static ZenModSetting AddSetting(string categoryId, string key, string title, array<string> options, int defaultIndex = 0, string description = "")
	{
		string fullId = BuildId(categoryId, key);
		return Get().RegisterOrUpdateSetting(fullId, categoryId, title, options, defaultIndex, description);
	}

	static ZenModSetting AddSetting2(string categoryId, string key, string title, string opt0, string opt1, int defaultIndex = 0, string description = "")
	{
		array<string> opts = { opt0, opt1 };
		return AddSetting(categoryId, key, title, opts, defaultIndex, description);
	}

	static ZenModSetting AddBoolSetting(string categoryId, string key, string title, bool defaultEnabled = false, string description = "", string offText = "#options_controls_disabled", string onText = "#options_controls_enabled")
	{
		array<string> opts = { offText, onText };

		int defIndex;
		if (defaultEnabled)
			defIndex = 1;
		else
			defIndex = 0;

		return AddSetting(categoryId, key, title, opts, defIndex, description);
	}

	static string BuildId(string categoryId, string key)
	{
		return categoryId + "." + key;
	}

	static int GetIndex(string fullId, int fallback = 0)
	{
		ZenOptions inst = Get();
	
		ZenModSetting s;
		if (inst.m_SettingMap.Find(fullId, s))
			return s.SavedIndex;
	
		int loaded;
		if (inst.m_LoadedValues && inst.m_LoadedValues.Find(fullId, loaded))
			return loaded;
	
		return fallback;
	}
	
	static bool GetBool(string fullId, bool fallback = false)
	{
		ZenOptions inst = Get();
	
		ZenModSetting s;
		if (inst.m_SettingMap.Find(fullId, s))
			return s.GetSavedBool(fallback);
	
		int loaded;
		if (inst.m_LoadedValues && inst.m_LoadedValues.Find(fullId, loaded))
		{
			if (loaded == 0)
				return false;
	
			return true;
		}
	
		return fallback;
	}

	array<ref ZenOptionsCategory> GetCategories()
	{
		return m_CategoryList;
	}

	bool IsAnyChanged()
	{
		foreach (string id, ZenModSetting s : m_SettingMap)
		{
			if (s)
			{
				if (s.IsChanged())
					return true;
			}
		}

		return false;
	}

	bool ApplyPending()
	{
		bool any = false;

		foreach (string id, ZenModSetting s : m_SettingMap)
		{
			if (s)
			{
				if (s.IsChanged())
				{
					s.ApplyPending();
					any = true;
				}
			}
		}

		if (any)
		{
			SaveToConfig();
			OnApplied.Invoke();
		}

		return any;
	}

	void RevertPending()
	{
		foreach (string id, ZenModSetting s : m_SettingMap)
		{
			if (s)
				s.RevertPending();
		}
	}

	// -------- Internal functions --------

	private void CacheLoadedValues()
	{
		m_LoadedValues.Clear();

		if (!m_Config)
			return;

		if (!m_Config.Entries)
			return;

		foreach (ZenOptionsSaveEntry e : m_Config.Entries)
		{
			if (e)
			{
				if (e.Id != "")
					m_LoadedValues.Insert(e.Id, e.Index);
			}
		}
	}

	private void SaveToConfig()
	{
		if (!m_Config)
			return;

		if (!m_Config.Entries)
			m_Config.Entries = new array<ref ZenOptionsSaveEntry>;
		else
			m_Config.Entries.Clear();

		foreach (string id, ZenModSetting s : m_SettingMap)
		{
			if (!s)
				continue;

			ZenOptionsSaveEntry e = new ZenOptionsSaveEntry();
			e.Id = s.Id;
			e.Index = s.SavedIndex;

			m_Config.Entries.Insert(e);
		}

		m_Config.Save();
	}

	private ZenOptionsCategory GetOrCreateCategory(string categoryId, string displayName, string description = "")
	{
		displayName = Widget.TranslateString(displayName);
		displayName.ToUpper();
		
		ZenOptionsCategory cat;
		if (m_CategoryMap.Find(categoryId, cat))
		{
			if (displayName != "")
				cat.DisplayName = displayName;

			if (description != "")
				cat.Description = description;

			return cat;
		}

		if (displayName == "")
			displayName = categoryId;

		cat = new ZenOptionsCategory(categoryId, displayName, description);
		m_CategoryMap.Insert(categoryId, cat);
		m_CategoryList.Insert(cat);
		return cat;
	}

	private ZenModSetting RegisterOrUpdateSetting(string fullId, string categoryId, string title, array<string> options, int defaultIndex, string description = "")
	{
		ZenOptionsCategory cat = GetOrCreateCategory(categoryId, "", "");
		
		title = Widget.TranslateString(title);
		title.ToUpper();
	
		ZenModSetting existing;
		if (m_SettingMap.Find(fullId, existing))
		{
			existing.Title = title;
			existing.Description = description;
			existing.SetOptions(options, defaultIndex);
			return existing;
		}
	
		ZenModSetting setting = new ZenModSetting(fullId, categoryId, title, options, defaultIndex, description);
	
		int loadedIdx;
		if (m_LoadedValues.Find(fullId, loadedIdx))
			setting.SetSavedIndex(loadedIdx);
	
		m_SettingMap.Insert(fullId, setting);
		cat.Settings.Insert(setting);
	
		return setting;
	}
}