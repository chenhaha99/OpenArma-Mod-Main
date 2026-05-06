// OA_DataStructs.c — Data structures for OpenArma <-> backend JSON communication.
// Uses JsonApiStruct for serialization (RegV in constructor).

class OA_SideConfig : JsonApiStruct
{
	string faction;
	string control;

	void OA_SideConfig()
	{
		faction = "US";
		control = "llm";
		RegV("faction");
		RegV("control");
	}
}

class OA_EnemyData : JsonApiStruct
{
	string entity_id;
	ref array<float> position;
	ref array<float> detected_position;
	float distance;
	float time_since_seen;
	float time_since_detected;
	float time_since_side_recognized;
	float time_since_type_recognized;
	float time_since_endangered;
	string unit_type;
	bool is_disarmed;
	float trace_fraction;
	string perceived_faction;

	void OA_EnemyData()
	{
		position = {};
		detected_position = {};
		RegV("entity_id");
		RegV("position");
		RegV("detected_position");
		RegV("distance");
		RegV("time_since_seen");
		RegV("time_since_detected");
		RegV("time_since_side_recognized");
		RegV("time_since_type_recognized");
		RegV("time_since_endangered");
		RegV("unit_type");
		RegV("is_disarmed");
		RegV("trace_fraction");
		RegV("perceived_faction");
	}
}

class OA_EnvData : JsonApiStruct
{
	float elevation;
	float elevation_north;
	float elevation_south;
	float elevation_east;
	float elevation_west;

	void OA_EnvData()
	{
		RegV("elevation");
		RegV("elevation_north");
		RegV("elevation_south");
		RegV("elevation_east");
		RegV("elevation_west");
	}
}

class OA_UnitData : JsonApiStruct
{
	string entity_id;
	string group_id;
	string faction;
	ref array<float> position;
	string life_state;
	float health;
	string weapon;
	int ammo;
	bool in_vehicle;
	string stance;
	string name;
	string surname;
	string alias;
	string full_name;
	int age;
	string blood_type;
	string bio;

	void OA_UnitData()
	{
		position = {};
		life_state = "alive";
		health = 1.0;
		weapon = "";
		ammo = 0;
		in_vehicle = false;
		stance = "relaxed";
		name = "";
		surname = "";
		alias = "";
		full_name = "";
		age = 0;
		blood_type = "";
		bio = "";
		RegV("entity_id");
		RegV("group_id");
		RegV("faction");
		RegV("position");
		RegV("life_state");
		RegV("health");
		RegV("weapon");
		RegV("ammo");
		RegV("in_vehicle");
		RegV("stance");
		RegV("name");
		RegV("surname");
		RegV("alias");
		RegV("full_name");
		RegV("age");
		RegV("blood_type");
		RegV("bio");
	}
}

class OA_VehicleData : JsonApiStruct
{
	string entity_id;
	ref array<float> position;
	string faction;
	string type;
	float health;
	bool is_occupied;
	string pilot_id;
	int crew_count;

	void OA_VehicleData()
	{
		position = {};
		type = "unknown";
		health = 1.0;
		is_occupied = false;
		pilot_id = "";
		crew_count = 0;
		RegV("entity_id");
		RegV("position");
		RegV("faction");
		RegV("type");
		RegV("health");
		RegV("is_occupied");
		RegV("pilot_id");
		RegV("crew_count");
	}
}

class OA_BattleEvent : JsonApiStruct
{
	string type;
	string victim_id;
	string victim_name;
	string victim_faction;
	string killer_id;
	string killer_name;
	string killer_faction;
	ref array<float> position;
	int timestamp;
	string weapon;
	string relation;

	void OA_BattleEvent()
	{
		position = {};
		timestamp = 0;
		victim_name = "";
		killer_name = "";
		weapon = "";
		relation = "unknown";
		RegV("type");
		RegV("victim_id");
		RegV("victim_name");
		RegV("victim_faction");
		RegV("killer_id");
		RegV("killer_name");
		RegV("killer_faction");
		RegV("position");
		RegV("timestamp");
		RegV("weapon");
		RegV("relation");
	}
}

class OA_MarkerData : JsonApiStruct
{
	int marker_id;
	string type;
	ref array<float> position;
	string text;
	int faction_flags;
	int rotation;

	void OA_MarkerData()
	{
		position = {};
		text = "";
		faction_flags = 0;
		rotation = 0;
		RegV("marker_id");
		RegV("type");
		RegV("position");
		RegV("text");
		RegV("faction_flags");
		RegV("rotation");
	}
}

class OA_GroupData : JsonApiStruct
{
	string id;
	string label;
	string description;
	string faction;
	string control;
	string role;
	ref array<float> position;
	int member_count;
	int casualties;
	string current_waypoint_type;
	string combat_mode;
	string speed_mode;
	string formation;
	bool in_vehicle;
	string leader_stance;
	ref array<ref OA_EnemyData> known_enemies;
	ref OA_EnvData environment;

	void OA_GroupData()
	{
		position = {};
		known_enemies = {};
		environment = new OA_EnvData();
		RegV("id");
		RegV("label");
		RegV("description");
		RegV("faction");
		RegV("control");
		RegV("role");
		RegV("position");
		RegV("member_count");
		RegV("casualties");
		RegV("current_waypoint_type");
		RegV("combat_mode");
		RegV("speed_mode");
		RegV("formation");
		RegV("in_vehicle");
		RegV("leader_stance");
		RegV("known_enemies");
		RegV("environment");
	}
}

class OA_HumanMessage : JsonApiStruct
{
	string sender;
	string text;
	string time;

	void OA_HumanMessage()
	{
		RegV("sender");
		RegV("text");
		RegV("time");
	}
}

class OA_GameState : JsonApiStruct
{
	string game_mode;
	string game_time;

	void OA_GameState()
	{
		RegV("game_mode");
		RegV("game_time");
	}
}

class OA_SituationReport : JsonApiStruct
{
	int request_id;
	int timestamp;
	string conversation_id;
	string priority;
	ref OA_GameState game_state;
	ref array<ref OA_GroupData> groups;
	ref array<ref OA_UnitData> units;
	ref array<ref OA_VehicleData> vehicles;
	ref array<ref OA_BattleEvent> events;
	ref array<ref OA_MarkerData> markers;
	ref array<ref OA_HumanMessage> human_messages;

	void OA_SituationReport()
	{
		game_state = new OA_GameState();
		groups = {};
		units = {};
		vehicles = {};
		events = {};
		markers = {};
		human_messages = {};
		RegV("request_id");
		RegV("timestamp");
		RegV("conversation_id");
		RegV("priority");
		RegV("game_state");
		RegV("groups");
		RegV("units");
		RegV("vehicles");
		RegV("events");
		RegV("markers");
		RegV("human_messages");
	}
}

class OA_Waypoint : JsonApiStruct
{
	ref array<float> pos;

	void OA_Waypoint()
	{
		pos = {};
		RegV("pos");
	}
}

class OA_Order : JsonApiStruct
{
	string type;
	string group_id;
	ref array<float> target;
	string speed;
	string mode;
	float duration;
	bool use_turrets;
	int rounds;
	string vehicle_id;
	bool cycle;
	string formation;
	ref array<ref OA_Waypoint> waypoints;

	void OA_Order()
	{
		target = {};
		waypoints = {};
		RegV("type");
		RegV("group_id");
		RegV("target");
		RegV("speed");
		RegV("mode");
		RegV("duration");
		RegV("use_turrets");
		RegV("rounds");
		RegV("vehicle_id");
		RegV("cycle");
		RegV("formation");
		RegV("waypoints");
	}
}

class OA_PendingOrders : JsonApiStruct
{
	int request_id;
	ref array<ref OA_Order> orders;
	string briefing;
	string assessment;
	ref array<string> priority_targets;

	void OA_PendingOrders()
	{
		orders = {};
		priority_targets = {};
		RegV("request_id");
		RegV("orders");
		RegV("briefing");
		RegV("assessment");
		RegV("priority_targets");
	}
}

class OA_WebMessage : JsonApiStruct
{
	string sender;
	string text;
	string time;

	void OA_WebMessage()
	{
		RegV("sender");
		RegV("text");
		RegV("time");
	}
}

class OA_ArmaConfigBlock : JsonApiStruct
{
	bool running;
	float decision_interval;
	ref array<ref OA_SideConfig> sides;
	int max_squads;
	bool emergency_enabled;
	string language;

	void OA_ArmaConfigBlock()
	{
		running = false;
		decision_interval = 30.0;
		sides = {};
		max_squads = 12;
		emergency_enabled = true;
		language = "en";

		RegV("running");
		RegV("decision_interval");
		RegV("sides");
		RegV("max_squads");
		RegV("emergency_enabled");
		RegV("language");
	}
}

class OA_CommandResponse : JsonApiStruct
{
	bool ack;
	int request_id;
	string status;
	ref OA_PendingOrders pending_orders;
	ref OA_ArmaConfigBlock config;
	ref array<ref OA_WebMessage> web_messages;
	string conversation_id;
	string error;

	void OA_CommandResponse()
	{
		pending_orders = new OA_PendingOrders();
		config = new OA_ArmaConfigBlock();
		web_messages = {};
		RegV("ack");
		RegV("request_id");
		RegV("status");
		RegV("pending_orders");
		RegV("config");
		RegV("web_messages");
		RegV("conversation_id");
		RegV("error");
	}
}
