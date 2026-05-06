// OA_GameModeInjector.c — Hooks OA into any GameMode via modded class.
// Server: runs OA_Main (data collection + REST communication + command execution).
// Client: runs HUD indicator + chat commands.
// RPC is handled by OA_PlayerControllerInjector (modded SCR_PlayerController).
// State sync uses [RplProp] on GameMode (server → all clients).

modded class SCR_BaseGameMode
{
	protected ref OA_Main m_pOA_Main;
	protected ref OA_HudIndicator m_pOA_Hud;
	protected bool m_bOA_Initialized;

	[RplProp()]
	protected bool m_bOA_Connected;
	[RplProp()]
	protected bool m_bOA_Running;
	[RplProp()]
	protected bool m_bOA_Error;

	override void OnGameModeStart()
	{
		super.OnGameModeStart();

		if (m_bOA_Initialized)
			return;

		m_bOA_Initialized = true;

		GetGame().GetCallqueue().CallLater(OA_DeferredInit, 2000, false);
	}

	protected void OA_DeferredInit()
	{
		bool isServer = IsMaster();

		if (isServer)
		{
			m_pOA_Main = OA_Main.CreateInstance();
			GetGame().GetCallqueue().CallLater(OA_ServerTick, 500, true);
			OA_Log.Log("[OA] Server initialized — awaiting login via client RPC");
		}

		if (GetGame().GetWorkspace())
		{
			m_pOA_Hud = OA_HudIndicator.Show();
		}

		OA_ChatCommandHandler.Register();

		if (!isServer)
		{
			GetGame().GetCallqueue().CallLater(OA_ClientTick, 2000, true);
			OA_Log.Log("[OA] Client initialized — use /oalogin <key> to connect");
		}
		else
		{
			OA_Log.Log("[OA] Initialized — use /oalogin <key> to connect");
		}
	}

	override void OnGameModeEnd(SCR_GameModeEndData endData)
	{
		super.OnGameModeEnd(endData);

		if (m_pOA_Main && m_pOA_Main.IsConnected())
			m_pOA_Main.Logout();

		OA_Log.Close();
	}

	protected void OA_ServerTick()
	{
		if (m_pOA_Main)
		{
			m_pOA_Main.OnTick(0.5);
			OA_SyncState();
		}

		if (m_pOA_Hud)
			m_pOA_Hud.Update(0.5);
	}

	protected void OA_ClientTick()
	{
		if (m_pOA_Hud)
			m_pOA_Hud.Update(2.0);
	}

	protected void OA_SyncState()
	{
		if (!m_pOA_Main)
			return;

		bool newConnected = m_pOA_Main.IsConnected();
		bool newRunning = m_pOA_Main.IsRunning();
		bool newError = m_pOA_Main.IsError();

		if (newConnected != m_bOA_Connected || newRunning != m_bOA_Running || newError != m_bOA_Error)
		{
			m_bOA_Connected = newConnected;
			m_bOA_Running = newRunning;
			m_bOA_Error = newError;
			Replication.BumpMe();
		}
	}

	// --- Server-side handlers (called by OA_PlayerControllerInjector RPC) ---

	void OA_HandleLogin(string apiKey, string urlOverride)
	{
		if (!m_pOA_Main)
		{
			OA_Log.Log("[OA] Server: OA_Main not initialized");
			return;
		}

		if (m_pOA_Main.IsConnected())
		{
			OA_Log.Log("[OA] Server: Already connected. Logout first.");
			return;
		}

		if (urlOverride == "local" || urlOverride == "dev")
		{
			m_pOA_Main.GetConfig().ApiUrl = "http://localhost:8000";
			OA_Log.Log("[OA] Server: URL override → http://localhost:8000");
		}
		else if (urlOverride.Length() > 4 && urlOverride.Substring(0, 4) == "http")
		{
			m_pOA_Main.GetConfig().ApiUrl = urlOverride;
			OA_Log.Log(string.Format("[OA] Server: URL override → %1", urlOverride));
		}

		OA_Log.Log(string.Format("[OA] Server: Connecting to %1 ...", m_pOA_Main.GetConfig().ApiUrl));
		m_pOA_Main.Login(apiKey);
	}

	void OA_HandleLogout()
	{
		if (!m_pOA_Main || !m_pOA_Main.IsConnected())
		{
			OA_Log.Log("[OA] Server: Not connected.");
			return;
		}

		m_pOA_Main.Logout();
		OA_Log.Log("[OA] Server: Disconnected.");
	}

	void OA_HandleChat(string sender, string text)
	{
		if (!m_pOA_Main || !m_pOA_Main.IsConnected())
		{
			OA_Log.Log("[OA] Server: Not connected, chat ignored.");
			return;
		}

		m_pOA_Main.SendHumanMessage(sender, text);
		OA_Log.Log("[OA] Server: Message queued for AI.");
	}

	// --- State accessors (replicated, works on all machines) ---

	bool OA_IsConnected()
	{
		return m_bOA_Connected;
	}

	bool OA_IsRunning()
	{
		return m_bOA_Running;
	}

	bool OA_IsError()
	{
		return m_bOA_Error;
	}

	OA_Main OA_GetMain()
	{
		return m_pOA_Main;
	}
}
