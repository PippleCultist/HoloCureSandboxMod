#include <Aurie/shared.hpp>
#include <YYToolkit/YYTK_Shared.hpp>
#include <CallbackManager/CallbackManagerInterface.h>
#include "ScriptFunctions.h"
#include "CodeEvents.h"
#include "ModuleMain.h"
#include <thread>
using namespace Aurie;
using namespace YYTK;

RValue GMLVarIndexMapGMLHash[1001];

TRoutine origStructGetFromHashFunc;
TRoutine origStructSetFromHashFunc;

CallbackManagerInterface* callbackManagerInterfacePtr = nullptr;
YYTKInterface* g_ModuleInterface = nullptr;
YYRunnerInterface g_RunnerInterface;

PFUNC_YYGMLScript origMouseOverButtonScript = nullptr;
PFUNC_YYGMLScript origAddAttackPlayerManagerOtherScript = nullptr;
PFUNC_YYGMLScript origRemoveAttackPlayerManagerOtherScript = nullptr;
PFUNC_YYGMLScript origAddItemPlayerManagerOtherScript = nullptr;
PFUNC_YYGMLScript origRemoveOwnedItemPlayerManagerOtherScript = nullptr;
PFUNC_YYGMLScript origAddCollabPlayerManagerOtherScript = nullptr;
PFUNC_YYGMLScript origCompleteStopBaseMobCreateScript = nullptr;
PFUNC_YYGMLScript origEndStopBaseMobCreateScript = nullptr;
PFUNC_YYGMLScript origDrawTextOutlineScript = nullptr;
HWND hWnd;

CInstance* globalInstance = nullptr;
std::thread framePauseThread;

int objAttackControllerIndex = -1;
int objPlayerIndex = -1;
int objInputManagerIndex = -1;
int objEnemyIndex = -1;
int objStickerIndex = -1;
int objStageManagerIndex = -1;
int objHoloAnvilIndex = -1;
int sprShopItemBGIndex = -1;
int sprShopIconIndex = -1;
int sprShopLevelsEmptyIndex = -1;
int sprShopLevelsFilledIndex = -1;
int sprShopIconSelectedIndex = -1;
int sprHudShopButtonIndex = -1;
int sprHudToggleButtonIndex = -1;
int sprHudOptionButtonIndex = -1;
int sprUnknownIconButtonIndex = -1;
int sprBulletTempIndex = -1;

EXPORTED AurieStatus ModuleInitialize(
	IN AurieModule* Module,
	IN const fs::path& ModulePath
)
{
	AurieStatus status = AURIE_SUCCESS;
	status = ObGetInterface("callbackManager", (AurieInterfaceBase*&)callbackManagerInterfacePtr);
	if (!AurieSuccess(status))
	{
		printf("Failed to get callback manager interface. Make sure that CallbackManagerMod is located in the mods/Aurie directory.\n");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	// Gets a handle to the interface exposed by YYTK
	// You can keep this pointer for future use, as it will not change unless YYTK is unloaded.
	status = ObGetInterface(
		"YYTK_Main",
		(AurieInterfaceBase*&)(g_ModuleInterface)
	);

	// If we can't get the interface, we fail loading.
	if (!AurieSuccess(status))
	{
		printf("Failed to get YYTK Interface\n");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}

	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterBuiltinFunctionCallback(MODNAME, "struct_get_from_hash", nullptr, nullptr, &origStructGetFromHashFunc)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "struct_get_from_hash");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterBuiltinFunctionCallback(MODNAME, "struct_set_from_hash", nullptr, nullptr, &origStructSetFromHashFunc)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "struct_set_from_hash");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}

	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_PlayerManager_Step_0", PlayerManagerStepBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_PlayerManager_Step_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_PlayerManager_Draw_64", nullptr, PlayerManagerDraw64After)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_PlayerManager_Draw_64");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_Player_Mouse_54", PlayerMouse54Before, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_Player_Mouse_54");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_PlayerManager_Alarm_0", PlayerManagerAlarm0Before, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_PlayerManager_Alarm_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_BaseMob_Draw_0", nullptr, BaseMobDrawAfter)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_BaseMob_Draw_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}

	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_CanSubmitScore@gml_Object_obj_PlayerManager_Create_0", CanSubmitScoreFuncBefore, nullptr, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_CanSubmitScore@gml_Object_obj_PlayerManager_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_MouseOverButton", nullptr, nullptr, &origMouseOverButtonScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_MouseOverButton");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_AddAttack@gml_Object_obj_PlayerManager_Other_24", nullptr, nullptr, &origAddAttackPlayerManagerOtherScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_AddAttack@gml_Object_obj_PlayerManager_Other_24");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_RemoveAttack@gml_Object_obj_PlayerManager_Other_24", nullptr, nullptr, &origRemoveAttackPlayerManagerOtherScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_RemoveAttack@gml_Object_obj_PlayerManager_Other_24");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_AddItem@gml_Object_obj_PlayerManager_Other_24", nullptr, nullptr, &origAddItemPlayerManagerOtherScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_AddItem@gml_Object_obj_PlayerManager_Other_24");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_RemoveOwnedItem@gml_Object_obj_PlayerManager_Other_24", nullptr, nullptr, &origRemoveOwnedItemPlayerManagerOtherScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_RemoveOwnedItem@gml_Object_obj_PlayerManager_Other_24");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_ApplyDamage@gml_Object_obj_BaseMob_Create_0", ApplyDamageBaseMobCreateBefore, nullptr, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_ApplyDamage@gml_Object_obj_BaseMob_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_AddCollab@gml_Object_obj_PlayerManager_Other_24", nullptr, nullptr, &origAddCollabPlayerManagerOtherScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_AddCollab@gml_Object_obj_PlayerManager_Other_24");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_CompleteStop@gml_Object_obj_BaseMob_Create_0", nullptr, nullptr, &origCompleteStopBaseMobCreateScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_CompleteStop@gml_Object_obj_BaseMob_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_EndStop@gml_Object_obj_BaseMob_Create_0", nullptr, nullptr, &origEndStopBaseMobCreateScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_EndStop@gml_Object_obj_BaseMob_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_GameOver@gml_Object_obj_PlayerManager_Create_0", GameOverPlayerManagerCreateBefore, nullptr, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_GameOver@gml_Object_obj_PlayerManager_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_DoAchievement", DoAchievementBefore, nullptr, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_DoAchievement");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_InitializeCharacter@gml_Object_obj_PlayerManager_Create_0", InitializeCharacterPlayerManagerCreateBefore, nullptr, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_InitializeCharacter@gml_Object_obj_PlayerManager_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_draw_text_outline", nullptr, nullptr, &origDrawTextOutlineScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_draw_text_outline");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}

	g_RunnerInterface = g_ModuleInterface->GetRunnerInterface();
	g_ModuleInterface->GetGlobalInstance(&globalInstance);

	objAttackControllerIndex = g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_AttackController" }).ToInt32();
	objPlayerIndex = g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_Player" }).ToInt32();
	objInputManagerIndex = g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_InputManager" }).ToInt32();
	objEnemyIndex = g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_Enemy" }).ToInt32();
	objStickerIndex = g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_Sticker" }).ToInt32();
	objStageManagerIndex = g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_StageManager" }).ToInt32();
	objHoloAnvilIndex = g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_holoAnvil" }).ToInt32();
	sprShopItemBGIndex = g_ModuleInterface->CallBuiltin("asset_get_index", { "spr_shopItemBG" }).ToInt32();
	sprShopIconIndex = g_ModuleInterface->CallBuiltin("asset_get_index", { "spr_shopIcon" }).ToInt32();
	sprShopLevelsEmptyIndex = g_ModuleInterface->CallBuiltin("asset_get_index", { "spr_shopLevels_Empty" }).ToInt32();
	sprShopLevelsFilledIndex = g_ModuleInterface->CallBuiltin("asset_get_index", { "spr_shopLevels_Filled" }).ToInt32();
	sprShopIconSelectedIndex = g_ModuleInterface->CallBuiltin("asset_get_index", { "spr_shopIconSelected" }).ToInt32();
	sprHudShopButtonIndex = g_ModuleInterface->CallBuiltin("asset_get_index", { "hud_shopButton" }).ToInt32();
	sprHudToggleButtonIndex = g_ModuleInterface->CallBuiltin("asset_get_index", { "hud_toggleButton" }).ToInt32();
	sprHudOptionButtonIndex = g_ModuleInterface->CallBuiltin("asset_get_index", { "hud_OptionButton" }).ToInt32();
	sprUnknownIconButtonIndex = g_ModuleInterface->CallBuiltin("asset_get_index", { "spr_UnknownIcon" }).ToInt32();
	sprBulletTempIndex = g_ModuleInterface->CallBuiltin("asset_get_index", { "spr_BulletTemp" }).ToInt32();

	for (int i = 0; i < std::extent<decltype(VariableNamesStringsArr)>::value; i++)
	{
		if (!AurieSuccess(status))
		{
			g_ModuleInterface->Print(CM_RED, "Failed to get hash for %s", VariableNamesStringsArr[i]);
		}
		GMLVarIndexMapGMLHash[i] = std::move(g_ModuleInterface->CallBuiltin("variable_get_hash", { VariableNamesStringsArr[i] }));
	}

	hWnd = FindWindow(NULL, L"HoloCure");

	framePauseThread = std::thread(framePauseThreadHandler);

	callbackManagerInterfacePtr->LogToFile(MODNAME, "Finished initializing");

	return AURIE_SUCCESS;
}