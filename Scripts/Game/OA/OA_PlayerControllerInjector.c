// OA_PlayerControllerInjector.c — Modded PlayerController for OA RPC.
// Each player has their own PlayerController with RplComponent.
// Client sends RPC to server via PlayerController, server forwards to OA_Main via GameMode.

modded class SCR_PlayerController
{
	protected bool OA_IsAuthority()
	{
		RplComponent rpl = RplComponent.Cast(FindComponent(RplComponent));
		if (!rpl)
			return true;

		return !rpl.IsProxy();
	}

	void OA_RequestLogin(string apiKey, string urlOverride)
	{
		if (OA_IsAuthority())
		{
			OA_OnServerLogin(apiKey, urlOverride);
			return;
		}

		Rpc(OA_RpcServerLogin, apiKey, urlOverride);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void OA_RpcServerLogin(string apiKey, string urlOverride)
	{
		OA_Log.Log(string.Format("[OA] RPC received: OA_RpcServerLogin"));
		OA_OnServerLogin(apiKey, urlOverride);
	}

	protected void OA_OnServerLogin(string apiKey, string urlOverride)
	{
		SCR_BaseGameMode gm = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gm)
		{
			OA_Log.Log("[OA] Server: GameMode not found");
			return;
		}

		gm.OA_HandleLogin(apiKey, urlOverride);
	}

	void OA_RequestLogout()
	{
		if (OA_IsAuthority())
		{
			OA_OnServerLogout();
			return;
		}

		Rpc(OA_RpcServerLogout);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void OA_RpcServerLogout()
	{
		OA_OnServerLogout();
	}

	protected void OA_OnServerLogout()
	{
		SCR_BaseGameMode gm = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gm)
			return;

		gm.OA_HandleLogout();
	}

	void OA_RequestChat(string sender, string text)
	{
		if (OA_IsAuthority())
		{
			OA_OnServerChat(sender, text);
			return;
		}

		Rpc(OA_RpcServerChat, sender, text);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void OA_RpcServerChat(string sender, string text)
	{
		OA_OnServerChat(sender, text);
	}

	protected void OA_OnServerChat(string sender, string text)
	{
		SCR_BaseGameMode gm = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gm)
			return;

		gm.OA_HandleChat(sender, text);
	}
}
