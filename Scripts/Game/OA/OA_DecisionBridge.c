// OA_DecisionBridge.c — REST API bridge: sends situation reports to OpenArma backend,
// receives pending orders + config via single POST /open/command endpoint.
// Uses async POST to avoid blocking the game thread.
// API Key passed via query parameter (Enfusion has no custom header support).

class OA_DecisionBridge
{
	protected RestContext m_RestCtx;
	protected string m_sApiKey;
	protected string m_sConversationId;
	protected bool m_bConnected;
	protected int m_iConsecutiveFailures;
	protected bool m_bRequestPending;

	protected ref OA_Main m_CallbackTarget;
	protected bool m_bIsInitRequest;

	protected ref RestCallback m_ActiveCb;

	void OA_DecisionBridge()
	{
		m_sApiKey = "";
		m_sConversationId = "";
		m_bConnected = false;
		m_iConsecutiveFailures = 0;
		m_bRequestPending = false;
		m_bIsInitRequest = false;
	}

	bool Init(string apiUrl, string apiKey)
	{
		if (apiUrl.IsEmpty() || apiKey.IsEmpty())
			return false;

		m_sApiKey = apiKey;

		string baseUrl = apiUrl;
		if (baseUrl.EndsWith("/"))
			baseUrl = baseUrl.Substring(0, baseUrl.Length() - 1);

		OA_Log.Log(string.Format("[OA] Creating REST context for: %1", baseUrl));
		m_RestCtx = GetGame().GetRestApi().GetContext(baseUrl);
		if (!m_RestCtx)
		{
			OA_Log.Log("[OA] Failed to create REST context");
			return false;
		}

		return true;
	}

	protected string BuildPath()
	{
		return string.Format("/api/v1/open/command?api_key=%1", m_sApiKey);
	}

	// --- Init request (login: empty situation to validate key + get config) ---

	void SendInit(OA_Main callbackTarget)
	{
		if (!m_RestCtx || m_bRequestPending)
			return;

		m_CallbackTarget = callbackTarget;
		m_bIsInitRequest = true;
		m_bRequestPending = true;

		string payload = "{\"request_id\":0,\"timestamp\":0,\"priority\":\"normal\",\"game_state\":{},\"groups\":[]}";
		string path = BuildPath();
		OA_Log.Log(string.Format("[OA] Init POST %1", path));

		m_ActiveCb = new RestCallback();
		m_ActiveCb.SetOnSuccess(OnRequestSuccess);
		m_ActiveCb.SetOnError(OnRequestError);

		m_RestCtx.POST(m_ActiveCb, path, payload);
	}

	// --- Situation heartbeat ---

	void SendSituation(string situationJson, OA_Main callbackTarget)
	{
		if (!m_RestCtx || m_bRequestPending)
			return;

		m_CallbackTarget = callbackTarget;
		m_bIsInitRequest = false;
		m_bRequestPending = true;

		string path = BuildPath();

		m_ActiveCb = new RestCallback();
		m_ActiveCb.SetOnSuccess(OnRequestSuccess);
		m_ActiveCb.SetOnError(OnRequestError);

		m_RestCtx.POST(m_ActiveCb, path, situationJson);
	}

	// --- Unified callbacks ---

	protected void OnRequestSuccess(RestCallback cb)
	{
		m_bRequestPending = false;
		m_iConsecutiveFailures = 0;
		m_bConnected = true;

		OA_CommandResponse resp = null;
		string data = cb.GetData();
		if (!data.IsEmpty())
		{
			resp = new OA_CommandResponse();
			resp.ExpandFromRAW(data);

			if (resp.conversation_id != "")
				m_sConversationId = resp.conversation_id;
		}

		if (m_bIsInitRequest)
		{
			OA_Log.Log("[OA] Init response received");
			if (m_CallbackTarget)
				m_CallbackTarget.OnLoginResult(true, resp);
		}
		else
		{
			string respStatus = "unknown";
			int orderCount = 0;
			if (resp)
			{
				respStatus = resp.status;
				if (resp.pending_orders && resp.pending_orders.orders)
					orderCount = resp.pending_orders.orders.Count();
			}
			OA_Log.Log(string.Format("[OA] Heartbeat OK: status=%1, orders=%2", respStatus, orderCount));
			if (m_CallbackTarget)
				m_CallbackTarget.OnHeartbeatResult(true, resp);
		}
	}

	protected void OnRequestError(RestCallback cb)
	{
		m_bRequestPending = false;
		m_iConsecutiveFailures++;

		if (m_iConsecutiveFailures >= 3)
			m_bConnected = false;

		OA_Log.Log(string.Format("[OA] REST error: %1 (HTTP %2, failures: %3)",
			cb.GetRestResult(), cb.GetHttpCode(), m_iConsecutiveFailures));

		if (m_bIsInitRequest)
		{
			if (m_CallbackTarget)
				m_CallbackTarget.OnLoginResult(false, null);
		}
		else
		{
			if (m_CallbackTarget)
				m_CallbackTarget.OnHeartbeatResult(false, null);
		}
	}

	// --- Accessors ---

	bool IsConnected()       { return m_bConnected; }
	bool IsRequestPending()  { return m_bRequestPending; }
	string GetConversationId() { return m_sConversationId; }
}
