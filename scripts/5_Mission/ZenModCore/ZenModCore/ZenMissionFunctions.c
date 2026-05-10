class ZenMissionFunctions: ZenFunctions
{
	static void FreezePlayerControls(bool showMouse = true, bool hideHud = true)
	{
		if (!g_Game)
			return;
			
		MissionGameplay mission = MissionGameplay.Cast(g_Game.GetMission());
		if (!mission)
			return;
		
		mission.AddActiveInputExcludes({"menu"});
		mission.AddActiveInputRestriction(EInputRestrictors.INVENTORY);
		GetUApi().SupressNextFrame(true);
		
		if (showMouse)
		{
			g_Game.GetUIManager().ShowCursor(true);
		}
		
		if (!hideHud)
			return;

		mission.GetHud().ShowHud(false);
		mission.GetHud().ShowQuickBar(false);
	}
	
	static void UnfreezePlayerControls(bool hideMouse = true, bool showHud = true)
	{
		if (!g_Game)
			return;

		MissionGameplay mission = MissionGameplay.Cast(g_Game.GetMission());
		if (!mission)
			return;

		mission.RemoveActiveInputExcludes({"menu"}, true);
		mission.RemoveActiveInputRestriction(EInputRestrictors.INVENTORY);

		if (!showHud)
			return;
		
		mission.GetHud().ShowHud(true);
		mission.GetHud().ShowQuickBar(true);
	}
}