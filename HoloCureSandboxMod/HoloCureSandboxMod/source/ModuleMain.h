#pragma once
#include <Aurie/shared.hpp>
#include <YYToolkit/shared.hpp>
#include <CallbackManager/CallbackManagerInterface.h>

#define VERSION_NUM "v1.0.0"
#define MODNAME "Holocure Sandbox Mod " VERSION_NUM

#define SOME_ENUM(DO) \
	DO(timePause) \
	DO(optionIcon) \
	DO(attackIndex) \
	DO(config) \
	DO(paused) \
	DO(CompleteStop) \
	DO(EndStop) \
	DO(canControl) \
	DO(mouseFollowMode) \
	DO(level) \
	DO(maxLevel) \
	DO(optionID) \
	DO(attacks) \
	DO(shopItemButtons) \
	DO(selectedLanguage) \
	DO(actionOnePressed) \
	DO(actionTwoPressed) \
	DO(weapons) \
	DO(hpSus) \
	DO(items) \
	DO(ITEMS) \
	DO(UpdatePlayer) \
	DO(optionIcon_Super) \
	DO(optionIcon_Normal) \
	DO(becomeSuper) \
	DO(canNotDie) \
	DO(isEnemy) \
	DO(shieldHP) \
	DO(sprite_index) \
	DO(attackID) \

#define MAKE_ENUM(VAR) GML_ ## VAR,
enum VariableNames
{
	SOME_ENUM(MAKE_ENUM)
};

#define MAKE_STRINGS(VAR) #VAR,
const char* const VariableNamesStringsArr[] =
{
	SOME_ENUM(MAKE_STRINGS)
};

extern RValue GMLVarIndexMapGMLHash[1001];
extern CInstance* globalInstance;
extern YYTKInterface* g_ModuleInterface;

extern PFUNC_YYGMLScript origMouseOverButtonScript;
extern PFUNC_YYGMLScript origAddAttackPlayerManagerOtherScript;
extern PFUNC_YYGMLScript origRemoveAttackPlayerManagerOtherScript;
extern PFUNC_YYGMLScript origAddItemPlayerManagerOtherScript;
extern PFUNC_YYGMLScript origRemoveOwnedItemPlayerManagerOtherScript;

extern TRoutine origStructGetFromHashFunc;
extern TRoutine origStructSetFromHashFunc;
extern int objAttackControllerIndex;
extern int objPlayerIndex;
extern int objInputManagerIndex;
extern int sprShopItemBGIndex;
extern int sprShopIconIndex;
extern int sprShopLevelsEmptyIndex;
extern int sprShopLevelsFilledIndex;
extern int sprShopIconSelectedIndex;
extern int sprHudShopButtonIndex;
extern int sprHudToggleButtonIndex;
extern int sprHudOptionButtonIndex;