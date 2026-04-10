modded class ExplosivesBase
{
	private PlayerBase m_ZenArmedPlayer;

	override void OnPlacementComplete(Man player, vector position = "0 0 0", vector orientation = "0 0 0")
	{
		super.OnPlacementComplete(player, position, orientation);

		if (g_Game.IsDedicatedServer())
		{
			m_ZenArmedPlayer = PlayerBase.Cast(player);
		}
	}

	PlayerBase Zen_GetArmedPlayer()
	{
		return m_ZenArmedPlayer;
	}
}