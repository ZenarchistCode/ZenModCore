/*
	(c) 2026 | ZenModCore | Zenarchist
*/

class CfgPatches
{
	class ZenModCore
	{
		requiredAddons[] =
		{
			"DZ_Data",
			"DZ_Scripts",
			"JM_CF_Scripts"
		};
	};
};

class CfgMods
{
	class ZenModCore
	{
		author = "Zenarchist";
		type = "mod";
		version = "1.0";
		storageVersion = 2; // CF storage
		defines[]=
		{
			//"ZenModCoreDebug"
		};
		dependencies[]=
		{
			"Game",
			"World",
			"Mission"
		};
		class defs
		{
			class gameScriptModule
			{
				value = "";
				files[] = 
				{ 
					"ZenModCore/scripts/3_Game"
				};
			};
			class worldScriptModule
			{
				value = "";
				files[] = 
				{ 
					"ZenModCore/scripts/4_World"
				};
			};
			class missionScriptModule
			{
				value = "";
				files[] = 
				{ 
					"ZenModCore/scripts/5_Mission"
				};
			};
		};
	};
};

class CfgVehicles
{
	class HouseNoDestruct;
	class Inventory_Base;

	class Zen_InvisibleNonInteractableObject : HouseNoDestruct
	{
		scope = 0;
		physLayer="item_small";
		model = "ZenModCore\data\models\ZenInvisible\invisible_object.p3d";
	};
	class Zen_InvisibleInteractableObject : HouseNoDestruct
	{
		scope = 0;
		physLayer="item_small";
		model = "ZenModCore\data\models\ZenInvisible\interactable_invisible_object.p3d";
		hiddenSelections[] = { "texture" };
		hiddenSelectionsTextures[] = { "#(argb,8,8,3)color(0,0,0,0,ca)" };
	};
	class Zen_InvisibleInteractableObject_Large : Zen_InvisibleInteractableObject
	{
		scope = 0;
		physLayer="item_small";
		model = "ZenModCore\data\models\ZenInvisible\interactable_invisible_object_large.p3d";
		hiddenSelections[] = { "texture" };
		hiddenSelectionsTextures[] = { "#(argb,8,8,3)color(0,0,0,0,ca)"  };
	};
	
	//! SOUND GENERATOR - override these vars and spawn in-game via script to trigger the given sound at this object's spawn position (syncs to all clients)
	class ZenSoundEmitterBase : Inventory_Base
	{
		scope = 0;
		physLayer="item_small";
		model = "ZenModCore\data\models\ZenInvisible\invisible_object.p3d";
		zenSoundset=""; // Soundset from CfgSoundsets
		zenReplaySecs=0; // Will replay the sound on a second-based timer if > 0
		zenShouldLoop=0; // Will loop the sound repeatedly (overrides ^)
		zenFadeInSecs=0; // Fade in time in seconds 
		zenFadeOutSecs=0; // Fade out time in seconds
	};
	class ZenSoundEmitterTest : ZenSoundEmitterBase
	{
		scope=1;
		zenSoundset="BearRoar_SoundSet";
	};
	
	//! GENERIC KIT BOX + SCRIPTED BEHAVIOUR (inherit from this & change projectionTypename and model/texture)
	class ZenKitBase : Inventory_Base
	{
		scope = 0;
		displayName = "Zen's Kit Box Template";
		descriptionShort = "Zen's Kit Box Template";
		model = "ZenModCore\data\models\zenkitbox\zenkitbox.p3d";
		hiddenSelections[] = { "texture" };
		hiddenSelectionsTextures[] = { "ZenModCore\data\models\zenkitbox\data\kit_co.paa" };
		weight = 10000;
		itemSize[] = { 9,5 };
		physLayer = "item_small";
		itemBehaviour = 0;
		soundImpactType = "cloth";
		// Override these to set hologram item type/tolerances/etc
		projectionTypename="Barrel_Green";
		zenIgnoreCollision=1;
		alignHologramToTerain=1;
		slopeTolerance=0.5;
		///////////////////////////////////////////////////
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints = 5000;
					healthLevels[] =
					{

						{
							1,

							{
								"ZenModCore\data\models\zenkitbox\data\kit.rvmat"
							}
						},

						{
							0.69999999,

							{
								"ZenModCore\data\models\zenkitbox\data\kit.rvmat"
							}
						},

						{
							0.5,

							{
								"ZenModCore\data\models\zenkitbox\data\kit_damage.rvmat"
							}
						},

						{
							0.30000001,

							{
								"ZenModCore\data\models\zenkitbox\data\kit_damage.rvmat"
							}
						},

						{
							0,

							{
								"ZenModCore\data\models\zenkitbox\data\kit_destruct.rvmat"
							}
						}
					};
				};
			};
		};
	};
	class ZenKitBoxBase : ZenKitBase
	{
		scope = 0;
	};
	class ZenKitBoxTest : ZenKitBoxBase
	{
		scope = 2;
	};
};