class ZenNotifications
{
	static void Notify(PlayerBase player, string title, string message, string icon = "", float time = 15)
	{
#ifdef SERVER
		if (player && player.GetIdentity())
		{
			NotificationSystem.SendNotificationToPlayerExtended(player, time, title, message, icon);
		}
#else 
		NotifyLocal(title, message, icon, time);
#endif
	}

	static void NotifyLocal(string title, string message, string icon = "", float p_time = 5)
	{
		if (g_Game.IsClient() && g_Game.GetPlayer())
		{
			float time = message.Length() / 10;
			if (time < p_time)
				time = p_time;

			NotificationSystem.AddNotificationExtended(time, title, message, icon);
		}
	}
}