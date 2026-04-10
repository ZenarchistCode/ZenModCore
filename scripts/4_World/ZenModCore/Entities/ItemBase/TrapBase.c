modded class TrapBase 
{
	private PlayerBase m_ZenTrapperPlayer;
	
	PlayerBase Zen_GetPlayerTrapper()
	{
		return m_ZenTrapperPlayer;
	}
	
	override void StartActivate(PlayerBase player)
	{
		super.StartActivate(player);

		if (g_Game.IsDedicatedServer())
		{
			m_ZenTrapperPlayer = player;
		}
	}
}