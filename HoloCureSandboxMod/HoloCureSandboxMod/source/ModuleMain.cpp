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
PFUNC_YYGMLScript origAddPerkPlayerManagerOtherScript = nullptr;
PFUNC_YYGMLScript origAddCollabPlayerManagerOtherScript = nullptr;
PFUNC_YYGMLScript origAddSuperCollabPlayerManagerOtherScript = nullptr;
PFUNC_YYGMLScript origCompleteStopBaseMobCreateScript = nullptr;
PFUNC_YYGMLScript origEndStopBaseMobCreateScript = nullptr;
PFUNC_YYGMLScript origDrawTextOutlineScript = nullptr;
PFUNC_YYGMLScript origAddEnchantPlayerManagerOtherScript = nullptr;
PFUNC_YYGMLScript origCalculateScoreScript = nullptr;
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

AurieStatus moduleInitStatus = AURIE_MODULE_INITIALIZATION_FAILED;

void initHooks()
{
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterBuiltinFunctionCallback(MODNAME, "struct_get_from_hash", nullptr, nullptr, &origStructGetFromHashFunc)))
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to register callback for %s", "struct_get_from_hash");
		return;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterBuiltinFunctionCallback(MODNAME, "struct_set_from_hash", nullptr, nullptr, &origStructSetFromHashFunc)))
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to register callback for %s", "struct_set_from_hash");
		return;
	}

	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_PlayerManager_Step_0", PlayerManagerStepBefore, PlayerManagerStepAfter)))
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to register callback for %s", "gml_Object_obj_PlayerManager_Step_0");
		return;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_PlayerManager_Draw_64", nullptr, PlayerManagerDraw64After)))
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to register callback for %s", "gml_Object_obj_PlayerManager_Draw_64");
		return;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_Player_Mouse_54", PlayerMouse54Before, nullptr)))
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to register callback for %s", "gml_Object_obj_Player_Mouse_54");
		return;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_PlayerManager_Alarm_0", PlayerManagerAlarm0Before, nullptr)))
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to register callback for %s", "gml_Object_obj_PlayerManager_Alarm_0");
		return;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_BaseMob_Draw_0", nullptr, BaseMobDrawAfter)))
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to register callback for %s", "gml_Object_obj_BaseMob_Draw_0");
		return;
	}

	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_CanSubmitScore@gml_Object_obj_PlayerManager_Create_0", CanSubmitScoreFuncBefore, nullptr, nullptr)))
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to register callback for %s", "gml_Script_CanSubmitScore@gml_Object_obj_PlayerManager_Create_0");
		return;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_MouseOverButton", nullptr, nullptr, &origMouseOverButtonScript)))
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to register callback for %s", "gml_Script_MouseOverButton");
		return;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_AddAttack@gml_Object_obj_PlayerManager_Other_24", nullptr, nullptr, &origAddAttackPlayerManagerOtherScript)))
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to register callback for %s", "gml_Script_AddAttack@gml_Object_obj_PlayerManager_Other_24");
		return;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_RemoveAttack@gml_Object_obj_PlayerManager_Other_24", nullptr, nullptr, &origRemoveAttackPlayerManagerOtherScript)))
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to register callback for %s", "gml_Script_RemoveAttack@gml_Object_obj_PlayerManager_Other_24");
		return;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_AddItem@gml_Object_obj_PlayerManager_Other_24", nullptr, nullptr, &origAddItemPlayerManagerOtherScript)))
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to register callback for %s", "gml_Script_AddItem@gml_Object_obj_PlayerManager_Other_24");
		return;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_RemoveOwnedItem@gml_Object_obj_PlayerManager_Other_24", nullptr, nullptr, &origRemoveOwnedItemPlayerManagerOtherScript)))
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to register callback for %s", "gml_Script_RemoveOwnedItem@gml_Object_obj_PlayerManager_Other_24");
		return;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_AddPerk@gml_Object_obj_PlayerManager_Other_24", nullptr, nullptr, &origAddPerkPlayerManagerOtherScript)))
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to register callback for %s", "gml_Script_AddPerk@gml_Object_obj_PlayerManager_Other_24");
		return;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_ApplyDamage@gml_Object_obj_BaseMob_Create_0", ApplyDamageBaseMobCreateBefore, nullptr, nullptr)))
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to register callback for %s", "gml_Script_ApplyDamage@gml_Object_obj_BaseMob_Create_0");
		return;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_AddCollab@gml_Object_obj_PlayerManager_Other_24", nullptr, nullptr, &origAddCollabPlayerManagerOtherScript)))
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to register callback for %s", "gml_Script_AddCollab@gml_Object_obj_PlayerManager_Other_24");
		return;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_AddSuperCollab@gml_Object_obj_PlayerManager_Other_24", nullptr, nullptr, &origAddSuperCollabPlayerManagerOtherScript)))
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to register callback for %s", "gml_Script_AddSuperCollab@gml_Object_obj_PlayerManager_Other_24");
		return;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_CompleteStop@gml_Object_obj_BaseMob_Create_0", nullptr, nullptr, &origCompleteStopBaseMobCreateScript)))
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to register callback for %s", "gml_Script_CompleteStop@gml_Object_obj_BaseMob_Create_0");
		return;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_EndStop@gml_Object_obj_BaseMob_Create_0", nullptr, nullptr, &origEndStopBaseMobCreateScript)))
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to register callback for %s", "gml_Script_EndStop@gml_Object_obj_BaseMob_Create_0");
		return;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_GameOver@gml_Object_obj_PlayerManager_Create_0", GameOverPlayerManagerCreateBefore, nullptr, nullptr)))
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to register callback for %s", "gml_Script_GameOver@gml_Object_obj_PlayerManager_Create_0");
		return;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_DoAchievement", DoAchievementBefore, nullptr, nullptr)))
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to register callback for %s", "gml_Script_DoAchievement");
		return;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_InitializeCharacter@gml_Object_obj_PlayerManager_Create_0", InitializeCharacterPlayerManagerCreateBefore, nullptr, nullptr)))
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to register callback for %s", "gml_Script_InitializeCharacter@gml_Object_obj_PlayerManager_Create_0");
		return;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_draw_text_outline", nullptr, nullptr, &origDrawTextOutlineScript)))
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to register callback for %s", "gml_Script_draw_text_outline");
		return;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_AddEnchant@gml_Object_obj_PlayerManager_Other_24", nullptr, nullptr, &origAddEnchantPlayerManagerOtherScript)))
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to register callback for %s", "gml_Script_AddEnchant@gml_Object_obj_PlayerManager_Other_24");
		return;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_CalculateScore", nullptr, nullptr, &origCalculateScoreScript)))
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to register callback for %s", "gml_Script_CalculateScore");
		return;
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

	AurieStatus status = AURIE_SUCCESS;
	for (int i = 0; i < std::extent<decltype(VariableNamesStringsArr)>::value; i++)
	{
		if (!AurieSuccess(status))
		{
			DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to get hash for %s", VariableNamesStringsArr[i]);
		}
		GMLVarIndexMapGMLHash[i] = std::move(g_ModuleInterface->CallBuiltin("variable_get_hash", { VariableNamesStringsArr[i] }));
	}
	moduleInitStatus = AURIE_SUCCESS;
}

void runnerInitCallback(FunctionWrapper<void(int)>& dummyWrapper)
{
	AurieStatus status = AURIE_SUCCESS;
	status = ObGetInterface("callbackManager", (AurieInterfaceBase*&)callbackManagerInterfacePtr);
	if (!AurieSuccess(status))
	{
		printf("Failed to get callback manager interface. Make sure that CallbackManagerMod is located in the mods/Aurie directory.\n");
		return;
	}

	callbackManagerInterfacePtr->RegisterInitFunction(initHooks);
}

EXPORTED AurieStatus ModulePreinitialize(
	IN AurieModule* Module,
	IN const fs::path& ModulePath
)
{
	UNREFERENCED_PARAMETER(ModulePath);

	// Gets a handle to the interface exposed by YYTK
	// You can keep this pointer for future use, as it will not change unless YYTK is unloaded.
	g_ModuleInterface = GetInterface();

	// If we can't get the interface, we fail loading.
	if (g_ModuleInterface == nullptr)
	{
		DbgPrintEx(LOG_SEVERITY_CRITICAL, "Failed to get YYTK interface");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}

	g_ModuleInterface->CreateCallback(
		Module,
		EVENT_RUNNER_INIT,
		runnerInitCallback,
		0
	);
	return AURIE_SUCCESS;
}

EXPORTED AurieStatus ModuleInitialize(
	IN AurieModule* Module,
	IN const fs::path& ModulePath
)
{
	hWnd = FindWindow(NULL, L"HoloCure");

	framePauseThread = std::thread(framePauseThreadHandler);

	callbackManagerInterfacePtr->LogToFile(MODNAME, "Finished initializing");

	return moduleInitStatus;
}