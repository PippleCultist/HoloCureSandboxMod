#pragma once
#include <Aurie/shared.hpp>
#include <YYToolkit/YYTK_Shared.hpp>
#include <CallbackManager/CallbackManagerInterface.h>

#define VERSION_NUM "v1.1.4"
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
	DO(weaponCollabs) \
	DO(combos) \
	DO(STICKERS) \
	DO(x) \
	DO(y) \
	DO(depth) \
	DO(stickerData) \
	DO(HP) \
	DO(currentHP) \
	DO(ATK) \
	DO(SPD) \
	DO(PERKS) \
	DO(perks) \
	DO(playerStatUps) \
	DO(crit) \
	DO(pickupRange) \
	DO(haste) \
	DO(playerCharacter) \
	DO(buffs) \
	DO(timer) \
	DO(buffIcon) \
	DO(Buffs) \
	DO(ApplyBuff) \
	DO(stacks) \
	DO(maxStacks) \
	DO(buffName) \
	DO(reapply) \
	DO(optionType) \
	DO(enhancements) \
	DO(gainedMods) \

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
extern YYRunnerInterface g_RunnerInterface;

extern PFUNC_YYGMLScript origMouseOverButtonScript;
extern PFUNC_YYGMLScript origAddAttackPlayerManagerOtherScript;
extern PFUNC_YYGMLScript origRemoveAttackPlayerManagerOtherScript;
extern PFUNC_YYGMLScript origAddItemPlayerManagerOtherScript;
extern PFUNC_YYGMLScript origRemoveOwnedItemPlayerManagerOtherScript;
extern PFUNC_YYGMLScript origAddCollabPlayerManagerOtherScript;
extern PFUNC_YYGMLScript origAddPerkPlayerManagerOtherScript;
extern PFUNC_YYGMLScript origCompleteStopBaseMobCreateScript;
extern PFUNC_YYGMLScript origEndStopBaseMobCreateScript;
extern PFUNC_YYGMLScript origDrawTextOutlineScript;
extern PFUNC_YYGMLScript origAddEnchantPlayerManagerOtherScript;

extern TRoutine origStructGetFromHashFunc;
extern TRoutine origStructSetFromHashFunc;
extern int objAttackControllerIndex;
extern int objPlayerIndex;
extern int objInputManagerIndex;
extern int objEnemyIndex;
extern int objStickerIndex;
extern int objStageManagerIndex;
extern int objHoloAnvilIndex;
extern int sprShopItemBGIndex;
extern int sprShopIconIndex;
extern int sprShopLevelsEmptyIndex;
extern int sprShopLevelsFilledIndex;
extern int sprShopIconSelectedIndex;
extern int sprHudShopButtonIndex;
extern int sprHudToggleButtonIndex;
extern int sprHudOptionButtonIndex;
extern int sprUnknownIconButtonIndex;
extern int sprBulletTempIndex;