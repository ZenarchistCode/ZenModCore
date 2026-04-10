modded class Grenade_Base
{
	private PlayerBase m_ZenUnpinPlayer = NULL;

	PlayerBase Zen_GetUnpinPlayer()
	{
		return m_ZenUnpinPlayer;
	}

	override void Unpin()
	{
		super.Unpin();

		if (g_Game.IsDedicatedServer() && GetHierarchyRootPlayer() != NULL)
		{
			PlayerBase pb = PlayerBase.Cast(GetHierarchyRootPlayer());
			if (pb)
			{
				m_ZenUnpinPlayer = pb;
			}
		}
		else 
		{
			m_ZenUnpinPlayer = NULL;
		}
	}
}
