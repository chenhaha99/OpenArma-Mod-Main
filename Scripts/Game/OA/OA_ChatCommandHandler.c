// OA_ChatCommandHandler.c — Registers /oalogin, /oalogout, /oachat, /oatest chat commands.
// Commands run on the client and send RPCs to the server via PlayerController.
// All commands require GM privileges.

class OA_ChatCommandHandler
{
	static void Register()
	{
		SCR_ChatPanelManager mgr = SCR_ChatPanelManager.GetInstance();
		if (!mgr)
		{
			OA_Log.Log("[OA] ChatPanelManager not available, deferring command registration");
			return;
		}

		ChatCommandInvoker loginInv = mgr.GetCommandInvoker("oalogin");
		if (loginInv)
			loginInv.Insert(OnCommandLogin);

		ChatCommandInvoker logoutInv = mgr.GetCommandInvoker("oalogout");
		if (logoutInv)
			logoutInv.Insert(OnCommandLogout);

		ChatCommandInvoker chatInv = mgr.GetCommandInvoker("oachat");
		if (chatInv)
			chatInv.Insert(OnCommandChat);

		ChatCommandInvoker testInv = mgr.GetCommandInvoker("oatest");
		if (testInv)
			testInv.Insert(OnCommandTest);

		OA_Log.Log("[OA] Chat commands registered: /oalogin /oalogout /oachat /oatest");
	}

	protected static SCR_PlayerController GetLocalPC()
	{
		return SCR_PlayerController.Cast(GetGame().GetPlayerController());
	}

	protected static void OnCommandLogin(SCR_ChatPanel panel, string data)
	{
		if (!HasGameMasterAccess())
		{
			OA_Log.Log("[OA] Command rejected: no Game Master access");
			return;
		}

		string trimmed = data.Trim();
		if (trimmed.IsEmpty())
		{
			OA_Log.Log("[OA] Usage: /oalogin <api_key> [local|<url>]");
			return;
		}

		string apiKey;
		string urlOverride;
		int spaceIdx = trimmed.IndexOf(" ");
		if (spaceIdx > 0)
		{
			apiKey = trimmed.Substring(0, spaceIdx);
			urlOverride = trimmed.Substring(spaceIdx + 1, trimmed.Length() - spaceIdx - 1).Trim();
		}
		else
		{
			apiKey = trimmed;
			urlOverride = "";
		}

		SCR_PlayerController pc = GetLocalPC();
		if (!pc)
		{
			OA_Log.Log("[OA] PlayerController not available.");
			return;
		}

		OA_Log.Log("[OA] Requesting login → server...");
		pc.OA_RequestLogin(apiKey, urlOverride);
	}

	protected static void OnCommandLogout(SCR_ChatPanel panel, string data)
	{
		if (!HasGameMasterAccess())
		{
			OA_Log.Log("[OA] Command rejected: no Game Master access");
			return;
		}

		SCR_PlayerController pc = GetLocalPC();
		if (!pc)
		{
			OA_Log.Log("[OA] PlayerController not available.");
			return;
		}

		OA_Log.Log("[OA] Requesting logout → server...");
		pc.OA_RequestLogout();
	}

	protected static void OnCommandChat(SCR_ChatPanel panel, string data)
	{
		if (!HasGameMasterAccess())
		{
			OA_Log.Log("[OA] Command rejected: no Game Master access");
			return;
		}

		string text = data.Trim();
		if (text.IsEmpty())
		{
			OA_Log.Log("[OA] Usage: /oachat <message>");
			return;
		}

		SCR_PlayerController pc = GetLocalPC();
		if (!pc)
		{
			OA_Log.Log("[OA] PlayerController not available.");
			return;
		}

		OA_Log.Log("[OA] Sending message → server...");
		pc.OA_RequestChat("GameMaster", text);
	}

	protected static void OnCommandTest(SCR_ChatPanel panel, string data)
	{
		if (!HasGameMasterAccess())
		{
			OA_Log.Log("[OA] Command rejected: no Game Master access");
			return;
		}

		OA_Log.Log("[OA] === SELF-TEST START (build:20260401d) ===");

		SCR_BaseGameMode gm = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		bool isServer = false;
		OA_Main oa = null;

		if (gm)
		{
			isServer = gm.IsMaster();
			oa = gm.OA_GetMain();
		}

		string machineRole = "CLIENT";
		if (isServer)
			machineRole = "SERVER";
		OA_Log.Log(string.Format("[OA] Running on: %1", machineRole));
		OA_Log.Log(string.Format("[OA] Replicated state: connected=%1 running=%2 error=%3",
			gm.OA_IsConnected(), gm.OA_IsRunning(), gm.OA_IsError()));

		OA_Log.Log("[OA] --- FACTIONS ---");
		FactionManager fm = GetGame().GetFactionManager();
		if (fm)
		{
			array<Faction> allFactions = {};
			int fCount = fm.GetFactionsList(allFactions);
			OA_Log.Log(string.Format("[OA] Total factions registered: %1", fCount));
			foreach (int i, Faction fac : allFactions)
			{
				string fKey = fac.GetFactionKey();
				string fName = fac.GetFactionName();
				Color fColor = fac.GetFactionColor();
				string colorStr = "";
				if (fColor)
					colorStr = string.Format("RGBA(%1,%2,%3,%4)",
						(int)(fColor.R() * 255), (int)(fColor.G() * 255),
						(int)(fColor.B() * 255), (int)(fColor.A() * 255));
				OA_Log.Log(string.Format("[OA]   [%1] key=\"%2\" name=\"%3\" color=%4",
					i, fKey, fName, colorStr));
			}
		}
		else
		{
			OA_Log.Log("[OA] FactionManager not available");
		}

		OA_Log.Log("[OA] --- AI GROUPS (via GetAIAgents) ---");

		OA_CommandExecutor executor;
		if (oa)
			executor = oa.GetExecutor();

		if (!executor)
			executor = new OA_CommandExecutor();

		executor.RefreshGroupRegistry();

		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld)
		{
			OA_Log.Log("[OA] AIWorld not available");
			if (!isServer)
				OA_Log.Log("[OA] NOTE: On client, AIWorld is empty. Test on server or local host.");
			OA_Log.Log("[OA] === SELF-TEST DONE ===");
			return;
		}

		OA_Log.Log(string.Format("[OA] AIWorld AI limit=%1 current=%2 active=%3",
			aiWorld.GetAILimit(), aiWorld.GetCurrentAmountOfLimitedAIs(),
			aiWorld.GetCurrentNumOfActiveAIs()));

		array<AIAgent> agents = {};
		aiWorld.GetAIAgents(agents);
		OA_Log.Log(string.Format("[OA] GetAIAgents returned: %1 agents total", agents.Count()));

		array<SCR_AIGroup> testGroups = {};
		map<string, int> factionGroupCount = new map<string, int>();
		int nonGroupAgents = 0;
		int aiGroupOnly = 0;

		foreach (AIAgent agent : agents)
		{
			SCR_AIGroup scrGroup = SCR_AIGroup.Cast(agent);
			if (scrGroup)
			{
				testGroups.Insert(scrGroup);

				string gFaction = "unknown";
				Faction gFac = scrGroup.GetFaction();
				if (gFac)
					gFaction = gFac.GetFactionKey();

				int memberCount = scrGroup.GetAgentsCount();
				vector pos = scrGroup.GetOrigin();

				OA_Log.Log(string.Format("[OA]   SCR_AIGroup id=%1 faction=\"%2\" members=%3 pos=(%4, %5, %6)",
					scrGroup.GetID(), gFaction, memberCount,
					(int)pos[0], (int)pos[1], (int)pos[2]));

				if (factionGroupCount.Contains(gFaction))
					factionGroupCount.Set(gFaction, factionGroupCount.Get(gFaction) + 1);
				else
					factionGroupCount.Set(gFaction, 1);
			}
			else
			{
				AIGroup baseGroup = AIGroup.Cast(agent);
				if (baseGroup)
				{
					aiGroupOnly++;
					OA_Log.Log(string.Format("[OA]   AIGroup(non-SCR) id=%1 members=%2",
						baseGroup.GetID(), baseGroup.GetAgentsCount()));
				}
				else
				{
					nonGroupAgents++;
				}
			}
		}

		OA_Log.Log(string.Format("[OA] SCR_AIGroup: %1, AIGroup(non-SCR): %2, non-group agents: %3",
			testGroups.Count(), aiGroupOnly, nonGroupAgents));

		OA_Log.Log("[OA] --- GROUPS (via SCR_GroupsManagerComponent) ---");
		SCR_GroupsManagerComponent groupsMgr = SCR_GroupsManagerComponent.GetInstance();
		if (groupsMgr)
		{
			array<SCR_AIGroup> allPlayableGroups = {};
			groupsMgr.GetAllPlayableGroups(allPlayableGroups);
			OA_Log.Log(string.Format("[OA] GetAllPlayableGroups: %1 groups", allPlayableGroups.Count()));

			foreach (SCR_AIGroup pg : allPlayableGroups)
			{
				if (!pg) continue;
				Faction pFac = pg.GetFaction();
				string pFaction = "null";
				if (pFac)
					pFaction = pFac.GetFactionKey();
				vector pPos = pg.GetOrigin();
				bool alreadyInAIWorld = testGroups.Contains(pg);
				OA_Log.Log(string.Format("[OA]   Playable id=%1 faction=\"%2\" members=%3 pos=(%4,%5,%6) inAIWorld=%7",
					pg.GetID(), pFaction, pg.GetAgentsCount(),
					(int)pPos[0], (int)pPos[1], (int)pPos[2], alreadyInAIWorld));
			}
		}
		else
		{
			OA_Log.Log("[OA] SCR_GroupsManagerComponent not available");
		}

		OA_Log.Log("[OA] --- FACTION GROUP SUMMARY ---");

		for (int fi = 0; fi < factionGroupCount.Count(); fi++)
		{
			string fk = factionGroupCount.GetKey(fi);
			int fc = factionGroupCount.GetElement(fi);
			OA_Log.Log(string.Format("[OA]   \"%1\": %2 groups", fk, fc));
		}

		if (testGroups.IsEmpty())
		{
			OA_Log.Log("[OA] No AI groups to test commands on.");
			OA_Log.Log("[OA] === SELF-TEST DONE ===");
			return;
		}

		string trimmedData = data.Trim();
		int testMode = 0;
		int testDuration = 120;
		if (!trimmedData.IsEmpty())
		{
			array<string> parts = {};
			trimmedData.Split(" ", parts, true);
			if (parts.Count() >= 1)
				testMode = parts[0].ToInt();
			if (parts.Count() >= 2)
				testDuration = parts[1].ToInt();
			if (testDuration < 30)
				testDuration = 30;
			if (testDuration > 1800)
				testDuration = 1800;
		}

		if (testMode == 1)
		{
			OA_Log.Log("[OA] --- PATROL CYCLE TEST ---");
			if (testGroups.IsEmpty())
			{
				OA_Log.Log("[OA] No SCR_AIGroup found for patrol test.");
				OA_Log.Log("[OA] === SELF-TEST DONE ===");
				return;
			}

			SCR_AIGroup pg = testGroups[0];
			string pLbl = OA_CommandExecutor.GetGroupLabel(pg);
			vector pOrigin = pg.GetOrigin();

			OA_Log.Log(string.Format("[OA] Patrol test: %1 duration=%2s", pLbl, testDuration));
			OA_Log.Log(string.Format("[OA]   Start pos: (%1, %2, %3)",
				(int)pOrigin[0], (int)pOrigin[1], (int)pOrigin[2]));

			OA_Order pOrd = new OA_Order();
			pOrd.type = "patrol";
			pOrd.group_id = pg.GetID().ToString();
			pOrd.mode = "cycle";
			pOrd.speed = "jog";
			pOrd.waypoints = {};

			float r = 200;
			array<vector> wpPositions = {};
			wpPositions.Insert(Vector(pOrigin[0] + r, 0, pOrigin[2]));
			wpPositions.Insert(Vector(pOrigin[0], 0, pOrigin[2] + r));
			wpPositions.Insert(Vector(pOrigin[0] - r, 0, pOrigin[2]));
			wpPositions.Insert(Vector(pOrigin[0], 0, pOrigin[2] - r));

			for (int wi = 0; wi < wpPositions.Count(); wi++)
			{
				vector wv = wpPositions[wi];
				wv[1] = GetGame().GetWorld().GetSurfaceY(wv[0], wv[2]);
				OA_Waypoint wpData = new OA_Waypoint();
				wpData.pos = {wv[0], wv[1], wv[2]};
				pOrd.waypoints.Insert(wpData);
				OA_Log.Log(string.Format("[OA]   WP%1: (%2, %3, %4)",
					wi + 1, (int)wv[0], (int)wv[1], (int)wv[2]));
			}

			executor.ExecuteOrderDirect(pg, pOrd);
			OA_Log.Log("[OA]   PATROL CYCLE issued (4 waypoints, 200m square)");

			int trackCount = testDuration / 10;
			OA_Log.Log(string.Format("[OA] --- PATROL TRACKING (every 10s for %1s, %2 checks) ---",
				testDuration, trackCount));
			for (int ti = 0; ti < trackCount; ti++)
			{
				GetGame().GetCallqueue().CallLater(
					OA_ChatCommandHandler.LogPatrolPosition, (ti + 1) * 10000, false, pg);
			}

			OA_Log.Log("[OA] === PATROL TEST STARTED ===");
			return;
		}

		OA_Log.Log("[OA] --- COMMAND TEST ---");
		int passed = 0;
		SCR_AIGroup g;
		OA_Order ord;
		vector origin;
		string lbl;

		if (testGroups.Count() >= 1)
		{
			g = testGroups[0];
			lbl = OA_CommandExecutor.GetGroupLabel(g);
			origin = g.GetOrigin();
			ord = new OA_Order();
			ord.type = "move";
			ord.group_id = g.GetID().ToString();
			ord.target = {origin[0] + 100, origin[1], origin[2] + 100};
			ord.speed = "jog";
			executor.ExecuteOrderDirect(g, ord);
			OA_Log.Log(string.Format("[OA]   MOVE: %1 -> +100,+100", lbl));
			passed++;
		}

		if (testGroups.Count() >= 2)
		{
			g = testGroups[1];
			lbl = OA_CommandExecutor.GetGroupLabel(g);
			origin = g.GetOrigin();
			ord = new OA_Order();
			ord.type = "defend";
			ord.group_id = g.GetID().ToString();
			ord.target = {origin[0], origin[1], origin[2]};
			ord.duration = 120;
			executor.ExecuteOrderDirect(g, ord);
			OA_Log.Log(string.Format("[OA]   DEFEND: %1 at current pos", lbl));
			passed++;
		}

		OA_Log.Log(string.Format("[OA] === SELF-TEST DONE: %1 commands issued ===", passed));
	}

	static void LogPatrolPosition(SCR_AIGroup group)
	{
		if (!group)
		{
			OA_Log.Log("[OA] [PATROL-TRACK] Group destroyed");
			return;
		}

		vector pos = group.GetOrigin();
		string wt = "?";
		AIWaypoint wp = group.GetCurrentWaypoint();
		if (wp)
		{
			if (AIWaypointCycle.Cast(wp))
				wt = "cycle";
			else
				wt = "waypoint";
		}
		else
		{
			wt = "none";
		}

		int memberCount = group.GetAgentsCount();

		array<AIWaypoint> allWps = {};
		group.GetWaypoints(allWps);
		int wpCount = allWps.Count();

		OA_Log.Log(string.Format("[OA] [PATROL-TRACK] pos=(%1,%2,%3) wp_type=%4 wp_count=%5 members=%6",
			(int)pos[0], (int)pos[1], (int)pos[2], wt, wpCount, memberCount));
	}

	protected static bool HasGameMasterAccess()
	{
		SCR_EditorManagerEntity editorMgr = SCR_EditorManagerEntity.GetInstance();
		if (!editorMgr)
			return false;

		return !SCR_EditorManagerEntity.IsLimitedInstance();
	}
}
