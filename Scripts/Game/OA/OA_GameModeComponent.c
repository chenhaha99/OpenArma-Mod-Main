// OA_GameModeComponent.c — ScriptComponent attached to the GameMode entity.
// Hooks OA_Main into EOnFrame for periodic situation scanning.

[ComponentEditorProps(category: "OA", description: "OpenArma AI Commander integration")]
class OA_GameModeComponentClass : SCR_BaseGameModeComponentClass
{
}

class OA_GameModeComponent : SCR_BaseGameModeComponent
{
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		// OA_GameModeInjector (modded SCR_BaseGameMode) handles initialization automatically.
		// This component is kept for backward compatibility but no longer drives the tick loop.
	}
}
