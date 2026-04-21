modded class ItemBase
{
	protected bool 		m_WasZenHologrammed;
	protected bool 		m_ZenHasDoneFirstInit;
	protected int 		m_ZenCreationTimestamp;
	protected int 		m_ZenFirstTouchedTimestamp;
	protected bool      m_ZenWasCECreated;
	
	void ItemBase()
	{
		m_ZenWasCECreated = false;
	}
	
	// This is called before DeferredInit - if we were created by loot economy, trigger that flag here.
	override void EEOnCECreate()
	{
		super.EEOnCECreate();
		
		ZenEEOnCECreate();
	}
	
	// Some vanilla item's scripts do not call super on EEOnCECreate.... 
	// so, a hacky way around it: call ZenEEOnCECreate from within the child class.
	void ZenEEOnCECreate()
	{
		m_ZenWasCECreated = true;
	}
	
	// This func is called AFTER the above EEOnCECreate() funcs.
	override void DeferredInit()
	{
		super.DeferredInit();
		
		#ifdef SERVER
		if (!m_ZenHasDoneFirstInit)
		{
			OnZenFirstInit();
		}
		#endif
	}
	
	void OnZenFirstInit()
	{
		m_ZenHasDoneFirstInit = true;

		m_ZenCreationTimestamp = CF_Date.Now().GetTimestamp();
		m_ZenFirstTouchedTimestamp = m_ZenCreationTimestamp;
		
		if (m_ZenWasCECreated) // Was NOT created via script - set last touched timestamp to zero. This is a virgin, world-spawned item.
		{
			m_ZenFirstTouchedTimestamp = 0;
		}
	}

	int GetZenCreationTimestamp() 
	{
		return m_ZenCreationTimestamp;
	}
	
	void SetZenCreationTimestamp(int ts)
	{
		m_ZenCreationTimestamp = ts;
	}
	
	int GetZenFirstTouchedTimestamp() 
	{
		return m_ZenFirstTouchedTimestamp;
	}
	
	void SetZenFirstTouchedTimestamp(int ts)
	{
		m_ZenFirstTouchedTimestamp = ts;
	}
	
	bool IsZenVirgin()
	{
		if (m_ZenFirstTouchedTimestamp <= 0)
			return true; // :(
		
		return false;
	}
	
	bool WasZenCECreated()
	{
		return m_ZenWasCECreated;
	}

	void SetZenHologrammed(bool hologram, string textureOverride = "")
	{
		m_WasZenHologrammed = hologram;

		array<string> config_textures = GetHiddenSelectionsTextures();
		if (!config_textures || config_textures.Count() == 0)
			return;

		if (m_WasZenHologrammed)
		{
			string textureAlpha = "#(argb,8,8,3)color(1,1,1,0.1,ca)";
			if (textureOverride != "")
				textureAlpha = textureOverride;

			SetObjectTexture(0, textureAlpha);
		}
		else
		{
			string textureNoAlpha = config_textures.Get(0);
			SetObjectTexture(0, textureNoAlpha);
		}
	}

	bool IsZenHologrammed()
	{
		return m_WasZenHologrammed;
	}

	bool ShouldZenHologram()
	{
		#ifdef ZenModPack
		return ZenModEnabled("ZenHologram");
		#endif
		
		return false;
	}
	
	override void OnItemLocationChanged(EntityAI old_owner, EntityAI new_owner)
	{
		super.OnItemLocationChanged(old_owner, new_owner);
		
		if (!g_Game.IsDedicatedServer())
			return;
	
		if (m_ZenFirstTouchedTimestamp == 0 && new_owner != null && new_owner.IsMan())
		{
			// Delay this by a tick so any functions which call OnItemLocationChanged() can still return true for IsVirgin check
			g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(SetZenTouchedTimestamp, 1, false);
		}
		
//		if (!GetZenUtilitiesConfig().ShouldLogLootCyclers)
//			return;

		// If new owner is not null, item was not dropped onto the ground.
//		if (new_owner != null)
//			return;

		// If old owner is null, it wasn't dropped by a player.
//		if (old_owner == null)
//			return;

//		PlayerBase player = PlayerBase.Cast(old_owner);
//		if (!player)
//			player = PlayerBase.Cast(old_owner.GetHierarchyRootPlayer());

		// If it was not dropped by a player, nothing to log.
//		if (!player)
//			return;
		
		//! TODO: We can now check:
		// 1. Was the timestamp difference between now and first touched < X minutes?
		// 2. If so, increase player's loot cycle counter - if it exceeds X per session flag them for potential cycling?
	}
	
	void SetZenTouchedTimestamp()
	{
		m_ZenFirstTouchedTimestamp = CF_Date.Now().GetTimestamp();
	}
	
	// USE CF_Load/Save because this will NOT break server persistence if the mod is added/removed mid-wipe. 
	// NOTE: storage[] must refer to this mod's CfgMods classname EXACTLY or this won't work.
	// Version control is handled in mod.cpp's storageVersion = X; value in CfgMods.
	
	override void CF_OnStoreSave(CF_ModStorageMap storage)
	{
		super.CF_OnStoreSave(storage);

		auto ctx = storage["ZenModCore"];
		if (!ctx) return;
		
		ctx.Write(m_ZenHasDoneFirstInit);
		ctx.Write(m_ZenFirstTouchedTimestamp);
		ctx.Write(m_ZenCreationTimestamp);
	}

	override bool CF_OnStoreLoad(CF_ModStorageMap storage)
	{
		if (!super.CF_OnStoreLoad(storage)) return false;

		auto ctx = storage["ZenModCore"];
		if (!ctx) return true;

		if (ctx.GetVersion() >= 1)
		{
			if (!ctx.Read(m_ZenHasDoneFirstInit)) return false;
			if (!ctx.Read(m_ZenFirstTouchedTimestamp)) return false;
			if (!ctx.Read(m_ZenCreationTimestamp)) return false;
		}

		return true;
	}
}