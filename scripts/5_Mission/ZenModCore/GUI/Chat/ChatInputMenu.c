modded class ChatInputMenu
{
	protected static string m_LastZenChatText = "";
	
	override void Update(float timeslice)
	{
		super.Update(timeslice);
	
		if (KeyState(KeyCode.KC_UP) && m_LastZenChatText != "")
		{
			m_edit_box.SetText(m_LastZenChatText);
			m_edit_box.Update();
		}
	}
	
	override bool OnChange(Widget w, int x, int y, bool finished)
	{
		if (finished)
		{
			string text = m_edit_box.GetText();
			m_LastZenChatText = text;

			// Admin messages
			if (text.IndexOf(GetZenCoreConfig().ZenCore_AdminConfig.CommandPrefix) == 0)
			{
				MissionBase mb = MissionBase.Cast(g_Game.GetMission());
				if (mb && mb.GetZenCommandHandler())
				{
					if (!mb.GetZenCommandHandler().HandleRawChatCommand(null, text.Trim()))
					{
						GetRPCManager().SendRPC(ZEN_RPC, "RPC_ReceiveZenAdminCommandChat", new Param1<string>(text.Trim()), true, NULL);
					}
				}
				
				m_close_timer.Run(0.1, this, "Close");
				GetUApi().GetInputByID(UAPersonView).Supress();	

				// Don't send on to vanilla handling - this is to be handled as a potential admin command
				return true;
			}
		}

		return super.OnChange(w, x, y, finished);
	}
}