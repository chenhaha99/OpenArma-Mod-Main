// OA_EventTracker.c — Event-driven battle event collection.
// Registers global listeners for kills, damage state changes, etc.
// Events are buffered and flushed each heartbeat.

class OA_EventTracker
{
	protected ref array<ref OA_BattleEvent> m_aEvents;
	protected bool m_bInitialized;

	void OA_EventTracker()
	{
		m_aEvents = {};
		m_bInitialized = false;
	}

	void Init()
	{
		if (m_bInitialized)
			return;

		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gameMode)
		{
			OA_Log.Log("[OA_EventTracker] No game mode found, events disabled");
			return;
		}

		gameMode.GetOnControllableDestroyed().Insert(OnControllableDestroyed);
		m_bInitialized = true;
		OA_Log.Log("[OA_EventTracker] Initialized — listening for kill events");
	}

	void Cleanup()
	{
		if (!m_bInitialized)
			return;

		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gameMode)
			gameMode.GetOnControllableDestroyed().Remove(OnControllableDestroyed);

		m_bInitialized = false;
		m_aEvents.Clear();
	}

	void OnControllableDestroyed(notnull SCR_InstigatorContextData ctx)
	{
		OA_BattleEvent evt = new OA_BattleEvent();
		evt.type = "kill";
		evt.timestamp = System.GetUnixTime();

		IEntity victim = ctx.GetVictimEntity();
		IEntity killer = ctx.GetKillerEntity();

		if (victim)
		{
			evt.victim_id = victim.GetID().ToString();
			vector vPos = victim.GetOrigin();
			evt.position = {vPos[0], vPos[1], vPos[2]};
			evt.victim_faction = _GetEntityFaction(victim);
			evt.victim_name = _GetEntityName(victim);
		}

		if (killer)
		{
			evt.killer_id = killer.GetID().ToString();
			evt.killer_faction = _GetEntityFaction(killer);
			evt.killer_name = _GetEntityName(killer);
			evt.weapon = _GetEntityWeapon(killer);
		}
		else
		{
			Instigator instigator = ctx.GetInstigator();
			if (instigator)
			{
				IEntity instigatorEntity = instigator.GetInstigatorEntity();
				if (instigatorEntity)
				{
					evt.killer_id = instigatorEntity.GetID().ToString();
					evt.killer_faction = _GetEntityFaction(instigatorEntity);
					evt.killer_name = _GetEntityName(instigatorEntity);
					evt.weapon = _GetEntityWeapon(instigatorEntity);
				}
			}
		}

		if (evt.victim_faction == evt.killer_faction && evt.killer_faction != "")
			evt.relation = "friendly_fire";
		else if (evt.killer_faction != "")
			evt.relation = "enemy";
		else
			evt.relation = "unknown";

		m_aEvents.Insert(evt);
		OA_Log.Log(string.Format("[OA_EventTracker] KILL: %1[%2] killed %3[%4] w/%5 (%6, pending=%7)",
			evt.killer_name, evt.killer_faction, evt.victim_name, evt.victim_faction,
			evt.weapon, evt.relation, m_aEvents.Count()));
	}

	array<ref OA_BattleEvent> Flush()
	{
		array<ref OA_BattleEvent> result = {};
		foreach (OA_BattleEvent evt : m_aEvents)
		{
			result.Insert(evt);
		}
		m_aEvents.Clear();
		return result;
	}

	int GetPendingCount()
	{
		return m_aEvents.Count();
	}

	protected string _GetEntityFaction(IEntity entity)
	{
		if (!entity)
			return "";

		SCR_AIGroup group = _FindGroupForEntity(entity);
		if (group)
		{
			Faction fac = group.GetFaction();
			if (fac)
				return fac.GetFactionKey();
		}

		FactionAffiliationComponent facComp = FactionAffiliationComponent.Cast(
			entity.FindComponent(FactionAffiliationComponent));
		if (facComp)
		{
			Faction fac = facComp.GetAffiliatedFaction();
			if (fac)
				return fac.GetFactionKey();
		}

		return "";
	}

	protected string _GetEntityName(IEntity entity)
	{
		if (!entity)
			return "";

		SCR_CharacterIdentityComponent idComp = SCR_CharacterIdentityComponent.Cast(
			entity.FindComponent(SCR_CharacterIdentityComponent));
		if (!idComp)
			return "";

		Identity identity = idComp.GetIdentity();
		if (!identity)
			return "";

		string n = OA_Log.StripLocKey(identity.GetName());
		string s = OA_Log.StripLocKey(identity.GetSurname());
		if (n != "" || s != "")
			return n + " " + s;

		return "";
	}

	protected string _GetEntityWeapon(IEntity entity)
	{
		if (!entity)
			return "";

		BaseWeaponManagerComponent weapMgr = BaseWeaponManagerComponent.Cast(
			entity.FindComponent(BaseWeaponManagerComponent));
		if (!weapMgr)
			return "";

		BaseWeaponComponent weapon = weapMgr.GetCurrentWeapon();
		if (!weapon)
			return "";

		UIInfo uiInfo = weapon.GetUIInfo();
		if (uiInfo)
		{
			string wName = uiInfo.GetName();
			if (wName != "")
				return OA_Log.StripWeaponLocKey(wName);
		}

		return "";
	}

	protected SCR_AIGroup _FindGroupForEntity(IEntity entity)
	{
		if (!entity)
			return null;

		AIControlComponent aiCtrl = AIControlComponent.Cast(entity.FindComponent(AIControlComponent));
		if (!aiCtrl)
			return null;

		AIAgent agent = aiCtrl.GetAIAgent();
		if (!agent)
			return null;

		AIGroup parentGroup = agent.GetParentGroup();
		return SCR_AIGroup.Cast(parentGroup);
	}
}
