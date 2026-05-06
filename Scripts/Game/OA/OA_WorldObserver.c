// OA_WorldObserver.c — Scans the game world and builds situation report JSON.
// Supports N-sided dynamic faction config via OA_SideConfig array.

class OA_WorldObserver
{
	protected float m_fScanTimer;
	protected int m_iRequestId;
	protected ref array<ref OA_HumanMessage> m_aMessageQueue;
	protected ref map<string, int> m_mInitialGroupSize;
	protected ref array<ref OA_VehicleData> m_aFoundVehicles;
	protected ref set<string> m_sScannedFactions;

	void OA_WorldObserver()
	{
		m_fScanTimer = 0;
		m_iRequestId = 0;
		m_aMessageQueue = {};
		m_mInitialGroupSize = new map<string, int>();
	}

	void Reset()
	{
		m_fScanTimer = 0;
		m_aMessageQueue.Clear();
		m_mInitialGroupSize.Clear();
	}

	void AddHumanMessage(string sender, string text)
	{
		OA_HumanMessage msg = new OA_HumanMessage();
		msg.sender = sender;
		msg.text = text;

		int hour, minute, second;
		System.GetHourMinuteSecondUTC(hour, minute, second);
		msg.time = string.Format("%1:%2", hour.ToString(2), minute.ToString(2));

		m_aMessageQueue.Insert(msg);
	}

	bool ShouldScan(float dt, float interval)
	{
		m_fScanTimer += dt;
		if (m_fScanTimer >= interval)
		{
			m_fScanTimer = 0;
			return true;
		}
		return false;
	}

	string BuildSituationReport(OA_Config config, string conversationId,
		array<ref OA_SideConfig> sides, array<ref OA_BattleEvent> battleEvents = null)
	{
		m_iRequestId++;

		OA_SituationReport report = new OA_SituationReport();
		report.request_id = m_iRequestId;
		report.timestamp = System.GetUnixTime();
		report.conversation_id = conversationId;
		report.priority = "normal";

		report.game_state.game_mode = config.GameMode;
		report.game_state.game_time = GetGameTimeString();

		ref map<string, ref OA_SideConfig> factionMap = BuildFactionMap(sides);

		ScanGroups(report.groups, config.MaxSquads, factionMap);
		ScanUnits(report.units, factionMap);
		ScanVehicles(report.vehicles, factionMap, sides);
		ScanMarkers(report.markers);

		if (battleEvents)
		{
			foreach (OA_BattleEvent evt : battleEvents)
				report.events.Insert(evt);
		}

		foreach (OA_HumanMessage msg : m_aMessageQueue)
		{
			report.human_messages.Insert(msg);
		}
		m_aMessageQueue.Clear();

		string json;
		report.Pack();
		json = report.AsString();
		return json;
	}

	int GetRequestId()
	{
		return m_iRequestId;
	}

	protected ref map<string, ref OA_SideConfig> BuildFactionMap(array<ref OA_SideConfig> sides)
	{
		ref map<string, ref OA_SideConfig> result = new map<string, ref OA_SideConfig>();
		if (!sides)
			return result;

		foreach (OA_SideConfig side : sides)
		{
			if (side && side.faction != "")
				result.Set(side.faction, side);
		}
		return result;
	}

	protected string GetGameTimeString()
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return "unknown";

		TimeAndWeatherManagerEntity timeManager = world.GetTimeAndWeatherManager();
		if (!timeManager)
			return "unknown";

		TimeContainer tc = timeManager.GetTime();
		return string.Format("%1:%2", tc.m_iHours.ToString(2), tc.m_iMinutes.ToString(2));
	}

	protected Faction ResolveFaction(string factionKey)
	{
		FactionManager fm = GetGame().GetFactionManager();
		if (!fm)
			return null;

		return fm.GetFactionByKey(factionKey);
	}

	protected bool IsGroupRelevant(notnull SCR_AIGroup group,
		map<string, ref OA_SideConfig> factionMap)
	{
		if (!factionMap || factionMap.Count() == 0)
			return true;

		Faction groupFaction = group.GetFaction();
		if (!groupFaction)
			return true;

		return factionMap.Contains(groupFaction.GetFactionKey());
	}

	protected string GetGroupControl(notnull SCR_AIGroup group,
		map<string, ref OA_SideConfig> factionMap)
	{
		Faction groupFaction = group.GetFaction();
		if (!groupFaction)
			return "ai";

		string key = groupFaction.GetFactionKey();
		if (factionMap.Contains(key))
		{
			OA_SideConfig side = factionMap.Get(key);
			return side.control;
		}

		return "ai";
	}

	protected string GetGroupFactionKey(notnull SCR_AIGroup group)
	{
		Faction groupFaction = group.GetFaction();
		if (!groupFaction)
			return "unknown";

		return groupFaction.GetFactionKey();
	}

	protected void ScanGroups(notnull array<ref OA_GroupData> outGroups, int maxSquads,
		map<string, ref OA_SideConfig> factionMap)
	{
		array<AIAgent> agents = {};
		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld)
			return;

		aiWorld.GetAIAgents(agents);

		foreach (AIAgent agent : agents)
		{
			SCR_AIGroup group = SCR_AIGroup.Cast(agent);
			if (!group)
				continue;

			OA_GroupData data = new OA_GroupData();
			FillGroupData(group, data);
			data.faction = GetGroupFactionKey(group);
			data.control = GetGroupControl(group, factionMap);
			outGroups.Insert(data);
		}
	}

	protected void ScanUnits(notnull array<ref OA_UnitData> outUnits,
		map<string, ref OA_SideConfig> factionMap)
	{
		array<AIAgent> agents = {};
		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld)
			return;

		aiWorld.GetAIAgents(agents);

		foreach (AIAgent agent : agents)
		{
			SCR_AIGroup group = SCR_AIGroup.Cast(agent);
			if (!group)
				continue;

			string groupId = group.GetID().ToString();
			string factionKey = GetGroupFactionKey(group);

			array<AIAgent> members = {};
			group.GetAgents(members);

			foreach (AIAgent member : members)
			{
				IEntity entity = member.GetControlledEntity();
				if (!entity)
					continue;

				OA_UnitData unit = new OA_UnitData();
				unit.entity_id = entity.GetID().ToString();
				unit.group_id = groupId;
				unit.faction = factionKey;

				vector pos = entity.GetOrigin();
				unit.position = {pos[0], pos[1], pos[2]};

				CharacterControllerComponent charCtrl = CharacterControllerComponent.Cast(
					entity.FindComponent(CharacterControllerComponent));
				if (charCtrl)
				{
					ECharacterLifeState ls = charCtrl.GetLifeState();
					if (ls == ECharacterLifeState.ALIVE)
						unit.life_state = "alive";
					else if (ls == ECharacterLifeState.INCAPACITATED)
						unit.life_state = "incapacitated";
					else
						unit.life_state = "dead";

					if (charCtrl.IsWeaponRaised())
						unit.stance = "combat";
					else
						unit.stance = "relaxed";
				}

				SCR_DamageManagerComponent dmg = SCR_DamageManagerComponent.Cast(
					entity.FindComponent(SCR_DamageManagerComponent));
				if (dmg)
					unit.health = dmg.GetHealthScaled();

				BaseWeaponManagerComponent weaponMgr = BaseWeaponManagerComponent.Cast(
					entity.FindComponent(BaseWeaponManagerComponent));
				if (weaponMgr)
				{
					BaseWeaponComponent currentWeapon = weaponMgr.GetCurrentWeapon();
					if (currentWeapon)
					{
						UIInfo weaponInfo = currentWeapon.GetUIInfo();
						if (weaponInfo)
							unit.weapon = OA_Log.StripWeaponLocKey(weaponInfo.GetName());

						BaseMuzzleComponent muzzle = currentWeapon.GetCurrentMuzzle();
						if (muzzle)
						{
							BaseMagazineComponent mag = muzzle.GetMagazine();
							if (mag)
								unit.ammo = mag.GetAmmoCount();
						}
					}
				}

				CompartmentAccessComponent compAccess = CompartmentAccessComponent.Cast(
					entity.FindComponent(CompartmentAccessComponent));
				unit.in_vehicle = (compAccess && compAccess.IsInCompartment());

				_FillIdentity(entity, unit);

				outUnits.Insert(unit);
			}
		}
	}

	protected void _FillIdentity(IEntity entity, notnull OA_UnitData unit)
	{
		SCR_CharacterIdentityComponent idComp = SCR_CharacterIdentityComponent.Cast(
			entity.FindComponent(SCR_CharacterIdentityComponent));
		if (idComp)
		{
			Identity identity = idComp.GetIdentity();
			if (identity)
			{
				unit.name = OA_Log.StripLocKey(identity.GetName());
				unit.surname = OA_Log.StripLocKey(identity.GetSurname());
				unit.alias = OA_Log.StripLocKey(identity.GetAlias());
				unit.full_name = unit.name + " " + unit.surname;
			}
		}

		SCR_ExtendedCharacterIdentityComponent extComp = SCR_ExtendedCharacterIdentityComponent.Cast(
			entity.FindComponent(SCR_ExtendedCharacterIdentityComponent));
		if (extComp)
		{
			SCR_ExtendedIdentity extId = extComp.GetExtendedIdentity();
			if (extId)
			{
				unit.age = extId.GetAge();
			}

			SCR_ExtendedCharacterIdentity charId = SCR_ExtendedCharacterIdentity.Cast(extId);
			if (charId)
			{
				SCR_EBloodType bt = charId.GetBloodType();
				string btStr = typename.EnumToString(SCR_EBloodType, bt);
				if (btStr != "")
					unit.blood_type = btStr;
				else
					unit.blood_type = bt.ToString();
			}

			SCR_IdentityBio identityBio = extComp.GetIdentityBio();
			if (identityBio)
				unit.bio = identityBio.GetBioText();
		}
	}

	protected void ScanVehicles(notnull array<ref OA_VehicleData> outVehicles,
		map<string, ref OA_SideConfig> factionMap, array<ref OA_SideConfig> sides)
	{
		m_aFoundVehicles = new array<ref OA_VehicleData>();
		m_sScannedFactions = null;

		GetGame().GetWorld().QueryEntitiesBySphere(
			"0 0 0", 999999, _VehicleQueryCallback, _VehicleFilterCallback, EQueryEntitiesFlags.ALL);

		foreach (OA_VehicleData v : m_aFoundVehicles)
			outVehicles.Insert(v);

		m_aFoundVehicles = null;
	}

	protected bool _VehicleFilterCallback(IEntity entity)
	{
		if (!entity)
			return false;

		BaseVehicleControllerComponent vehCtrl = BaseVehicleControllerComponent.Cast(
			entity.FindComponent(BaseVehicleControllerComponent));
		return (vehCtrl != null);
	}

	protected bool _VehicleQueryCallback(IEntity entity)
	{
		if (!entity)
			return true;

		OA_VehicleData veh = new OA_VehicleData();
		veh.entity_id = entity.GetID().ToString();

		vector vPos = entity.GetOrigin();
		veh.position = {vPos[0], vPos[1], vPos[2]};

		FactionAffiliationComponent facComp = FactionAffiliationComponent.Cast(
			entity.FindComponent(FactionAffiliationComponent));
		if (facComp)
		{
			Faction fac = facComp.GetAffiliatedFaction();
			if (fac)
				veh.faction = fac.GetFactionKey();
		}

		if (m_sScannedFactions && m_sScannedFactions.Count() > 0 && veh.faction != "")
		{
			if (!m_sScannedFactions.Contains(veh.faction))
				return true;
		}

		EntityPrefabData prefabData = entity.GetPrefabData();
		if (prefabData)
			veh.type = prefabData.GetPrefabName();

		SCR_DamageManagerComponent vDmg = SCR_DamageManagerComponent.Cast(
			entity.FindComponent(SCR_DamageManagerComponent));
		if (vDmg)
			veh.health = vDmg.GetHealthScaled();

		SCR_BaseCompartmentManagerComponent compMgr = SCR_BaseCompartmentManagerComponent.Cast(
			entity.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (compMgr)
		{
			veh.crew_count = compMgr.GetOccupantCount();
			veh.is_occupied = (veh.crew_count > 0);
		}
		else
		{
			veh.is_occupied = false;
			veh.crew_count = 0;
		}

		BaseVehicleControllerComponent vehCtrl = BaseVehicleControllerComponent.Cast(
			entity.FindComponent(BaseVehicleControllerComponent));
		if (vehCtrl)
		{
			PilotCompartmentSlot pilotSlot = vehCtrl.GetPilotCompartmentSlot();
			if (pilotSlot && pilotSlot.IsOccupied())
			{
				IEntity pilot = pilotSlot.GetOccupant();
				if (pilot)
					veh.pilot_id = pilot.GetID().ToString();
			}
		}

		m_aFoundVehicles.Insert(veh);
		return true;
	}

	protected void ScanMarkers(notnull array<ref OA_MarkerData> outMarkers)
	{
		SCR_MapMarkerManagerComponent markerMgr = SCR_MapMarkerManagerComponent.Cast(
			GetGame().GetGameMode().FindComponent(SCR_MapMarkerManagerComponent));
		if (!markerMgr)
			return;

		array<SCR_MapMarkerBase> markers = markerMgr.GetStaticMarkers();
		if (!markers)
			return;

		foreach (SCR_MapMarkerBase marker : markers)
		{
			if (!marker)
				continue;

			OA_MarkerData md = new OA_MarkerData();
			md.marker_id = marker.GetMarkerID();

			SCR_EMapMarkerType mType = marker.GetType();
			md.type = typename.EnumToString(SCR_EMapMarkerType, mType);

			int mPos[2];
			marker.GetWorldPos(mPos);
			md.position = {mPos[0], 0, mPos[1]};
			md.text = marker.GetCustomText();
			md.rotation = marker.GetRotation();

			outMarkers.Insert(md);
		}
	}

	protected void FillGroupData(notnull SCR_AIGroup group, notnull OA_GroupData data)
	{
		data.id = group.GetID().ToString();

		SCR_CallsignGroupComponent callsignComp = SCR_CallsignGroupComponent.Cast(group.FindComponent(SCR_CallsignGroupComponent));
		if (callsignComp)
		{
			int compIdx, platIdx, squadIdx;
			if (callsignComp.GetCallsignIndexes(compIdx, platIdx, squadIdx))
			{
				string compName = OA_CommandExecutor._PhoneticName(compIdx);
				data.label = string.Format("%1-%2-%3", compName, platIdx + 1, squadIdx + 1);
			}
		}

		SCR_EditableGroupComponent editable = SCR_EditableGroupComponent.Cast(group.FindComponent(SCR_EditableGroupComponent));
		if (editable)
		{
			SCR_UIInfo info = editable.GetInfo();
			if (info)
			{
				if (data.label.IsEmpty())
					data.label = info.GetName();
				string desc = info.GetDescription();
				if (!desc.IsEmpty())
					data.description = desc;
			}
		}

		if (data.label.IsEmpty())
			data.label = string.Format("Group_%1", data.id);

		data.role = "infantry";

		vector pos = group.GetOrigin();
		data.position = {pos[0], pos[1], pos[2]};

		array<AIAgent> members = {};
		group.GetAgents(members);
		data.member_count = members.Count();

		string groupId = data.id;
		if (!m_mInitialGroupSize.Contains(groupId))
			m_mInitialGroupSize.Set(groupId, members.Count());

		int initialSize = m_mInitialGroupSize.Get(groupId);
		data.casualties = Math.Max(0, initialSize - members.Count());

		data.current_waypoint_type = GetCurrentWaypointType(group);
		data.combat_mode = _GetCombatModeStr(group);

		SCR_AIGroupMovementComponent moveComp = SCR_AIGroupMovementComponent.Cast(group.FindComponent(SCR_AIGroupMovementComponent));
		if (moveComp)
		{
			EMovementType mt = moveComp.GetGroupCharactersMovementTypeWanted();
			if (mt == EMovementType.WALK)
				data.speed_mode = "walk";
			else if (mt == EMovementType.SPRINT)
				data.speed_mode = "sprint";
			else
				data.speed_mode = "run";

			AIFormationDefinition formDef = moveComp.GetFormationDefinition(0);
			if (formDef)
				data.formation = formDef.GetName();
			else
				data.formation = "unknown";
		}
		else
		{
			data.speed_mode = "run";
			data.formation = "unknown";
		}

		if (members.Count() > 0)
		{
			IEntity leaderEntity = members[0].GetControlledEntity();
			if (leaderEntity)
			{
				CompartmentAccessComponent compAccess = CompartmentAccessComponent.Cast(leaderEntity.FindComponent(CompartmentAccessComponent));
				data.in_vehicle = (compAccess && compAccess.IsInCompartment());

				CharacterControllerComponent charCtrl = CharacterControllerComponent.Cast(leaderEntity.FindComponent(CharacterControllerComponent));
				if (charCtrl)
				{
					if (charCtrl.IsWeaponRaised())
						data.leader_stance = "combat";
					else
						data.leader_stance = "relaxed";
				}
			}
		}

		ScanPerception(group, data.known_enemies);
		ScanEnvironment(pos, data.environment);

		EnforceCombatModeSafety(group, data);
	}

	protected void EnforceCombatModeSafety(notnull SCR_AIGroup group, notnull OA_GroupData data)
	{
		if (data.combat_mode != "hold_fire")
			return;

		static const float SELF_DEFENSE_RANGE = 100.0;

		foreach (OA_EnemyData enemy : data.known_enemies)
		{
			if (enemy.distance < SELF_DEFENSE_RANGE && enemy.time_since_seen < 10.0)
			{
				SCR_AIGroupUtilityComponent utility = group.GetGroupUtilityComponent();
				if (utility)
				{
					utility.SetCombatMode(EAIGroupCombatMode.FIRE_AT_WILL);
					data.combat_mode = "fire_at_will";
					OA_Log.Log(string.Format("[OA] SAFETY: %1 auto-switched to FIRE_AT_WILL (enemy at %2m)",
						data.label, enemy.distance));
				}
				return;
			}
		}
	}

	protected string GetCurrentWaypointType(notnull SCR_AIGroup group)
	{
		AIWaypoint wp = group.GetCurrentWaypoint();
		if (!wp)
			return "none";

		array<AIWaypoint> allWps = {};
		group.GetWaypoints(allWps);
		foreach (AIWaypoint w : allWps)
		{
			if (AIWaypointCycle.Cast(w))
				return "patrol";
		}

		if (SCR_DefendWaypoint.Cast(wp))
			return "defend";
		if (SCR_TimedWaypoint.Cast(wp))
			return "timed";

		return "move";
	}

	protected void ScanPerception(notnull SCR_AIGroup group, notnull array<ref OA_EnemyData> outEnemies)
	{
		array<AIAgent> members = {};
		group.GetAgents(members);

		set<IEntity> seenEntities = new set<IEntity>();

		foreach (AIAgent member : members)
		{
			IEntity entity = member.GetControlledEntity();
			if (!entity)
				continue;

			PerceptionComponent perception = PerceptionComponent.Cast(entity.FindComponent(PerceptionComponent));
			if (!perception)
				continue;

			array<BaseTarget> targets = {};
			perception.GetTargetsList(targets, ETargetCategory.ENEMY);

			foreach (BaseTarget target : targets)
			{
				IEntity targetEntity = target.GetTargetEntity();
				if (targetEntity && seenEntities.Contains(targetEntity))
					continue;
				if (targetEntity)
					seenEntities.Insert(targetEntity);

				OA_EnemyData enemy = new OA_EnemyData();

				if (targetEntity)
					enemy.entity_id = string.Format("%1", targetEntity.GetID());
				else
					enemy.entity_id = "";

				vector seenPos = target.GetLastSeenPosition();
				enemy.position = {seenPos[0], seenPos[1], seenPos[2]};

				vector detPos = target.GetLastDetectedPosition();
				enemy.detected_position = {detPos[0], detPos[1], detPos[2]};

				enemy.distance = target.GetDistance();
				enemy.time_since_seen = target.GetTimeSinceSeen();
				enemy.time_since_detected = target.GetTimeSinceDetected();
				enemy.time_since_side_recognized = target.GetTimeSinceSideRecognized();
				enemy.time_since_type_recognized = target.GetTimeSinceTypeRecognized();
				enemy.time_since_endangered = target.GetTimeSinceEndangered();
				enemy.unit_type = _UnitTypeStr(target.GetUnitType());
				enemy.is_disarmed = target.IsDisarmed();
				enemy.trace_fraction = target.GetTraceFraction();

				Faction faction = target.GetPerceivedFaction();
				if (faction)
					enemy.perceived_faction = faction.GetFactionKey();
				else
					enemy.perceived_faction = "unknown";

				outEnemies.Insert(enemy);
			}
		}
	}

	protected static string _UnitTypeStr(EAIUnitType t)
	{
		if (t == EAIUnitType.UnitType_Infantry)
			return "infantry";

		int v = t;
		return "unit_type_" + v.ToString();
	}

	protected static string _GetCombatModeStr(notnull SCR_AIGroup group)
	{
		SCR_AIGroupUtilityComponent utility = group.GetGroupUtilityComponent();
		if (!utility)
			return "unknown";

		EAIGroupCombatMode cm = utility.GetCombatModeActual();
		if (cm == EAIGroupCombatMode.HOLD_FIRE)
			return "hold_fire";
		if (cm == EAIGroupCombatMode.FIRE_AT_WILL)
			return "fire_at_will";
		return "unknown";
	}

	protected void ScanEnvironment(vector pos, notnull OA_EnvData env)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		float surfY = world.GetSurfaceY(pos[0], pos[2]);
		env.elevation = surfY;

		float offset = 100.0;
		env.elevation_north = world.GetSurfaceY(pos[0], pos[2] + offset);
		env.elevation_south = world.GetSurfaceY(pos[0], pos[2] - offset);
		env.elevation_east  = world.GetSurfaceY(pos[0] + offset, pos[2]);
		env.elevation_west  = world.GetSurfaceY(pos[0] - offset, pos[2]);
	}
}
