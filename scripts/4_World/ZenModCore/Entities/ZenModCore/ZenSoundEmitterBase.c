class ZenSoundEmitterBase: Inventory_Base
{
	protected bool m_ZenCheckedSoundset;
	protected string m_ZenSoundset;
	protected int m_ZenReplaySecs;
	protected bool m_ZenShouldLoop;
	protected float m_ZenFadeInSecs;
	protected float m_ZenFadeOutSecs;
	
	protected ref EffectSound m_ZenSound;
	protected ref Timer m_ZenReplayTimer;
	
	override void EEInit()
	{
		super.EEInit();
		
		#ifndef SERVER
		LoadZenSoundset();
		#endif
	}
	
	protected void LoadZenSoundset()
	{
		if (!m_ZenCheckedSoundset)
		{
			m_ZenSoundset = ConfigGetString("zenSoundset");
			m_ZenReplaySecs = ConfigGetInt("zenReplaySecs");
			m_ZenShouldLoop = ConfigGetBool("zenShouldLoop");
			m_ZenFadeInSecs = ConfigGetFloat("zenFadeInSecs");
			m_ZenFadeOutSecs = ConfigGetFloat("zenFadeOutSecs");
			m_ZenCheckedSoundset = true;
		}
	}
	
	// Objects spawn at 0 0 0 world coords so play sound after deferred init once it's moved to where it should be.
	override void DeferredInit()
	{
		super.DeferredInit();
		
		PlayZenSoundset();
	}
	
	void PlayZenSoundset()
	{
		#ifndef SERVER
		if (!m_ZenSoundset)
		{
			Error("No valid soundset found for emitter: " + GetType());
			return;
		}
		
		m_ZenSound = SEffectManager.PlaySoundOnObject(m_ZenSoundset, this, m_ZenFadeInSecs, m_ZenFadeOutSecs, m_ZenShouldLoop);
		m_ZenSound.SetSoundAutodestroy(true);
		
		if (!m_ZenShouldLoop && m_ZenReplaySecs > 0 && !m_ZenReplayTimer || !m_ZenReplayTimer.IsRunning())
		{
			m_ZenReplayTimer = new Timer();
			m_ZenReplayTimer.Run(m_ZenReplaySecs, this, "PlayZenSoundset", null, true);
		}
		#endif
		
		if (!m_ZenShouldLoop)
		{
			m_ZenReplayTimer = new Timer();
			m_ZenReplayTimer.Run(10, this, "DeleteSafe", null, true);
		}
	}
	
	string GetZenSoundset()
	{
		return m_ZenSoundset;
	}
	
	override bool DisableVicinityIcon()
	{
		return true;
	}

	override int GetHideIconMask()
	{
		return EInventoryIconVisibility.HIDE_VICINITY;
	}

	override bool CanPutInCargo(EntityAI parent)
	{
		return false;
	}

	override bool CanPutIntoHands(EntityAI parent)
	{
		return false;
	}

	override bool IsTakeable()
	{
		return false;
	}

	override bool CanBeActionTarget()
	{
        return false;
    }
	
	override void AfterStoreLoad()
	{
		super.AfterStoreLoad();
		
		DeleteSafe();
	}
}

class ZenSoundEmitterTest : ZenSoundEmitterBase {};