// OA_Main.c — Singleton controller: Login → Heartbeat Loop → Logout.
// The mod is an "agent MCP" — it polls the backend, the backend drives all decisions.
// Config (running, factions, interval) comes from the backend response each heartbeat.

class OA_Main
{
	private static ref OA_Main s_Instance;

	protected ref OA_Config m_Config;
	protected ref OA_WorldObserver m_Observer;
	protected ref OA_DecisionBridge m_Bridge;
	protected ref OA_CommandExecutor m_Executor;
	protected ref OA_EventTracker m_EventTracker;

	protected bool m_bConnected;
	protected bool m_bRunning;
	protected bool m_bError;
	protected float m_fRetryCooldown;
	protected int m_iRetryCount;

	static const float RETRY_BASE_DELAY = 5.0;
	static const float RETRY_MAX_DELAY = 60.0;
	static const int RETRY_MAX_COUNT = 10;

	protected ref array<ref OA_SideConfig> m_aSides;

	void OA_Main()
	{
		m_Config = new OA_Config();
		m_Observer = new OA_WorldObserver();
		m_Bridge = new OA_DecisionBridge();
		m_Executor = new OA_CommandExecutor();
		m_EventTracker = new OA_EventTracker();

		m_bConnected = false;
		m_bRunning = false;
		m_bError = false;
		m_fRetryCooldown = 0;
		m_iRetryCount = 0;

		m_aSides = {};
		OA_SideConfig defaultA = new OA_SideConfig();
		defaultA.faction = "US";
		defaultA.control = "llm";
		m_aSides.Insert(defaultA);
		OA_SideConfig defaultB = new OA_SideConfig();
		defaultB.faction = "USSR";
		defaultB.control = "human";
		m_aSides.Insert(defaultB);

		s_Instance = this;
	}

	static OA_Main GetInstance()
	{
		return s_Instance;
	}

	static OA_Main CreateInstance()
	{
		s_Instance = new OA_Main();
		return s_Instance;
	}

	// --- Login / Logout ---

	void Login(string apiKey)
	{
		if (m_bConnected)
		{
			OA_Log.Log("[OA] Already connected. Logout first.");
			return;
		}

		m_Config.ApiKey = apiKey;
		m_bError = false;

		if (!m_Bridge.Init(m_Config.ApiUrl, apiKey))
		{
			OA_Log.Log("[OA] Failed to create REST context");
			m_bError = true;
			return;
		}

		m_Bridge.SendInit(this);
	}

	void OnLoginResult(bool success, OA_CommandResponse response)
	{
		if (!success)
		{
			m_bConnected = false;
			m_bError = true;
			OA_Log.Log("[OA] Login failed");
			return;
		}

		m_bConnected = true;
		m_bError = false;

		if (response)
			ApplyConfig(response);

		m_EventTracker.Init();
		m_Executor.RefreshGroupRegistry();

		int sideCount = 0;
		if (m_aSides)
			sideCount = m_aSides.Count();
		OA_Log.Log(string.Format("[OA] Logged in. Groups: %1, Running: %2, Sides: %3",
			m_Executor.GetGroupCount(), m_bRunning, sideCount));
	}

	void Logout()
	{
		m_bConnected = false;
		m_bRunning = false;
		m_bError = false;
		m_EventTracker.Cleanup();
		m_Observer.Reset();
		OA_Log.Log("[OA] Logged out");
	}

	// --- Heartbeat Tick ---

	void OnTick(float dt)
	{
		if (!m_bConnected)
			return;

		if (m_bError)
		{
			if (m_iRetryCount >= RETRY_MAX_COUNT)
				return;

			m_fRetryCooldown -= dt;
			if (m_fRetryCooldown > 0)
				return;

			m_bError = false;
			OA_Log.Log(string.Format("[OA] Retry attempt %1/%2", m_iRetryCount + 1, RETRY_MAX_COUNT));
		}

		if (m_Bridge.IsRequestPending())
			return;

		if (!m_Observer.ShouldScan(dt, m_Config.DecisionInterval))
			return;

		m_Executor.RefreshGroupRegistry();

		int reqId = m_Observer.GetRequestId() + 1;
		int pendingEvents = m_EventTracker.GetPendingCount();
		array<ref OA_BattleEvent> events = m_EventTracker.Flush();

		string situationJson = m_Observer.BuildSituationReport(
			m_Config, m_Bridge.GetConversationId(), m_aSides, events
		);

		int eventCount = 0;
		if (events)
			eventCount = events.Count();

		OA_Log.Log(string.Format("[OA] Sending req#%1 events=%2 running=%3",
			reqId, eventCount, m_bRunning));

		m_Bridge.SendSituation(situationJson, this);
	}


	void OnHeartbeatResult(bool success, OA_CommandResponse response)
	{
		if (!success)
		{
			m_iRetryCount++;
			float delay = Math.Min(RETRY_BASE_DELAY * m_iRetryCount, RETRY_MAX_DELAY);
			m_fRetryCooldown = delay;
			m_bError = true;
			OA_Log.Log(string.Format("[OA] Heartbeat failed (retry %1/%2 in %3s)",
				m_iRetryCount, RETRY_MAX_COUNT, delay));
			return;
		}

		m_bError = false;
		if (m_iRetryCount > 0)
		{
			OA_Log.Log(string.Format("[OA] Connection restored after %1 retries", m_iRetryCount));
			m_iRetryCount = 0;
		}

		if (response)
		{
			ApplyConfig(response);

			if (m_bRunning && response.pending_orders
				&& response.pending_orders.orders
				&& response.pending_orders.orders.Count() > 0)
			{
				m_Executor.ExecuteOrders(response.pending_orders, m_aSides);
			}
		}
	}

	// --- Config from backend ---

	protected void ApplyConfig(OA_CommandResponse response)
	{
		if (!response.config)
			return;

		OA_ArmaConfigBlock cfg = response.config;

		m_bRunning = cfg.running;
		m_Config.DecisionInterval = cfg.decision_interval;
		m_Config.MaxSquads = cfg.max_squads;
		m_Config.EmergencyDetection = cfg.emergency_enabled;

		if (cfg.sides && cfg.sides.Count() > 0)
		{
			m_aSides.Clear();
			foreach (OA_SideConfig side : cfg.sides)
			{
				if (side && side.faction != "")
					m_aSides.Insert(side);
			}
		}

		if (cfg.language != "")
			m_Config.Language = cfg.language;
	}

	// --- Human message (from #oachat) ---

	void SendHumanMessage(string sender, string text)
	{
		m_Observer.AddHumanMessage(sender, text);
	}

	// --- State accessors ---

	bool IsConnected()  { return m_bConnected; }
	bool IsRunning()    { return m_bRunning; }
	bool IsError()      { return m_bError; }

	array<ref OA_SideConfig> GetSides()  { return m_aSides; }

	OA_Config GetConfig()              { return m_Config; }
	OA_WorldObserver GetObserver()     { return m_Observer; }
	OA_DecisionBridge GetBridge()      { return m_Bridge; }
	OA_CommandExecutor GetExecutor()   { return m_Executor; }
	OA_EventTracker GetEventTracker()  { return m_EventTracker; }
}
