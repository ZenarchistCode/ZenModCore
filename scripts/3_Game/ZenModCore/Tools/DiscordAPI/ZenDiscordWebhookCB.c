class ZenDiscordWebhookCB extends RestCallback
{
	protected ZenDiscordAPI m_API;
	protected string m_Webhook;
	protected string m_JSON;
	protected string m_ContextType;
	protected string m_ContextId;

	void ZenDiscordWebhookCB(ZenDiscordAPI api, string webhook, string json, string contextType = "", string contextId = "")
	{
		m_API = api;
		m_Webhook = webhook;
		m_JSON = json;
		m_ContextType = contextType;
		m_ContextId = contextId;
	}

	string GetWebhook()
	{
		return m_Webhook;
	}

	string GetJSON()
	{
		return m_JSON;
	}

	string GetContextType()
	{
		return m_ContextType;
	}

	string GetContextId()
	{
		return m_ContextId;
	}

	override void OnSuccess(string data, int dataSize)
	{
		if (m_API)
		{
			m_API._OnRequestSuccess(this, data, dataSize);
		}
	}

	override void OnError(int errorCode)
	{
		if (m_API)
		{
			m_API._OnRequestError(this, errorCode);
		}
	}

	override void OnTimeout()
	{
		if (m_API)
		{
			m_API._OnRequestTimeout(this);
		}
	}
}
