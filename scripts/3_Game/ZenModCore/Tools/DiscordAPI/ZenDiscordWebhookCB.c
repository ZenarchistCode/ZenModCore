class ZenDiscordWebhookCB extends RestCallback
{
	protected ZenDiscordAPI m_API;     // soft ref is fine (API is global anyway)
	protected string m_Webhook;
	protected string m_JSON;
	protected Class m_Context;         // optional: caller context (e.g. the raid alarm station)

	void ZenDiscordWebhookCB(ZenDiscordAPI api, string webhook, string json, Class context = null)
	{
		m_API     = api;
		m_Webhook = webhook;
		m_JSON    = json;
		m_Context = context;
	}

	string GetWebhook()  { return m_Webhook; }
	string GetJSON()     { return m_JSON; }
	Class  GetContext()  { return m_Context; }

	override void OnSuccess(string data, int dataSize)
	{
		if (m_API)
			m_API._OnRequestSuccess(this, data, dataSize);
	}

	override void OnError(int errorCode)
	{
		if (m_API)
			m_API._OnRequestError(this, errorCode);
	}

	override void OnTimeout()
	{
		if (m_API)
			m_API._OnRequestTimeout(this);
	}
}
