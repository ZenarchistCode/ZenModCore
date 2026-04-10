modded class ManBase
{
	/*
	THERE SEEMS TO BE A BUG WITH EEKilled in vanilla code in AnimalBase (not sure if it affects ManBase but put this here anyway): 
	Sometimes EEKilled(Object killer) passes the entity itself as the "killer". 
	Which makes tracking kills via EEKilled inconsistent.
	This method seems to work ok - not great but better than nothing!
	*/
	protected bool m_ZenWasAlive = false;
	protected bool m_ZenKilledProcessed = false;
	
	override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		m_ZenWasAlive = IsAlive();
		return super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
	}
	
	bool ZenWasAlive()
	{
		return m_ZenWasAlive;
	}
	
	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
		
		if (!source)
			return;

		if (!IsAlive() && m_ZenWasAlive)
		{
			ZenHandleEntityKilledInternal(source);
		}
	}
	
	private void ZenHandleEntityKilledInternal(notnull Object killer)
	{
		m_ZenWasAlive = false;
		
		if (m_ZenKilledProcessed)
			return;
		
		m_ZenKilledProcessed = true;
		EEKilledZen(killer);
	}
	
	override void EEKilled(Object killer)
	{
		super.EEKilled(killer);
		
		if (killer != null && killer != this)
			ZenHandleEntityKilledInternal(killer);
	}
	
	// Override this to get a more consistent & robust EEKilled killer hook
	void EEKilledZen(notnull Object killer)
	{
	}
}