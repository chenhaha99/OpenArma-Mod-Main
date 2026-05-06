// OA_HudIndicator.c — Small HUD widget showing connection and running status.
// Grey = disconnected, Yellow = connected idle,
// Green = connected running, Red = connection error.
// Reads state from GameMode (works on both server and client).

class OA_HudIndicator : ScriptedWidgetEventHandler
{
	protected static ref OA_HudIndicator s_Instance;

	protected Widget m_wRoot;
	protected ImageWidget m_wStatusIcon;

	protected float m_fUpdateTimer;
	protected static const float UPDATE_INTERVAL = 2.0;

	protected static const string LAYOUT = "{0000000000000000}UI/layouts/OA_HudIndicator.layout";

	protected static const ResourceName TEX_GREY   = "{143656E23B8BCF3C}UI/Textures/OA_Status_Grey.edds";
	protected static const ResourceName TEX_YELLOW = "{C77D9EEC12A7FE32}UI/Textures/OA_Status_Yellow.edds";
	protected static const ResourceName TEX_GREEN  = "{1112C0A97C88FFFD}UI/Textures/OA_Status_Green.edds";
	protected static const ResourceName TEX_RED    = "{C04332F10B331F48}UI/Textures/OA_Status_Red.edds";

	protected ResourceName m_sCurrentTex;

	static OA_HudIndicator Show()
	{
		if (s_Instance && s_Instance.m_wRoot)
			return s_Instance;

		s_Instance = new OA_HudIndicator();
		s_Instance.CreateUI();
		return s_Instance;
	}

	static void Hide()
	{
		if (!s_Instance || !s_Instance.m_wRoot)
			return;

		s_Instance.m_wRoot.RemoveFromHierarchy();
		s_Instance.m_wRoot = null;
		s_Instance = null;
	}

	protected void CreateUI()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
		{
			OA_Log.Log("[OA] HudIndicator: No workspace");
			return;
		}

		m_wRoot = workspace.CreateWidgets(LAYOUT);
		if (!m_wRoot)
		{
			OA_Log.Log(string.Format("[OA] HudIndicator: Failed to create widgets from '%1'", LAYOUT));
			return;
		}

		OA_Log.Log("[OA] HudIndicator: UI created");

		m_wStatusIcon = ImageWidget.Cast(m_wRoot.FindAnyWidget("StatusIcon"));

		Refresh();
	}

	void Update(float dt)
	{
		m_fUpdateTimer += dt;
		if (m_fUpdateTimer < UPDATE_INTERVAL)
			return;

		m_fUpdateTimer = 0;
		Refresh();
	}

	protected void Refresh()
	{
		if (!m_wStatusIcon)
			return;

		ResourceName tex;
		bool connected = false;
		bool running = false;
		bool error = false;

		SCR_BaseGameMode gm = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gm)
		{
			connected = gm.OA_IsConnected();
			running = gm.OA_IsRunning();
			error = gm.OA_IsError();
		}

		if (!connected)
		{
			if (error)
				tex = TEX_RED;
			else
				tex = TEX_GREY;
		}
		else if (running)
		{
			tex = TEX_GREEN;
		}
		else
		{
			tex = TEX_YELLOW;
		}

		if (tex != m_sCurrentTex)
		{
			m_wStatusIcon.LoadImageTexture(0, tex);
			m_wStatusIcon.SetImage(0);
			m_sCurrentTex = tex;
		}
	}
}
