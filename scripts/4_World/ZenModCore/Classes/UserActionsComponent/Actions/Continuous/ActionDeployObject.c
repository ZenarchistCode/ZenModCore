modded class ActionDeployObject 
{
	override bool Post_SetupAction(ActionData action_data)
	{
		#ifndef SERVER 
		// If the player places a hologram at vector.Zero, it actually places the object there and deletes the kit.
		// So without this check, the player loses their item and you have a bunch of objects spawned at 0 0 0 coords on your server.
		if (GetZenCoreConfig().ZenCore_GeneralConfig.PreventHologramPlacementAt000)
		{
			Hologram h = action_data.m_Player.GetHologramLocal();
	
			if (h && h.GetProjectionPosition() == vector.Zero)
			{
				//ZenFunctions.ZenClientMessage("You cannot place an item while its hologram is invisible.");
				return false;
			}
		}
		#endif

		return super.Post_SetupAction(action_data);
	}
}