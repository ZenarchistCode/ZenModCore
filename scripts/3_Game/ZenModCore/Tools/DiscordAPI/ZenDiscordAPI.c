class ZenDiscordAPI
{
	static const string DISCORD_API = "/api/webhooks/";

	// Keep callbacks alive until they complete (prevents GC while request is in-flight)
	protected ref array<ref ZenDiscordWebhookCB> m_Pending = new array<ref ZenDiscordWebhookCB>();

	// Events other mods can subscribe to
	protected ref ScriptInvoker m_OnWebhookSuccess = new ScriptInvoker(); // (ZenDiscordWebhookCB cb, string data, int dataSize)
	protected ref ScriptInvoker m_OnWebhookError   = new ScriptInvoker(); // (ZenDiscordWebhookCB cb, int errorCode)
	protected ref ScriptInvoker m_OnWebhookTimeout = new ScriptInvoker(); // (ZenDiscordWebhookCB cb)

	ScriptInvoker GetOnWebhookSuccess() { return m_OnWebhookSuccess; }
	ScriptInvoker GetOnWebhookError()   { return m_OnWebhookError; }
	ScriptInvoker GetOnWebhookTimeout() { return m_OnWebhookTimeout; }

	void ZenDiscordAPI()
	{
		if (!GetRestApi())
			CreateRestApi();
	}

	protected void _Release(ZenDiscordWebhookCB cb)
	{
		int idx = m_Pending.Find(cb);
		if (idx > -1)
			m_Pending.Remove(idx);
	}

	void _OnRequestSuccess(ZenDiscordWebhookCB cb, string data, int dataSize)
	{
		m_OnWebhookSuccess.Invoke(cb, data, dataSize);
		_Release(cb);
	}

	// Called by callback
	void _OnRequestError(ZenDiscordWebhookCB cb, int errorCode)
	{
		// IMPORTANT: treat 8 (EREST_ERROR_APPERROR) as "success" for Discord webhook POSTs
		// because Discord often returns 204 No Content and DayZ REST can surface that as Error Code 8
		if (errorCode == ERestResultState.EREST_ERROR_APPERROR)
		{
			m_OnWebhookSuccess.Invoke(cb, "", 0);
		}
		else
		{
			m_OnWebhookError.Invoke(cb, errorCode);
		}

		_Release(cb);
	}

	// Called by callback
	void _OnRequestTimeout(ZenDiscordWebhookCB cb)
	{
		m_OnWebhookTimeout.Invoke(cb);
		_Release(cb);
	}

	void SendMessage(notnull ZenDiscordMessage msg, Class context = null)
	{
		if (!context)
			context = this;
		
		ZMPrint("[ZenDiscordAPI] Sending webhook - callback context=" + context.ClassName());
		
		string json = msg.GetJSON();

		for (int i = 0; i < msg.GetWebhooks().Count(); i++)
		{
			string full = msg.GetWebhooks().Get(i);

			int idx = full.IndexOf(DISCORD_API);
			if (idx < 0)
			{
				Print("[ZenDiscordAPI] Invalid Discord webhook URL: " + full);
				continue;
			}

			int baseEnd   = idx + DISCORD_API.Length();
			string baseUrl = full.Substring(0, baseEnd);
			string route   = full.Substring(baseEnd, full.Length() - baseEnd);

			// OPTIONAL improvement:
			// Discord returns 204 by default; adding wait=true can return 200 + JSON.
			// This *may* reduce "error 8 on success" noise depending on RestApi behavior.
			// route = route + "?wait=true";

			RestContext ctx = GetRestApi().GetRestContext(baseUrl);
			ctx.SetHeader("application/json");

			ref ZenDiscordWebhookCB cb = new ZenDiscordWebhookCB(this, full, json, context);
			m_Pending.Insert(cb);

			ctx.POST(cb, route, json);
		}

		delete msg;
	}
}

ref ZenDiscordAPI m_ZenDiscordAPI;

static ZenDiscordAPI GetZenDiscordAPI()
{
	if (!m_ZenDiscordAPI)
	{
		m_ZenDiscordAPI = new ZenDiscordAPI;
	}

	return m_ZenDiscordAPI;
}