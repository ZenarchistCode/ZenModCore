class ZenKitBase: DeployableContainer_Base
{
	override bool ShouldZenHologram()
	{
		return true;
	}
	
	vector GetZenDeployPositionOffset() { return vector.Zero; }
	vector GetZenDeployOrientationOffset() { return vector.Zero; }

	bool GetZenHoloShouldIgnoreCollision()
	{
		return ConfigGetBool("zenIgnoreCollision");
	}

	override void OnPlacementComplete(Man player, vector position = "0 0 0", vector orientation = "0 0 0")
	{
		super.OnPlacementComplete(player, position, orientation);

		if (!g_Game.IsDedicatedServer() || position == vector.Zero)
			return;

		string deployedItemType = "";
		string configPathProjectionTypename = string.Format("CfgVehicles %1 projectionTypename", GetType());
		if (g_Game.ConfigIsExisting(configPathProjectionTypename))
		{
			deployedItemType = g_Game.ConfigGetTextOut(configPathProjectionTypename);
		}

		if (deployedItemType == "")
			return;

		PlayerBase pb = PlayerBase.Cast(player);
		if (!pb)
			return;

		ItemBase deployedItem = ItemBase.Cast(g_Game.CreateObject(deployedItemType, pb.GetLocalProjectionPosition(), false));
		if (!deployedItem)
		{
			Error("ZenKitBoxBase - failed to deploy classname: " + deployedItemType);
			return;
		}

		SetIsDeploySound(true);
		deployedItem.SetPosition(position);
		deployedItem.SetOrientation(orientation);
		DeleteSafe();
	}

	override bool IsBasebuildingKit()
	{
		return true;
	}

	override bool IsDeployable()
	{
		return true;
	}

	override void SetActions()
	{
		super.SetActions();

		AddAction(ActionTogglePlaceObject);
		AddAction(ActionPlaceObject);
	}
}

class ZenKitBoxBase : ZenKitBase {};