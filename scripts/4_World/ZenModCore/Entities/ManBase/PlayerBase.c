modded class PlayerBase
{
	protected bool m_ZenCore_IsAdmin;
	
	void PlayerBase()
	{
		m_ZenCore_IsAdmin = false;
	}
	
	void SetZenAdmin(bool state)
	{
		m_ZenCore_IsAdmin = state;
		ZMPrint("[ZenCore Admin Status=" + state);
	}
	
	bool IsZenAdmin()
	{
		return m_ZenCore_IsAdmin;
	}
	
	override void OnPlayerLoaded()
	{
		super.OnPlayerLoaded();

		if (g_Game.IsClient() && IsControlledPlayer())
		{
			Zen_OnPlayerLoadedClientPlayer();
		}
	}
	
	override void OnDisconnect()
	{
		super.OnDisconnect();
		
		if (g_Game.IsClient() && IsControlledPlayer())
		{
			Zen_OnDisconnectClientPlayer();
		}
	}
	
	void Zen_OnPlayerLoadedClientPlayer() 
	{
		GetZenConfigRegister().RequestAllOutdatedConfigs();
	}
	
	void Zen_OnDisconnectClientPlayer() 
	{
	}
	
	override void EEKilled(Object killer)
	{
		super.EEKilled(killer);

		if (!killer)
		{
			//ZMPrint("Killfeed: No killer.");
			return;
		}

		if (killer == this)
		{
			//ZMPrint("Killfeed: Killer = self.");
			return;
		}

		ZenCoreConfig zenCfg = GetZenCoreConfig();
		if (!zenCfg)
			return;
		
		if (!zenCfg.ZenCore_DiscordConfig.EnableDiscord)
		{
			//ZMPrint("Killfeed: Discord disabled.");
			return;
		}
		
		if (!zenCfg.ZenCore_DiscordConfig.KillfeedConfig.EnableKillfeed)
		{
			//ZMPrint("Killfeed: Killfeed disabled.");
			return;
		}
		
		if (!zenCfg.ZenCore_DiscordConfig.KillfeedConfig.KillfeedWebhooks || zenCfg.ZenCore_DiscordConfig.KillfeedConfig.KillfeedWebhooks.Count() == 0)
		{
			//ZMPrint("Killfeed: No valid webhooks.");
			return;
		}

		if (!GetIdentity() || Zen_IsAI() || GetType().Contains("_Ghost"))
		{
			//ZMPrint("Killfeed: No identity or is AI=" + Zen_IsAI());
			return;
		}

		EntityAI killerAI = EntityAI.Cast(killer);
		if (!killerAI)
		{
			//ZMPrint("Killfeed: No killer entity.");
			return;
		}

		// Check player melee kill & guns through HierarchyRootPlayer
		PlayerBase playerKiller = PlayerBase.Cast(killerAI);
		if (!playerKiller)
			playerKiller = PlayerBase.Cast(killerAI.GetHierarchyRootPlayer());

		if (!playerKiller)
		{
			// Check for grenades 
			Grenade_Base grenade = Grenade_Base.Cast(killerAI);
			if (!playerKiller && grenade != null)
				playerKiller = PlayerBase.Cast(grenade.Zen_GetUnpinPlayer());
			
			// Check for explosives
			ExplosivesBase explosive = ExplosivesBase.Cast(killerAI);
			if (!playerKiller && explosive != null)
				playerKiller = PlayerBase.Cast(explosive.Zen_GetArmedPlayer());

			// Check for traps 
			if (!playerKiller)
			{
				TrapBase trap = TrapBase.Cast(killerAI);
				if (trap != null)
					playerKiller = PlayerBase.Cast(trap.Zen_GetPlayerTrapper());
			}

			// Check for vehicles
			if (!playerKiller)
			{
				CarScript vehicle = CarScript.Cast(killerAI);
				if (vehicle != null)
				{
					for (int index = 0; index < vehicle.CrewSize(); index++)
					{
						if (vehicle.CrewMember(index) != null && vehicle.GetSeatAnimationType(index) == DayZPlayerConstants.VEHICLESEAT_DRIVER)
							playerKiller = PlayerBase.Cast(vehicle.CrewMember(index));
					}
				}
			}

			//! TODO: ChemGas and projectile explosions
		}

		if (!playerKiller)
		{
			//ZMPrint("Killfeed: No hierarchy killer identity.");
			return;
		}

		if (playerKiller.GetType().Contains("_Ghost"))
			return;
		
		if (playerKiller.Zen_IsAI() && !zenCfg.ZenCore_DiscordConfig.KillfeedConfig.DisplayKillsByAI)
		{
			//ZMPrint("Killfeed: AI kills suppressed.");
			return;
		}
		
		if (!playerKiller.Zen_IsAI() && !playerKiller.GetIdentity())
		{
			//ZMPrint("Killfeed: Killer has no identity.");
			return;
		}

		ZenKillFeed(playerKiller, killer);
	}
	
	// Override this for things like Syberia mod to change character name
	string GetZenKillFeedName()
	{
		return GetCachedName();
	}
	
	bool Zen_IsAI()
	{
		#ifdef DZ_Expansion_AI
		return IsAI();
		#endif 
		
		return false;
	}

	// Send kill feed info
	private void ZenKillFeed(notnull PlayerBase playerKiller, notnull Object object)
	{
		//ZMPrint("Killfeed: Send to kill feed " + playerKiller.GetType() + " obj=" + object.GetType());
		
		vector myPos = GetPosition();
		vector shooterPos = playerKiller.GetPosition();
		string killerText = playerKiller.GetZenKillFeedName();
		string victimText = GetZenKillFeedName();
		string victimLocation = Widget.TranslateString("#STR_ZenVictim #STR_ZenPositionLabel: ") + ZenGameFunctions.GetMapLinkPosition(myPos);
		string shooterLocation = Widget.TranslateString("#STR_ZenKiller #STR_ZenPositionLabel: ") + ZenGameFunctions.GetMapLinkPosition(shooterPos);
		string distance = MiscGameplayFunctions.TruncateToS(vector.Distance(myPos, shooterPos));

		if (GetZenCoreConfig().ZenCore_DiscordConfig.KillfeedConfig.DisplayPlayerSteamID)
		{
			if (!playerKiller.GetIdentity())
			{
				killerText = playerKiller.GetType();
			} else
			{
				string idLink = "http://steamcommunity.com/profiles/";
				
				#ifdef GameLabs 
				idLink = "https://app.cftools.cloud/profile/";
				#endif
				
				killerText = "[" + playerKiller.GetZenKillFeedName() + "](" + idLink + playerKiller.GetIdentity().GetPlainId() + ")";
			}
			
			victimText = "[" + GetZenKillFeedName() + "](http://steamcommunity.com/profiles/" + GetIdentity().GetPlainId() + ")";
		}

		string weaponText = "";
		if (object.GetType() == playerKiller.GetType())
			weaponText = Widget.TranslateString("#STR_ZenKillfeed3");
		weaponText = object.GetDisplayName();

		int victimPlayMinutes = StatGet(AnalyticsManagerServer.STAT_PLAYTIME) / 60;
		int killerPlayerMinutes = playerKiller.StatGet(AnalyticsManagerServer.STAT_PLAYTIME) / 60;
		string victimAge = Widget.TranslateString("#STR_ZenKillfeed5: ");
		string killerAge = Widget.TranslateString("#STR_ZenKillfeed4: ");

		string victimAggressor = "";
		string killerAggressor = "";

		// My Anticombat logout mod tries to detect who shot/injured who first. If it's in the modlist, use that to list possible aggressor.
		#ifdef ZenAntiCombatLogout
		if (this.Zen_DidWeStartCombatWith(playerKiller))
		{
			victimAggressor = Widget.TranslateString(" #STR_ZenKillfeed6");
		}
		else 
		if (playerKiller.Zen_DidWeStartCombatWith(this))
		{
			killerAggressor = Widget.TranslateString(" #STR_ZenKillfeed6");
		}
		#endif

		if (victimPlayMinutes > 60)
			victimAge = victimAge + (victimPlayMinutes / 60) + Widget.TranslateString(" #STR_ZenHours") + victimAggressor;
		else
			victimAge = victimAge + (victimPlayMinutes) + Widget.TranslateString(" #STR_ZenMinutes") + victimAggressor;

		if (killerPlayerMinutes > 60)
			killerAge = killerAge + (killerPlayerMinutes / 60) + Widget.TranslateString(" #STR_ZenHours") + killerAggressor;
		else
			killerAge = killerAge + (killerPlayerMinutes) + Widget.TranslateString(" #STR_ZenMinutes") + killerAggressor;

		string discordMsg = killerText + Widget.TranslateString(" #STR_ZenKillfeed1 ");
		discordMsg = discordMsg + victimText + Widget.TranslateString(" #STR_ZenKillfeed2 ") + weaponText + " " + distance + "m ";
		if (GetZenCoreConfig().ZenCore_DiscordConfig.KillfeedConfig.DisplayKillLocation)
		{
			discordMsg = discordMsg + "\n" + victimLocation + "\n" + shooterLocation;
		}

		discordMsg = discordMsg + "\n\n" + victimAge + "\n" + killerAge + "\n" + Widget.TranslateString("#STR_ZenKiller #STR_ZenHPLabel: ") + playerKiller.GetHealth();

		if (GetZenCoreConfig().ZenCore_DiscordConfig.KillfeedConfig.DisplayPlayerSteamID)
		{
			discordMsg = discordMsg + "\n\n" + Widget.TranslateString("#STR_ZenVictim") + ": " + GetIdentity().GetId();
			if (!playerKiller.GetIdentity())
				discordMsg = discordMsg + "\n" + playerKiller.GetType();
			else
				discordMsg = discordMsg + "\n" + Widget.TranslateString("#STR_ZenKiller") + ": " + playerKiller.GetIdentity().GetId();
		}

		// Send discord webhook message
		ZenDiscordMessage msg = new ZenDiscordMessage("#STR_ZenKillfeed0");
		msg.SetTitle("#STR_ZenKillfeed0");
		msg.SetMessage(discordMsg, false);
		msg.SetColor(255, 255, 255);
		msg.AddWebhooks(GetZenCoreConfig().ZenCore_DiscordConfig.KillfeedConfig.KillfeedWebhooks);
		GetZenDiscordAPI().SendMessage(msg);
	}
	
	bool Zen_DidWeStartCombatWith(notnull PlayerBase enemy)
	{
		return false;
	}
	
	void ZenForceBreakLegs()
	{
		if (GetBrokenLegs() == eBrokenLegs.BROKEN_LEGS)
			return;

		DamageAllLegs(GetMaxHealth("RightLeg", "") + 1);
		ProcessDirectDamage(DamageType.CUSTOM, this, "", "FallDamageHealth", WorldToModel(GetPosition()), 0.01);
	}
	
	void Zen_SendMessage(string message)
	{
		#ifdef SERVER
		Param1<string> m_MessageParam = new Param1<string>("");
		if (message != "" && m_MessageParam != null && !IsPlayerDisconnected())
		{
			m_MessageParam.param1 = message;
			g_Game.RPCSingleParam(this, ERPCs.RPC_USER_ACTION_MESSAGE, m_MessageParam, true, GetIdentity());
		}
		#endif
	}
	
	// Sends a text message with X miliseconds delay
	void Zen_SendMessageDelayed(string message, float timeDelay)
	{
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Zen_SendMessage, timeDelay, false, message);
	}

	// Print a client-side white text message
	void Zen_DisplayClientMessage(string message)
	{
		if (g_Game.GetPlayer())
		{
			g_Game.GetMission().OnEvent(ChatMessageEventTypeID, new ChatMessageEventParams(CCDirect, "", message, ""));
		}
	}
}