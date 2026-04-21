class ZenAdminCommandHandler
{
	void ZenAdminCommandHandler()
	{
		ZMPrint("[ZenAdminCommandHandler] Init RPCs.");
		GetRPCManager().AddRPC(ZEN_RPC, "RPC_ReceiveZenAdminCommandChat", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC(ZEN_RPC, "RPC_ReceiveZenOpenURL", this, SingleplayerExecutionType.Client);
	}
	
	void ~ZenAdminCommandHandler()
	{
		ZMPrint("[ZenAdminCommandHandler] Destroyed.");
	}
	
	// COMMAND HANDLERS

	bool HandleRawChatCommand(PlayerBase player, string msg)
	{
		if (g_Game.IsClient())
		{
			return HandleRawChatCommandClient(player, msg);
		}
		
		if (!GetZenCoreConfig().ZenCore_AdminConfig.EnableCommands)
			return false;
		
		if (msg == "")
			return false;
		
		msg.ToLower();

		string id = player.GetIdentity().GetId();
		array<string> t = new array<string>;
		msg.Split(" ", t);
		
		string cmd = t.Get(0);
		cmd = cmd.Substring(1, cmd.Length() - 1);
		t.Remove(0);
		
		ZMPrint("[ZenAdminCommandHandler] Received command: '" + msg + "' from " + id);
		
		if (!player)
		{
			return HandleCivilianCommand(null, id, cmd, t);
		}

		if (player.IsZenAdmin() && HandleAdminCommand(player, id, cmd, t))
		{
			ZMLog("Admin", "commands", id + " executed admin command: " + msg);
			ZenFunctions.SendPlayerMessage(player, "#STR_ZenCommandExecutedSuccessfully");
			return true;
		}
		
		if (GetZenCoreConfig().IsModerator(id) && HandleModeratorCommand(player, id, cmd, t))
		{
			ZMLog("Admin", "commands", id + " executed moderator command: " + msg);
			ZenFunctions.SendPlayerMessage(player, "#STR_ZenCommandExecutedSuccessfully");
			return true;
		}
		
		if (HandleCivilianCommand(player, id, cmd, t))
		{
			ZMLog("Admin", "commands", id + " executed civilian command: " + msg);
			return true;
		}

		return true; // return true so that chat message gets stopped from relay to other players
	}
	
	bool HandleRawChatCommandClient(PlayerBase player, string msg)
	{
		return false;
	}
	
	bool HandleCivilianCommand(PlayerBase player, string uid, string cmd, array<string> params)
	{
		if (cmd == "testcivilian")
		{
			SendMsg(player, "CIVILIAN TEST PASSED!");
			return true;
		}
		
		if (GetZenCoreConfig().ZenCore_DiscordConfig.EnableDiscord && cmd == GetZenCoreConfig().ZenCore_DiscordConfig.PingAdminCommand && GetZenCoreConfig().ZenCore_DiscordConfig.PingAdminCommand != "")
		{
			string profileName = player.GetCachedName();
            string steamid = player.GetIdentity().GetPlainId();
            string from = "[" + profileName + "](http://steamcommunity.com/profiles/" + steamid + ")\n" + uid + "\n" + steamid;
            string status = ZenGameFunctions.GetMapLinkPosition(player.GetPosition()) + "\nHP=" + (player.GetHealth01() * 100) + "%";

            string title = cmd;
            title.ToUpper();
			
			string text = "";
			foreach (string s : params)
			{
				text = text + s;
			}

            // Build discord notification string
            ZenDiscordMessage msg = new ZenDiscordMessage(title);
		    msg.SetTitle(title);
		    msg.SetMessage(text + "\n\n" + from + "\n\n" + status);
		    msg.SetColor(255, 160, 0);
		    msg.AddWebhooks(GetZenCoreConfig().ZenCore_DiscordConfig.AdminWebhooks);
		    GetZenDiscordAPI().SendMessage(msg);

            // Notify player
			SendMsg(player, "#STR_ZenDiscordMessageReceived");
		}
		
		string cmdURL;
		if (GetZenCoreConfig().ZenCore_AdminConfig.CommandURLs.Find(cmd, cmdURL))
		{
			if (player)
			{
				GetRPCManager().SendRPC(ZEN_RPC, "RPC_ReceiveZenOpenURL", new Param1<string>(cmdURL), true, player.GetIdentity());
			}
			else 
			if (g_Game.IsClient())
			{
				g_Game.OpenURL(cmdURL);
			}
			
			return true;
		}

		return false;
	}
	
	ZenConfigBase GetConfigByName(string findCfgName)
	{
		foreach (typename typeNameKey, ZenConfigBase cfgBase : GetZenConfigRegister().GetConfigs())
		{
			if (!cfgBase)
				continue;
			
			string cfgName = cfgBase.GetFileName();
			cfgName.ToLower();
			cfgName.Replace(".json", "");
			
			if (cfgName == findCfgName)
			{
				return cfgBase;
			}
		}
		
		return null;
	}
	
	bool HandleAdminCommand(PlayerBase player, string uid, string cmd, array<string> params)
	{
		// Reload ALL configs
		if (cmd == "reload" && params.Count() == 0)
		{
			int fileCount = 0;
			int syncCount = 0;
			
			foreach (typename typeNameKey, ZenConfigBase cfgBase : GetZenConfigRegister().GetConfigs())
			{
				if (!cfgBase)
					continue;
				
				cfgBase.Load();
				
				if (cfgBase.ShouldSyncToClient())
				{
					cfgBase.SyncToClient();
					syncCount++;
				}
				
				fileCount++;
			}
			
			SendMsg(player, "Reloaded " + fileCount + " and resync'ed " + syncCount + " config files successfully");
			return true;
		}
		
		// Reload specific config 
        if (cmd == "reload" && params.Count() > 0)
        {
            bool reload = false;
			string findCfgName = params.Get(0);
			findCfgName.Replace(".json", "");
			
			ZenConfigBase cfg = GetConfigByName(findCfgName);
			if (!cfg)
				cfg = GetConfigByName(findCfgName + "config");
						
			if (cfg)
			{
				cfg.Load();
				if (cfg.ShouldSyncToClient())
				{
					cfg.SyncToClient();
				}
				
				SendMsg(player, "Reloaded " + cfg.GetFileName() + " config successfully");
			}
            else
            {
				SendMsg(player, "Failed! Couldn't find " + params.Get(0) + " config.");
            }

            return true;
        }
		
		if (params.Count() >= 2 && cmd == "teleport")
		{
			float x = params.Get(0).ToFloat();
			float z = params.Get(1).ToFloat();
			float y = g_Game.SurfaceY(x, z);
			DeveloperTeleport.SetPlayerPosition(player, Vector(x, y, z));
			
			ZenFunctions.SendPlayerMessage(player, "Teleported you to " + x + " " + y);
			
			return true;
		}

		return false;
	}
	
	bool HandleModeratorCommand(PlayerBase player, string uid, string cmd, array<string> params)
	{		
		if (cmd == "test")
		{
			ZenFunctions.DebugMessage("MODERATOR TEST PASSED!");
			return true;
		}

		return false;
	}
	
	void SendMsg(PlayerBase player, string msg)
	{
		ZenFunctions.SendPlayerMessage(player, "[CommandHandler] " + msg);
	}
	
	// RPCs
	
	// Client -> server :: receive potential admin command
	void RPC_ReceiveZenAdminCommandChat(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Server && g_Game.IsDedicatedServer())
        {
            Param1<string> data;
            if (!ctx.Read(data))
            {
                Error("Error receiving data - RPC_ReceiveZenAdminCommandChat");
                return;
            }

            if (sender && data.param1)
            {
				if (!HandleRawChatCommand(PlayerBase.Cast(sender.GetPlayer()), data.param1))
				{
					GetRPCManager().SendRPC(ZEN_RPC, "RPC_ReceiveZenAdminCommandFailed", new Param1<string>(data.param1), true, sender);
				}
            }
        }
    }
	
	// Client -> server :: receive open URL request
	void RPC_ReceiveZenOpenURL(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
		Param1<string> data;
		if (!ctx.Read(data))
		{
		    Error("Error receiving data - RPC_ReceiveZenOpenURL");
		    return;
		}
		
		if (data.param1 && g_Game)
		{
			g_Game.OpenURL(data.param1);
		}
    }
}