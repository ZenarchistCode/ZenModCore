// This class code only executes on the server.
modded class MissionServer 
{
	private static int ZEN_LAST_DB_SAVE;
	
	void ~MissionServer()
	{
		OnZenMissionFinish();
	}
	
	void MissionServer()
	{
		GetZenObjectHookSpawner().Init();
		ZEN_LAST_DB_SAVE = g_Game.GetTime();
	}
	
	override void OnMissionStart()
	{
		super.OnMissionStart();
		
		ZMPrint("[ZenModCore] OnMissionStart.");
	}
	
	override void OnZenInit()
	{
		super.OnZenInit();
		
		ZMPrint("Server session count: " + ZenGameFunctions.ServerSessionCount);
		
		string mapName = g_Game.GetWorldName();
		
		// g_Game is not available when config first loads - so set it here if it hasn't been set already.
		if (GetZenCoreConfig().ZenCore_DiscordConfig.ServerName == "")
		{
			GetZenCoreConfig().ZenCore_DiscordConfig.ServerName = mapName;
			GetZenCoreConfig().ZenCore_DiscordConfig.ServerName.ToUpper();
			GetZenCoreConfig().Save();
		}
		
		if (GetZenCoreConfig().MapIzurviveURL == "")
		{
			if (mapName == "chernarusplus")
				GetZenCoreConfig().MapIzurviveURL = "https://www.izurvive.com/chernarusplus/";
			else if (mapName == "enoch")
				GetZenCoreConfig().MapIzurviveURL = "https://www.izurvive.com/livonia/";
			else 
				GetZenCoreConfig().MapIzurviveURL = "https://www.izurvive.com/" + mapName + "/";
			
			GetZenCoreConfig().Save();
		}
		
		GetZenObjectHookSpawner().SpawnOrDump();
	}
	
	// All config which is required ONLY on the server should be init here.
	override void LoadZenConfig()
	{
		super.LoadZenConfig();
	}
	
	override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity) 
	{
		super.InvokeOnConnect(player, identity);
		
		ZenInvokeOnConnect(player, identity);
	}
	
	void ZenInvokeOnConnect(PlayerBase player, PlayerIdentity identity)
	{
		GetZenConfigRegister().SyncToClient(identity);
		
		if (GetZenCoreConfig().IsAdmin(identity))
		{
			player.SetZenAdmin(true);
			GetRPCManager().SendRPC("ZenMod_RPC", "RPC_ReceiveZenCoreAdminStatusOnClient", new Param1<bool>(true), true, identity);
		}
		
		if (!GetZenCoreConfig().ZenCore_DiscordConfig.EnableDiscord)
			return;
		
		if (!GetZenCoreConfig().ZenCore_DiscordConfig.PlayerWatchlist || GetZenCoreConfig().ZenCore_DiscordConfig.PlayerWatchlist.Count() == 0)
			return;
		
		string playerID = identity.GetId();
		string playerName = identity.GetName();
		string watchlistReason;
		
		if (GetZenCoreConfig().ZenCore_DiscordConfig.PlayerWatchlist.Find(playerID, watchlistReason))
		{
			if (player.GetCachedName() != playerName)
			{
				playerName = playerName + " / " + player.GetCachedName();
			}
			
			ZenDiscordMessage playerWatchlistMsg = new ZenDiscordMessage("Watchlist");
			playerWatchlistMsg.SetTitle("#STR_ZenWatchlistPlayerConnected " + player.GetCachedName() + " - " + GetZenCoreConfig().ZenCore_DiscordConfig.ServerName);
			playerWatchlistMsg.SetMessage(watchlistReason + "\n\n" + ZenGameFunctions.GetMapLinkPosition(player.GetPosition()) + "\n\n" + identity.GetId() + "\n\n[" + identity.GetPlainId() + "](http://steamcommunity.com/profiles/" + identity.GetPlainId() + ")");
			playerWatchlistMsg.SetColor(255, 255, 0);

			foreach (string s : GetZenCoreConfig().ZenCore_DiscordConfig.AdminWebhooks)
			{
				playerWatchlistMsg.AddWebhook(s);
			}
				
			GetZenDiscordAPI().SendMessage(playerWatchlistMsg);
		}
	}

	override void OnEvent(EventType eventTypeId, Param params)
	{
		if (eventTypeId == ChatMessageEventTypeID)
		{
			// Log chat.
			ChatMessageEventParams chat;
			if (Class.CastTo(chat, params))
			{
				string name = chat.param2; // "from" string
				string msg = chat.param3;  // text
				string playerID = "UNKNOWNUID";
				
				PlayerBase pb = ZenFunctions.GetIdentityFromChatName(name);
				if (pb != null)
				{
					playerID = pb.GetIdentity().GetId();
				}
				
				ZMLog("Chat", "chat", "[" + playerID + "] " + name + ": " + msg);
			}
		}

		super.OnEvent(eventTypeId, params);
	}
	
	// If a player just logged out and they were the LAST player to logout, save all DBs.
	// This is a robust way to save databases at server restarts if server runs CFTools and kicks
	// players before a restart, which most high-pop DayZ servers typically do to prevent item duping.
	// It can also trigger if the server is simply quiet and the last player logs out - no harm there either.
	override void PlayerDisconnected(PlayerBase player, PlayerIdentity identity, string uid)
	{
		super.PlayerDisconnected(player, identity, uid);
		
		array<Man> players = new array<Man>();
		g_Game.GetPlayers(players);
		
		if (players.Count() > 1)
			return;
		
		if (g_Game.GetTime() - ZEN_LAST_DB_SAVE > 30000)
		{
			return;
		}
		
		OnZenShutdownOrPlayersLoggedOut();
		ZEN_LAST_DB_SAVE = g_Game.GetTime();
	}
	
	void OnZenShutdownOrPlayersLoggedOut()
	{
		ZMPrint("Final active player disconnected: saving all databases.");
		GetZenConfigRegister().SaveAllConfigs();
	}

	override void ZenDeferredInit()
	{
		super.ZenDeferredInit();

		if (!GetZenCoreConfig().ZenCore_GeneralConfig.DeleteObjectsAt000)
			return;

		// Delete any ghost items spawned at 0 0 0 (vector.Zero) and log them to debug it
		// this can happen on some mods where the hologram gets fucked up during placement
		// I found hundreds of objects placed at this location on my server once clogging up server resources.
		// At least this way an error log will be generated to make the server admin aware it is happening
		// and find a way to fix it properly instead of letting them accumulate.
		array<Object> nearest_objects = new array<Object>;
        g_Game.GetObjectsAtPosition3D("0 0 0", 1, nearest_objects, NULL);
		foreach (Object obj : nearest_objects)
		{
			Error("Deleted " + obj.GetType() + " @ 0 0 0 world coordinates.");
			g_Game.ObjectDelete(obj);
		}
	}
}