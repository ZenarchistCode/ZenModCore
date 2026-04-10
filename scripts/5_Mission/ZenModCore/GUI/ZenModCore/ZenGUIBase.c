class ZenGUIBase extends ZenGUIBase_Game
{
    override Widget Init()
    {
		string filePath = Zen_GetLayoutFile();
		
		if (filePath == ZEN_LAYOUT_NOT_SET)
		{
			Error(filePath);
			return null;
		}

		layoutRoot = g_Game.GetWorkspace().CreateWidgets(filePath);
		Zen_LoadWidgets();
        return layoutRoot;
    }
	
	override void OnShow()
	{
		super.OnShow();

		if (g_Game && Zen_LockMouseAndKeyboardOnShow())
		{
			g_Game.GetInput().ChangeGameFocus(1);
			SetFocus(layoutRoot);
			
			PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
			if (player) 
				player.GetInputController().SetDisabled(true);
			
			g_Game.GetUIManager().ShowCursor(true);
		}
	}

	override void OnHide()
	{
		super.OnHide();

		if (g_Game && Zen_LockMouseAndKeyboardOnShow())
		{
			g_Game.GetInput().ResetGameFocus();
			g_Game.GetUIManager().ShowCursor(false);
	
			PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
			if (player) 
				player.GetInputController().SetDisabled(false);
		}
	}
}
