modded class Hologram
{
	override void EvaluateCollision(ItemBase action_item = null)
	{
		ZenKitBase kit = ZenKitBase.Cast(action_item);

		if (kit && kit.GetZenHoloShouldIgnoreCollision())
		{
			SetIsColliding(false);
			return;
		}

		super.EvaluateCollision(action_item);
	}
	
	override void SetProjectionPosition(vector position)
	{
		#ifndef ZenHologramControls
		ZenKitBase zenkit = ZenKitBase.Cast(m_Parent);
		if (zenkit != NULL)
		{
			m_Projection.SetPosition(position + zenkit.GetZenDeployPositionOffset());
			return;
		}
		#endif

		super.SetProjectionPosition(position);
	}

	override void SetProjectionOrientation(vector orientation)
	{
		ZenKitBase zenkit = ZenKitBase.Cast(m_Parent);
		if (zenkit != NULL)
		{
			m_Projection.SetOrientation(orientation + zenkit.GetZenDeployOrientationOffset());
			return;	
		}

		super.SetProjectionOrientation(orientation);
	}

	override void RefreshVisual()
    {
        super.RefreshVisual();

        if (!m_Parent || !m_Player)
            return;

        if (m_Parent.ShouldZenHologram() && !m_Parent.IsZenHologrammed() && m_Player.IsPlacingLocal())
        {
            m_Parent.SetZenHologrammed(true);
        }
    }

	void ~Hologram()
    {
        if (!m_Parent || !m_Player)
            return;

        if (m_Parent.IsZenHologrammed())
        {
            m_Parent.SetZenHologrammed(false);
        }
    }
}