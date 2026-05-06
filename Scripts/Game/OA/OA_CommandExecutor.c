// OA_CommandExecutor.c — Translates AI orders into game actions (Waypoints, CombatMode, Speed).
// Supports N-sided dynamic faction config via OA_SideConfig array.

class OA_CommandExecutor
{
	protected ref map<string, SCR_AIGroup> m_mGroups;

	void OA_CommandExecutor()
	{
		m_mGroups = new map<string, SCR_AIGroup>();
	}

	static string GetGroupLabel(notnull SCR_AIGroup group)
	{
		string callsign = "";

		SCR_EditableGroupComponent editComp = SCR_EditableGroupComponent.Cast(group.FindComponent(SCR_EditableGroupComponent));
		if (editComp)
		{
			string displayName = editComp.GetDisplayName();
			if (displayName != "" && displayName.IndexOf("#") != 0)
				callsign = displayName;
		}

		if (callsign == "")
		{
			SCR_CallsignGroupComponent csComp = SCR_CallsignGroupComponent.Cast(group.FindComponent(SCR_CallsignGroupComponent));
			if (csComp)
			{
				int compIdx, platIdx, squadIdx;
				if (csComp.GetCallsignIndexes(compIdx, platIdx, squadIdx))
				{
					string compName = _PhoneticName(compIdx);
					callsign = string.Format("%1-%2-%3", compName, platIdx + 1, squadIdx + 1);
				}
			}
		}

		int count = group.GetAgentsCount();

		Faction fac = group.GetFaction();
		string facKey = "";
		if (fac)
			facKey = fac.GetFactionKey();

		string grid = OA_Log.GridRef(group.GetOrigin());

		if (callsign == "")
			callsign = group.GetID().ToString();

		if (facKey != "")
			return string.Format("%1(%2 x%3 @%4)", callsign, facKey, count, grid);
		return string.Format("%1(x%2 @%3)", callsign, count, grid);
	}

	static string _PhoneticName(int idx)
	{
		switch (idx)
		{
			case 0: return "Alpha";
			case 1: return "Bravo";
			case 2: return "Charlie";
			case 3: return "Delta";
			case 4: return "Echo";
			case 5: return "Foxtrot";
			case 6: return "Golf";
			case 7: return "Hotel";
			case 8: return "India";
			case 9: return "Juliet";
			case 10: return "Kilo";
			case 11: return "Lima";
		}
		return idx.ToString();
	}

	void RefreshGroupRegistry()
	{
		m_mGroups.Clear();

		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld)
			return;

		array<AIAgent> agents = {};
		aiWorld.GetAIAgents(agents);

		foreach (AIAgent agent : agents)
		{
			SCR_AIGroup group = SCR_AIGroup.Cast(agent);
			if (!group)
				continue;

			string gid = group.GetID().ToString();
			m_mGroups.Set(gid, group);
		}
	}

	void ExecuteOrders(OA_PendingOrders pendingOrders, array<ref OA_SideConfig> sides)
	{
		if (!pendingOrders || !pendingOrders.orders)
			return;

		int total = pendingOrders.orders.Count();
		OA_Log.Log(string.Format("[OA] Executing %1 orders...", total));

		int executed = 0;
		int skippedFaction = 0;

		foreach (OA_Order order : pendingOrders.orders)
		{
			if (!order)
				continue;

			SCR_AIGroup group = FindGroupById(order.group_id);
			if (!group)
			{
				OA_Log.Log(string.Format("[OA] Group not found: %1", order.group_id));
				continue;
			}

			string label = GetGroupLabel(group);

			if (!IsGroupLLMControlled(group, sides))
			{
				OA_Log.Log(string.Format("[OA] Skipping %1: not LLM-controlled", label));
				skippedFaction++;
				continue;
			}

			string orderType = order.type;
			string targetInfo = "";
			if (order.target && order.target.Count() >= 3)
				targetInfo = string.Format(" @%1 [%2,%3,%4]",
					OA_Log.GridRefXZ(order.target[0], order.target[2]),
					order.target[0], order.target[1], order.target[2]);
			else if (order.waypoints && order.waypoints.Count() > 0)
				targetInfo = string.Format(" %1wp", order.waypoints.Count());
			OA_Log.Log(string.Format("[OA] %1 -> %2%3", orderType, label, targetInfo));

			if (orderType == "move")
				ExecuteMove(group, order);
			else if (orderType == "attack")
				ExecuteAttack(group, order);
			else if (orderType == "retreat")
				ExecuteRetreat(group, order);
			else if (orderType == "force_move")
				ExecuteForceMove(group, order);
			else if (orderType == "defend")
				ExecuteDefend(group, order);
			else if (orderType == "patrol")
				ExecutePatrol(group, order);
			else if (orderType == "route")
				ExecuteRoute(group, order);
			else if (orderType == "set_combat_mode")
				ExecuteSetCombatMode(group, order);
			else if (orderType == "set_speed")
				ExecuteSetSpeed(group, order);
			else if (orderType == "hold")
				ExecuteHold(group, order);
			else
			{
				OA_Log.Log(string.Format("[OA] Unknown order type: %1", orderType));
				continue;
			}
			executed++;
		}

		OA_Log.Log(string.Format("[OA] Executed %1/%2 orders (skipped %3 non-LLM)",
			executed, total, skippedFaction));
	}

	protected bool IsGroupLLMControlled(notnull SCR_AIGroup group, array<ref OA_SideConfig> sides)
	{
		Faction groupFaction = group.GetFaction();
		if (!groupFaction)
			return true;

		if (!sides || sides.Count() == 0)
			return true;

		string groupFactionKey = groupFaction.GetFactionKey();
		FactionManager fm = GetGame().GetFactionManager();
		if (!fm)
			return true;

		bool matchedAnySide = false;
		foreach (OA_SideConfig side : sides)
		{
			if (!side || side.faction == "")
				continue;

			Faction sideFaction = fm.GetFactionByKey(side.faction);
			if (sideFaction && groupFaction == sideFaction)
			{
				matchedAnySide = true;
				return (side.control == "llm");
			}
		}

		if (!matchedAnySide)
			return true;

		return false;
	}

	void ExecuteOrderDirect(notnull SCR_AIGroup group, notnull OA_Order order)
	{
		string orderType = order.type;

		if (orderType == "move")
			ExecuteMove(group, order);
		else if (orderType == "attack")
			ExecuteAttack(group, order);
		else if (orderType == "retreat")
			ExecuteRetreat(group, order);
		else if (orderType == "force_move")
			ExecuteForceMove(group, order);
		else if (orderType == "defend")
			ExecuteDefend(group, order);
		else if (orderType == "patrol")
			ExecutePatrol(group, order);
		else if (orderType == "route")
			ExecuteRoute(group, order);
		else if (orderType == "set_combat_mode")
			ExecuteSetCombatMode(group, order);
		else if (orderType == "set_speed")
			ExecuteSetSpeed(group, order);
		else if (orderType == "hold")
			ExecuteHold(group, order);
		else
			OA_Log.Log(string.Format("[OA] Unknown order type: %1", orderType));
	}

	protected SCR_AIGroup FindGroupById(string groupId)
	{
		if (m_mGroups.Contains(groupId))
			return m_mGroups.Get(groupId);

		return null;
	}

	protected void ClearGroupWaypoints(notnull SCR_AIGroup group)
	{
		array<AIWaypoint> currentWps = {};
		group.GetWaypoints(currentWps);

		int removed = 0;
		foreach (AIWaypoint wp : currentWps)
		{
			group.RemoveWaypoint(wp);
			SCR_EntityHelper.DeleteEntityAndChildren(wp);
			removed++;
		}

		if (removed > 0)
			OA_Log.Log(string.Format("[OA]   Cleared %1 waypoint(s) from %2", removed, GetGroupLabel(group)));
	}

	protected void ExecuteMove(notnull SCR_AIGroup group, notnull OA_Order order)
	{
		if (!order.target || order.target.Count() < 3)
			return;

		ClearGroupWaypoints(group);

		vector targetPos = Vector(order.target[0], order.target[1], order.target[2]);

		float surfY = GetGame().GetWorld().GetSurfaceY(targetPos[0], targetPos[2]);
		targetPos[1] = surfY;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = targetPos;

		Resource wpRes = Resource.Load("{750A8D1695BD6998}Prefabs/AI/Waypoints/AIWaypoint_Move.et");
		if (!wpRes.IsValid())
			return;

		AIWaypoint wp = AIWaypoint.Cast(GetGame().SpawnEntityPrefab(wpRes, GetGame().GetWorld(), params));
		if (!wp)
			return;

		wp.SetCompletionRadius(20);
		group.AddWaypoint(wp);
		OA_Log.Log(string.Format("[OA]   WP @%1 [%2,%3,%4] -> %5",
			OA_Log.GridRef(targetPos),
			targetPos[0], targetPos[1], targetPos[2],
			GetGroupLabel(group)));

		if (order.speed != "")
			ApplySpeed(group, order.speed);

		if (order.mode != "")
			ApplyCombatMode(group, order.mode);
		else
			ApplyCombatMode(group, "fire_at_will");

		ApplyFormation(group, order.formation);
	}

	protected void ExecuteAttack(notnull SCR_AIGroup group, notnull OA_Order order)
	{
		if (!order.target || order.target.Count() < 3)
			return;

		ClearGroupWaypoints(group);

		vector targetPos = Vector(order.target[0], order.target[1], order.target[2]);
		targetPos[1] = GetGame().GetWorld().GetSurfaceY(targetPos[0], targetPos[2]);

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = targetPos;

		Resource wpRes = Resource.Load("{750A8D1695BD6998}Prefabs/AI/Waypoints/AIWaypoint_Move.et");
		if (!wpRes.IsValid())
			return;

		AIWaypoint wp = AIWaypoint.Cast(GetGame().SpawnEntityPrefab(wpRes, GetGame().GetWorld(), params));
		if (!wp)
			return;

		wp.SetCompletionRadius(20);
		wp.SetCompletionType(EAIWaypointCompletionType.All);
		group.AddWaypoint(wp);

		ApplyCombatMode(group, "fire_at_will");

		if (order.speed != "")
			ApplySpeed(group, order.speed);
		else
			ApplySpeed(group, "jog");

		if (order.formation != "")
			ApplyFormation(group, order.formation);
		else
			ApplyFormation(group, "Wedge");

		OA_Log.Log(string.Format("[OA]   ATTACK @%1 -> %2 (FIRE_AT_WILL, Wedge)",
			OA_Log.GridRef(targetPos), GetGroupLabel(group)));
	}

	protected void ExecuteRetreat(notnull SCR_AIGroup group, notnull OA_Order order)
	{
		if (!order.target || order.target.Count() < 3)
			return;

		ClearGroupWaypoints(group);

		vector targetPos = Vector(order.target[0], order.target[1], order.target[2]);
		targetPos[1] = GetGame().GetWorld().GetSurfaceY(targetPos[0], targetPos[2]);

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = targetPos;

		Resource wpRes = Resource.Load("{750A8D1695BD6998}Prefabs/AI/Waypoints/AIWaypoint_Move.et");
		if (!wpRes.IsValid())
			return;

		AIWaypoint wp = AIWaypoint.Cast(GetGame().SpawnEntityPrefab(wpRes, GetGame().GetWorld(), params));
		if (!wp)
			return;

		wp.SetCompletionRadius(20);
		group.AddWaypoint(wp);

		ApplyCombatMode(group, "fire_at_will");
		ApplySpeed(group, "sprint");

		if (order.formation != "")
			ApplyFormation(group, order.formation);
		else
			ApplyFormation(group, "Column");

		OA_Log.Log(string.Format("[OA]   RETREAT @%1 -> %2 (FIRE_AT_WILL, sprint, Column)",
			OA_Log.GridRef(targetPos), GetGroupLabel(group)));
	}

	protected void ExecuteForceMove(notnull SCR_AIGroup group, notnull OA_Order order)
	{
		if (!order.target || order.target.Count() < 3)
			return;

		ClearGroupWaypoints(group);

		vector targetPos = Vector(order.target[0], order.target[1], order.target[2]);
		targetPos[1] = GetGame().GetWorld().GetSurfaceY(targetPos[0], targetPos[2]);

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = targetPos;

		Resource wpRes = Resource.Load("{750A8D1695BD6998}Prefabs/AI/Waypoints/AIWaypoint_Move.et");
		if (!wpRes.IsValid())
			return;

		AIWaypoint wp = AIWaypoint.Cast(GetGame().SpawnEntityPrefab(wpRes, GetGame().GetWorld(), params));
		if (!wp)
			return;

		wp.SetCompletionRadius(20);
		group.AddWaypoint(wp);

		ApplyCombatMode(group, "fire_at_will");

		if (order.speed != "")
			ApplySpeed(group, order.speed);
		else
			ApplySpeed(group, "sprint");

		if (order.formation != "")
			ApplyFormation(group, order.formation);
		else
			ApplyFormation(group, "Column");

		OA_Log.Log(string.Format("[OA]   FORCE_MOVE @%1 -> %2 (FIRE_AT_WILL, sprint)",
			OA_Log.GridRef(targetPos), GetGroupLabel(group)));
	}

	protected void ExecuteDefend(notnull SCR_AIGroup group, notnull OA_Order order)
	{
		vector defendPos;
		if (order.target && order.target.Count() >= 3)
			defendPos = Vector(order.target[0], order.target[1], order.target[2]);
		else
			defendPos = group.GetOrigin();

		float surfY = GetGame().GetWorld().GetSurfaceY(defendPos[0], defendPos[2]);
		defendPos[1] = surfY;

		ClearGroupWaypoints(group);

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = defendPos;

		Resource wpRes = Resource.Load("{93291E72AC23930F}Prefabs/AI/Waypoints/AIWaypoint_Defend.et");
		if (!wpRes.IsValid())
			return;

		SCR_DefendWaypoint wp = SCR_DefendWaypoint.Cast(GetGame().SpawnEntityPrefab(wpRes, GetGame().GetWorld(), params));
		if (!wp)
			return;

		wp.SetCompletionRadius(50);

		float holdTime = order.duration;
		if (holdTime <= 0)
			holdTime = 120;
		wp.SetHoldingTime(holdTime);

		group.AddWaypoint(wp);

		if (order.mode != "")
			ApplyCombatMode(group, order.mode);
		else
			ApplyCombatMode(group, "fire_at_will");

		if (order.formation != "")
			ApplyFormation(group, order.formation);
		else
			ApplyFormation(group, "Line");

		OA_Log.Log(string.Format("[OA]   DEFEND @%1 -> %2 (hold=%3s)",
			OA_Log.GridRef(defendPos), GetGroupLabel(group), holdTime));
	}

	protected void ExecutePatrol(notnull SCR_AIGroup group, notnull OA_Order order)
	{
		if (!order.waypoints || order.waypoints.Count() < 2)
		{
			int wpCount = 0;
			if (order.waypoints)
				wpCount = order.waypoints.Count();
			OA_Log.Log(string.Format("[OA] Patrol skipped for %1: waypoints=%2 (need >=2)",
				GetGroupLabel(group), wpCount));
			return;
		}

		ClearGroupWaypoints(group);

		Resource wpRes = Resource.Load("{750A8D1695BD6998}Prefabs/AI/Waypoints/AIWaypoint_Move.et");
		if (!wpRes.IsValid())
			return;

		array<AIWaypoint> patrolWps = {};

		foreach (OA_Waypoint wpData : order.waypoints)
		{
			if (!wpData || !wpData.pos || wpData.pos.Count() < 3)
				continue;

			vector wpPos = Vector(wpData.pos[0], wpData.pos[1], wpData.pos[2]);
			wpPos[1] = GetGame().GetWorld().GetSurfaceY(wpPos[0], wpPos[2]);

			EntitySpawnParams params = new EntitySpawnParams();
			params.TransformMode = ETransformMode.WORLD;
			params.Transform[3] = wpPos;

			AIWaypoint wp = AIWaypoint.Cast(GetGame().SpawnEntityPrefab(wpRes, GetGame().GetWorld(), params));
			if (wp)
			{
				wp.SetCompletionRadius(20);
				wp.SetCompletionType(EAIWaypointCompletionType.All);
				patrolWps.Insert(wp);
			}
		}

		if (patrolWps.IsEmpty())
			return;

		Resource cycleRes = Resource.Load("{35BD6541CBB8AC08}Prefabs/AI/Waypoints/AIWaypoint_Cycle.et");
		if (!cycleRes.IsValid())
		{
			foreach (AIWaypoint wp : patrolWps)
				group.AddWaypoint(wp);
			return;
		}

		EntitySpawnParams cycleParams = new EntitySpawnParams();
		cycleParams.TransformMode = ETransformMode.WORLD;
		cycleParams.Transform[3] = group.GetOrigin();

		AIWaypointCycle cycleWp = AIWaypointCycle.Cast(GetGame().SpawnEntityPrefab(cycleRes, GetGame().GetWorld(), cycleParams));
		if (cycleWp)
		{
			cycleWp.SetWaypoints(patrolWps);
			cycleWp.SetRerunCounter(-1);
			group.AddWaypoint(cycleWp);
		}
		else
		{
			foreach (AIWaypoint wp : patrolWps)
				group.AddWaypoint(wp);
		}

		if (order.mode != "")
			ApplyCombatMode(group, order.mode);
		else
			ApplyCombatMode(group, "fire_at_will");

		if (order.speed != "")
			ApplySpeed(group, order.speed);

		ApplyFormation(group, order.formation);
	}

	protected void ExecuteRoute(notnull SCR_AIGroup group, notnull OA_Order order)
	{
		if (!order.waypoints || order.waypoints.Count() < 2)
		{
			int wpCount = 0;
			if (order.waypoints)
				wpCount = order.waypoints.Count();
			OA_Log.Log(string.Format("[OA] Route skipped for %1: waypoints=%2 (need >=2)",
				GetGroupLabel(group), wpCount));
			return;
		}

		ClearGroupWaypoints(group);

		Resource wpRes = Resource.Load("{750A8D1695BD6998}Prefabs/AI/Waypoints/AIWaypoint_Move.et");
		if (!wpRes.IsValid())
			return;

		int added = 0;
		foreach (OA_Waypoint wpData : order.waypoints)
		{
			if (!wpData || !wpData.pos || wpData.pos.Count() < 3)
				continue;

			vector wpPos = Vector(wpData.pos[0], wpData.pos[1], wpData.pos[2]);
			wpPos[1] = GetGame().GetWorld().GetSurfaceY(wpPos[0], wpPos[2]);

			EntitySpawnParams params = new EntitySpawnParams();
			params.TransformMode = ETransformMode.WORLD;
			params.Transform[3] = wpPos;

			AIWaypoint wp = AIWaypoint.Cast(GetGame().SpawnEntityPrefab(wpRes, GetGame().GetWorld(), params));
			if (wp)
			{
				wp.SetCompletionRadius(20);
				wp.SetCompletionType(EAIWaypointCompletionType.All);
				group.AddWaypoint(wp);
				added++;
			}
		}

		OA_Log.Log(string.Format("[OA]   route: %1 waypoints added for %2", added, GetGroupLabel(group)));

		if (order.speed != "")
			ApplySpeed(group, order.speed);

		if (order.mode != "")
			ApplyCombatMode(group, order.mode);
		else
			ApplyCombatMode(group, "fire_at_will");

		ApplyFormation(group, order.formation);
	}

	protected void ExecuteSetCombatMode(notnull SCR_AIGroup group, notnull OA_Order order)
	{
		ApplyCombatMode(group, order.mode);
	}

	protected void ExecuteSetSpeed(notnull SCR_AIGroup group, notnull OA_Order order)
	{
		ApplySpeed(group, order.speed);
	}

	protected void ExecuteHold(notnull SCR_AIGroup group, notnull OA_Order order)
	{
		ClearGroupWaypoints(group);

		vector holdPos = group.GetOrigin();

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = holdPos;

		Resource wpRes = Resource.Load("{93291E72AC23930F}Prefabs/AI/Waypoints/AIWaypoint_Defend.et");
		if (!wpRes.IsValid())
			return;

		SCR_DefendWaypoint wp = SCR_DefendWaypoint.Cast(GetGame().SpawnEntityPrefab(wpRes, GetGame().GetWorld(), params));
		if (!wp)
			return;

		wp.SetCompletionRadius(30);

		float holdTime = order.duration;
		if (holdTime <= 0)
			holdTime = 90;
		wp.SetHoldingTime(holdTime);

		group.AddWaypoint(wp);

		if (order.formation != "")
			ApplyFormation(group, order.formation);
		else
			ApplyFormation(group, "Line");
	}

	protected void ApplyCombatMode(notnull SCR_AIGroup group, string mode)
	{
		SCR_AIGroupUtilityComponent utility = group.GetGroupUtilityComponent();
		if (!utility)
			return;

		EAIGroupCombatMode cm = _ParseCombatMode(mode);
		utility.SetCombatMode(cm);
		OA_Log.Log(string.Format("[OA]   combat_mode %1 -> %2", GetGroupLabel(group), mode));
	}

	protected static EAIGroupCombatMode _ParseCombatMode(string mode)
	{
		if (mode == "stealth" || mode == "passive" || mode == "hold_fire")
			return EAIGroupCombatMode.HOLD_FIRE;
		return EAIGroupCombatMode.FIRE_AT_WILL;
	}

	protected void ApplySpeed(notnull SCR_AIGroup group, string speed)
	{
		EMovementType moveType = EMovementType.RUN;

		if (speed == "walk")
			moveType = EMovementType.WALK;
		else if (speed == "jog")
			moveType = EMovementType.RUN;
		else if (speed == "sprint")
			moveType = EMovementType.SPRINT;

		AIGroupMovementComponent moveComp = AIGroupMovementComponent.Cast(group.FindComponent(AIGroupMovementComponent));
		if (moveComp)
			moveComp.SetGroupCharactersWantedMovementType(moveType);
	}

	protected void ApplyFormation(notnull SCR_AIGroup group, string formation)
	{
		if (formation == "")
			formation = "StaggeredColumn";

		SCR_AIGroupMovementComponent moveComp = SCR_AIGroupMovementComponent.Cast(
			group.FindComponent(SCR_AIGroupMovementComponent));
		if (!moveComp)
			return;

		bool ok = moveComp.SetFormationDefinition(0, formation);
		OA_Log.Log(string.Format("[OA]   formation %1 -> %2 (ok=%3)",
			GetGroupLabel(group), formation, ok));
	}

	int GetGroupCount()
	{
		return m_mGroups.Count();
	}
}
