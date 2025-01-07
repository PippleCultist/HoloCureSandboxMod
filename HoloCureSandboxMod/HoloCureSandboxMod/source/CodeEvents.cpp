#include "CommonFunctions.h"
#include "CodeEvents.h"
#include "ScriptFunctions.h"
#include <thread>

extern std::unordered_map<std::string, damageData> damagePerFrameMap;
extern CallbackManagerInterface* callbackManagerInterfacePtr;
extern HWND hWnd;

bool isInSandboxMenu = false;
bool prevIsTimePaused = false;
bool prevCanControl = false;
bool prevMouseFollowMode = false;
bool isSandboxMenuButtonPressed = false;

bool isTimePausedButtonPressed = false;
bool isProcessPausedButtonPressed = false;
bool isProcessPaused = false;
bool isNextFrameButtonPressed = false;
bool isNextFrame = false;
int curSandboxMenuPage = 0;
int selectedItemIndex = -1;
int dpsNumFrameCount = 0;

std::vector<std::pair<double, std::string>> damageList;

enum sandboxItemType
{
	SANDBOXITEMTYPE_Weapon,
	SANDBOXITEMTYPE_Item,
	SANDBOXITEMTYPE_Collab,
	SANDBOXITEMTYPE_Stamp,
};

struct sandboxShopItemData
{
	std::string itemName;
	int optionIcon;
	int curLevel;
	int maxLevel;
	bool canSuper;
	sandboxItemType itemType;

	sandboxShopItemData(std::string itemName, int optionIcon, int curLevel, int maxLevel, bool canSuper, sandboxItemType itemType) : itemName(itemName), optionIcon(optionIcon), curLevel(curLevel), maxLevel(maxLevel), canSuper(canSuper), itemType(itemType)
	{
	}
};

void drawTextOutline(CInstance* Self, double xPos, double yPos, std::string text, double outlineWidth, int outlineColor, double numOutline, double linePixelSeparation, double pixelsBeforeLineBreak, int textColor, double alpha)
{
	RValue** args = new RValue*[10];
	args[0] = new RValue(xPos);
	args[1] = new RValue(yPos);
	args[2] = new RValue(text);
	args[3] = new RValue(outlineWidth);
	args[4] = new RValue(static_cast<double>(outlineColor));
	args[5] = new RValue(numOutline);
	args[6] = new RValue(linePixelSeparation);
	args[7] = new RValue(pixelsBeforeLineBreak);
	args[8] = new RValue(static_cast<double>(textColor));
	args[9] = new RValue(alpha);
	RValue returnVal;
	origDrawTextOutlineScript(Self, nullptr, returnVal, 10, args);
}

void setTimeToThirtyMin()
{
	RValue timeArr = g_ModuleInterface->CallBuiltin("variable_global_get", { "time" });
	timeArr[0] = 0;
	timeArr[1] = 29;
	timeArr[2] = 59;
	RValue stageManager = g_ModuleInterface->CallBuiltin("instance_find", { objStageManagerIndex, 0 });
	RValue mobSpawnChoices;
	g_RunnerInterface.StructCreate(&mobSpawnChoices);
	g_ModuleInterface->CallBuiltin("variable_struct_set", { stageManager, "mobSpawnChoices", mobSpawnChoices });
}

void setTimeToFortyMin()
{
	RValue timeArr = g_ModuleInterface->CallBuiltin("variable_global_get", { "time" });
	timeArr[0] = 0;
	timeArr[1] = 39;
	timeArr[2] = 59;
	RValue stageManager = g_ModuleInterface->CallBuiltin("instance_find", { objStageManagerIndex, 0 });
	RValue mobSpawnChoices;
	g_RunnerInterface.StructCreate(&mobSpawnChoices);
	g_ModuleInterface->CallBuiltin("variable_struct_set", { stageManager, "mobSpawnChoices", mobSpawnChoices });
}

void createAnvil()
{
	RValue player = g_ModuleInterface->CallBuiltin("instance_find", { objPlayerIndex, 0 });
	RValue xPos = getInstanceVariable(player, GML_x);
	RValue yPos = getInstanceVariable(player, GML_y);
	RValue depth = getInstanceVariable(player, GML_depth);
	RValue sticker = g_ModuleInterface->CallBuiltin("instance_create_depth", { xPos, yPos.m_Real - 20, depth, objHoloAnvilIndex });
}

bool isKeyPressed(int vKey)
{
	return GetForegroundWindow() == hWnd && (GetAsyncKeyState(vKey) & 0xFFFE) != 0;
}

void framePauseThreadHandler()
{
	while (true)
	{
		if (isKeyPressed('N'))
		{
			if (!isProcessPausedButtonPressed)
			{
				isProcessPaused = !isProcessPaused;
				isProcessPausedButtonPressed = true;
			}
		}
		else
		{
			isProcessPausedButtonPressed = false;
		}
		if (isProcessPaused)
		{
			if (isKeyPressed('M'))
			{
				if (!isNextFrameButtonPressed)
				{
					isNextFrame = true;
					isNextFrameButtonPressed = true;
				}
			}
			else
			{
				isNextFrameButtonPressed = false;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

std::vector<sandboxMenuData*> sandboxOptionList = {
	new sandboxCheckBox(10, 90, false, "Immortal Enemies"),
	new sandboxCheckBox(10, 125, true, "DPS Tracker"),
	new sandboxCheckBox(10, 160, false, "Enable Debug"),
	new sandboxCheckBox(10, 195, false, "Show Mob HP"),
	new sandboxButton(10, 230, setTimeToThirtyMin, "Go to 30:00"),
	new sandboxButton(10, 265, setTimeToFortyMin, "Go to 40:00"),
	new sandboxButton(10, 300, createAnvil, "Create anvil"),
};

void PlayerManagerStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	CInstance* Self = std::get<0>(Args);
	while (isProcessPaused)
	{
		if (isNextFrame)
		{
			isNextFrame = false;
			break;
		}
		PeekMessageA(nullptr, nullptr, 0, 0, PM_NOREMOVE);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	RValue paused = getInstanceVariable(Self, GML_paused);
	if (paused.AsBool())
	{
		return;
	}

	for (auto& damagePerFramePair : damagePerFrameMap)
	{
		damageData& curDamageData = damagePerFramePair.second;
		curDamageData.damageQueue.push_back(curDamageData.curFrameDamage);
		// Keep 3 seconds of damage data
		if (curDamageData.damageQueue.size() > 180)
		{
			curDamageData.damageQueue.pop_front();
		}
		curDamageData.curFrameDamage = 0;
	}

	bool hasUpdatedTimePaused = false;
	if (isKeyPressed('Y'))
	{
		if (!isSandboxMenuButtonPressed)
		{
			isSandboxMenuButtonPressed = true;
			isInSandboxMenu = !isInSandboxMenu;
			bool isTimePaused = false;
			RValue player = g_ModuleInterface->CallBuiltin("instance_find", { objPlayerIndex, 0 });
			if (isInSandboxMenu)
			{
				prevIsTimePaused = g_ModuleInterface->CallBuiltin("variable_global_get", { "timePause" }).AsBool();
				isTimePaused = true;
				prevCanControl = getInstanceVariable(player, GML_canControl).AsBool();
				prevMouseFollowMode = getInstanceVariable(player, GML_mouseFollowMode).AsBool();
				setInstanceVariable(player, GML_canControl, false);
				setInstanceVariable(player, GML_mouseFollowMode, false);
				curSandboxMenuPage = 0;
			}
			else
			{
				isTimePaused = prevIsTimePaused;
				setInstanceVariable(player, GML_canControl, prevCanControl);
				setInstanceVariable(player, GML_mouseFollowMode, prevMouseFollowMode);
				selectedItemIndex = -1;
			}
			hasUpdatedTimePaused = true;
			g_ModuleInterface->CallBuiltin("variable_global_set", { "timePause", isTimePaused });
		}
	}
	else
	{
		isSandboxMenuButtonPressed = false;
	}

	if (!isInSandboxMenu)
	{
		if (isKeyPressed('P'))
		{
			if (!isTimePausedButtonPressed)
			{
				isTimePausedButtonPressed = true;
				bool isTimePaused = g_ModuleInterface->CallBuiltin("variable_global_get", { "timePause" }).AsBool();
				g_ModuleInterface->CallBuiltin("variable_global_set", { "timePause", !isTimePaused });
				hasUpdatedTimePaused = true;
			}
		}
		else
		{
			isTimePausedButtonPressed = false;
		}
	}

	if (hasUpdatedTimePaused)
	{
		bool isTimePaused = g_ModuleInterface->CallBuiltin("variable_global_get", { "timePause" }).AsBool();
		if (isTimePaused)
		{
			int enemyCount = static_cast<int>(lround(g_ModuleInterface->CallBuiltin("instance_number", { objEnemyIndex }).AsReal()));
			for (int i = 0; i < enemyCount; i++)
			{
				RValue instance = g_ModuleInterface->CallBuiltin("instance_find", { objEnemyIndex, i });
				int instanceID = static_cast<int>(lround(instance.AsReal()));
				CInstance* objectInstance = nullptr;
				if (!AurieSuccess(g_ModuleInterface->GetInstanceObject(instanceID, objectInstance)))
				{
					continue;
				}
				RValue result;
				origCompleteStopBaseMobCreateScript(objectInstance, objectInstance, result, 0, nullptr);
			}
		}
		else
		{
			int enemyCount = static_cast<int>(lround(g_ModuleInterface->CallBuiltin("instance_number", { objEnemyIndex }).AsReal()));
			for (int i = 0; i < enemyCount; i++)
			{
				RValue instance = g_ModuleInterface->CallBuiltin("instance_find", { objEnemyIndex, i });
				int instanceID = static_cast<int>(lround(instance.AsReal()));
				CInstance* objectInstance = nullptr;
				if (!AurieSuccess(g_ModuleInterface->GetInstanceObject(instanceID, objectInstance)))
				{
					continue;
				}
				RValue result;
				origEndStopBaseMobCreateScript(objectInstance, objectInstance, result, 0, nullptr);
			}
		}
	}
}

void handleSandboxItemMenuInteract(CInstance* playerManagerInstance, sandboxShopItemData& curItem, int buyOption)
{
	switch (curItem.itemType)
	{
		case SANDBOXITEMTYPE_Weapon:
		{
			if (buyOption == 0)
			{
				if (curItem.curLevel == curItem.maxLevel)
				{
					return;
				}
				RValue itemName = curItem.itemName;
				RValue returnVal;
				RValue** args = new RValue*[1];
				args[0] = &itemName;
				origAddAttackPlayerManagerOtherScript(playerManagerInstance, nullptr, returnVal, 1, args);
			}
			else
			{
				RValue player = g_ModuleInterface->CallBuiltin("instance_find", { objPlayerIndex, 0 });
				RValue attacks = getInstanceVariable(player, GML_attacks);
				RValue playerWeapon = g_ModuleInterface->CallBuiltin("ds_map_find_value", { attacks, curItem.itemName });
				if (playerWeapon.m_Kind == VALUE_UNDEFINED)
				{
					return;
				}
				RValue config = getInstanceVariable(playerWeapon, GML_config);
				RValue optionID = getInstanceVariable(config, GML_optionID);
				RValue weapons = getInstanceVariable(playerManagerInstance, GML_weapons);
				RValue curWeapon = g_ModuleInterface->CallBuiltin("variable_struct_get", { weapons, optionID });
				setInstanceVariable(curWeapon, GML_level, 0);
				g_ModuleInterface->CallBuiltin("ds_map_delete", { attacks, optionID });
			}
			break;
		}
		case SANDBOXITEMTYPE_Item:
		{
			if (buyOption == 0)
			{
				if (curItem.curLevel == curItem.maxLevel)
				{
					return;
				}
				if (curItem.curLevel == curItem.maxLevel - 1 && curItem.canSuper)
				{
					RValue itemName = curItem.itemName;
					RValue itemsMap = getInstanceVariable(playerManagerInstance, GML_ITEMS);
					RValue item = g_ModuleInterface->CallBuiltin("ds_map_find_value", { itemsMap, itemName });
					RValue maxLevel = getInstanceVariable(item, GML_maxLevel);
					setInstanceVariable(item, GML_becomeSuper, true);
					setInstanceVariable(item, GML_optionIcon, getInstanceVariable(item, GML_optionIcon_Super));
					RValue returnVal;
					RValue** args = new RValue*[1];
					args[0] = &itemName;
					origAddItemPlayerManagerOtherScript(playerManagerInstance, nullptr, returnVal, 1, args);
				}
				else
				{
					RValue itemName = curItem.itemName;
					RValue returnVal;
					RValue** args = new RValue*[1];
					args[0] = &itemName;
					origAddItemPlayerManagerOtherScript(playerManagerInstance, nullptr, returnVal, 1, args);
				}
			}
			else
			{
				RValue itemsMap = getInstanceVariable(playerManagerInstance, GML_ITEMS);
				RValue itemName = curItem.itemName;
				RValue item = g_ModuleInterface->CallBuiltin("ds_map_find_value", { itemsMap, itemName });
				setInstanceVariable(item, GML_level, -1.0);
				RValue returnVal;
				RValue** args = new RValue*[1];
				args[0] = &itemName;
				origRemoveOwnedItemPlayerManagerOtherScript(playerManagerInstance, nullptr, returnVal, 1, args);
				RValue UpdatePlayerMethod = getInstanceVariable(playerManagerInstance, GML_UpdatePlayer);
				RValue UpdatePlayerMethodArr = g_ModuleInterface->CallBuiltin("array_create", { RValue(0.0) });
				g_ModuleInterface->CallBuiltin("method_call", { UpdatePlayerMethod, UpdatePlayerMethodArr });
				setInstanceVariable(item, GML_becomeSuper, false);
				setInstanceVariable(item, GML_optionIcon, getInstanceVariable(item, GML_optionIcon_Normal));
			}
			break;
		}
		case SANDBOXITEMTYPE_Collab:
		{
			if (buyOption == 0)
			{
				if (curItem.curLevel == curItem.maxLevel)
				{
					return;
				}
				RValue itemName = curItem.itemName;
				RValue returnVal;
				RValue** args = new RValue*[1];
				args[0] = &itemName;

				RValue player = g_ModuleInterface->CallBuiltin("instance_find", { objPlayerIndex, 0 });
				RValue attacks = getInstanceVariable(player, GML_attacks);

				RValue weaponCollabs = getInstanceVariable(playerManagerInstance, GML_weaponCollabs);
				RValue weapon = g_ModuleInterface->CallBuiltin("variable_instance_get", { weaponCollabs, itemName });
				RValue config = getInstanceVariable(weapon, GML_config);
				RValue combos = getInstanceVariable(config, GML_combos);
				
				RValue attackComboOne = g_ModuleInterface->CallBuiltin("ds_map_find_value", { attacks, combos[0] });
				RValue attackComboTwo = g_ModuleInterface->CallBuiltin("ds_map_find_value", { attacks, combos[1] });
				RValue tempWeapon;
				RValue tempConfig;
				RValue tempGainedMods = g_ModuleInterface->CallBuiltin("array_create", { 0 });
				g_RunnerInterface.StructCreate(&tempWeapon);
				g_RunnerInterface.StructCreate(&tempConfig);
				g_RunnerInterface.StructAddRValue(&tempConfig, "gainedMods", &tempGainedMods);
				g_RunnerInterface.StructAddRValue(&tempWeapon, "config", &tempConfig);
				if (attackComboOne.m_Kind != VALUE_UNDEFINED)
				{
					g_ModuleInterface->CallBuiltin("ds_map_set", { attacks, "attackComboOne", attackComboOne });
				}
				if (attackComboTwo.m_Kind != VALUE_UNDEFINED)
				{
					g_ModuleInterface->CallBuiltin("ds_map_set", { attacks, "attackComboTwo", attackComboTwo });
				}
				g_ModuleInterface->CallBuiltin("ds_map_set", { attacks, combos[0], tempWeapon });
				g_ModuleInterface->CallBuiltin("ds_map_set", { attacks, combos[1], tempWeapon });

				origAddCollabPlayerManagerOtherScript(playerManagerInstance, nullptr, returnVal, 1, args);
				if (attackComboOne.m_Kind != VALUE_UNDEFINED)
				{
					g_ModuleInterface->CallBuiltin("ds_map_set", { attacks, combos[0], attackComboOne });
					g_ModuleInterface->CallBuiltin("ds_map_delete", { attacks, "attackComboOne" });
				}
				if (attackComboTwo.m_Kind != VALUE_UNDEFINED)
				{
					g_ModuleInterface->CallBuiltin("ds_map_set", { attacks, combos[1], attackComboTwo });
					g_ModuleInterface->CallBuiltin("ds_map_delete", { attacks, "attackComboTwo" });
				}
			}
			else
			{
				RValue player = g_ModuleInterface->CallBuiltin("instance_find", { objPlayerIndex, 0 });
				RValue attacks = getInstanceVariable(player, GML_attacks);
				RValue playerWeapon = g_ModuleInterface->CallBuiltin("ds_map_find_value", { attacks, curItem.itemName });
				if (playerWeapon.m_Kind == VALUE_UNDEFINED)
				{
					return;
				}
				RValue config = getInstanceVariable(playerWeapon, GML_config);
				RValue optionID = getInstanceVariable(config, GML_optionID);
				RValue weaponCollabs = getInstanceVariable(playerManagerInstance, GML_weaponCollabs);
				RValue curWeapon = g_ModuleInterface->CallBuiltin("variable_struct_get", { weaponCollabs, optionID });
				setInstanceVariable(curWeapon, GML_level, 0);
				g_ModuleInterface->CallBuiltin("ds_map_delete", { attacks, optionID });
			}
			break;
		}
		case SANDBOXITEMTYPE_Stamp:
		{
			if (buyOption == 0)
			{
				if (curItem.curLevel == curItem.maxLevel)
				{
					return;
				}
				RValue player = g_ModuleInterface->CallBuiltin("instance_find", { objPlayerIndex, 0 });
				RValue xPos = getInstanceVariable(player, GML_x);
				RValue yPos = getInstanceVariable(player, GML_y);
				RValue depth = getInstanceVariable(player, GML_depth);
				RValue sticker = g_ModuleInterface->CallBuiltin("instance_create_depth", { xPos, yPos, depth, objStickerIndex });
				RValue stickersMap = getInstanceVariable(playerManagerInstance, GML_STICKERS);
				RValue stickerData = g_ModuleInterface->CallBuiltin("ds_map_find_value", { stickersMap, curItem.itemName });
				setInstanceVariable(sticker, GML_stickerData, stickerData);
				setInstanceVariable(sticker, GML_sprite_index, curItem.optionIcon);
				RValue availableStickers = g_ModuleInterface->CallBuiltin("variable_global_get", { "availableStickers" });
				int availableStickersLength = static_cast<int>(g_ModuleInterface->CallBuiltin("array_length", { availableStickers }).m_Real);
				for (int i = 0; i < availableStickersLength; i++)
				{
					if (availableStickers[i].AsString().compare(curItem.itemName) == 0)
					{
						g_ModuleInterface->CallBuiltin("array_delete", { availableStickers, i, 1 });
						break;
					}
				}
			}
			break;
		}
		default:
		{
			g_ModuleInterface->Print(CM_RED, "ERROR: COULDN'T HANDLE SANDBOX ITEM INTERACT");
		}
	}
}

int getSpriteIndexFromAttackName(std::string& attackID)
{
	if (attackID.empty())
	{
		return sprUnknownIconButtonIndex;
	}

	RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
	RValue attackIndex = getInstanceVariable(attackController, GML_attackIndex);
	if (attackIndex.m_Kind == VALUE_UNDEFINED)
	{
		return sprUnknownIconButtonIndex;
	}
	RValue curWeapon = g_ModuleInterface->CallBuiltin("ds_map_find_value", { attackIndex, attackID });
	if (curWeapon.m_Kind != VALUE_UNDEFINED)
	{
		RValue config = getInstanceVariable(curWeapon, GML_config);
		int optionIcon = static_cast<int>(lround(getInstanceVariable(config, GML_optionIcon).AsReal()));
		if (optionIcon == sprBulletTempIndex)
		{
			return sprUnknownIconButtonIndex;
		}
		return optionIcon;
	}
	
	return sprUnknownIconButtonIndex;
}

int cursorLifetime = 0;
int buyOptionIndex = 0;

void PlayerManagerDraw64After(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	CInstance* Self = std::get<0>(Args);
	RValue paused = getInstanceVariable(Self, GML_paused);
	RValue player = g_ModuleInterface->CallBuiltin("instance_find", { objPlayerIndex, 0 });
	setInstanceVariable(player, GML_hpSus, 0);
	if (isInSandboxMenu)
	{
		if (paused.AsBool())
		{
			return;
		}

		RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
		RValue attackIndex = getInstanceVariable(attackController, GML_attackIndex);

		if (attackIndex.m_Kind == VALUE_UNDEFINED)
		{
			return;
		}

		RValue inputManager = g_ModuleInterface->CallBuiltin("instance_find", { objInputManagerIndex, 0 });
		RValue actionTwoPressed = getInstanceVariable(inputManager, GML_actionTwoPressed);
		RValue isRightMouseButtonPressed = g_ModuleInterface->CallBuiltin("mouse_check_button_pressed", { 2 });
		if (actionTwoPressed.AsBool() || isRightMouseButtonPressed.AsBool())
		{
			selectedItemIndex = -1;
		}

		std::vector<sandboxShopItemData> itemDataList;

		RValue attacks = getInstanceVariable(player, GML_attacks);
		RValue playerSave = g_ModuleInterface->CallBuiltin("variable_global_get", { "PlayerSave" });
		RValue unlockedWeapons = g_ModuleInterface->CallBuiltin("ds_map_find_value", { playerSave, "unlockedWeapons" });
		int unlockedWeaponsLength = static_cast<int>(g_ModuleInterface->CallBuiltin("array_length", { unlockedWeapons }).m_Real);
		for (int i = 0; i < unlockedWeaponsLength; i++)
		{
			RValue curWeaponName = unlockedWeapons[i];
			RValue curWeapon = g_ModuleInterface->CallBuiltin("ds_map_find_value", { attackIndex, curWeaponName });
			RValue config = getInstanceVariable(curWeapon, GML_config);
			RValue optionIcon = getInstanceVariable(config, GML_optionIcon);
			RValue maxLevel = getInstanceVariable(config, GML_maxLevel);
			int curLevel = 0;
			RValue playerWeapon = g_ModuleInterface->CallBuiltin("ds_map_find_value", { attacks, curWeaponName });
			if (playerWeapon.m_Kind != VALUE_UNDEFINED)
			{
				RValue playerWeaponConfig = getInstanceVariable(playerWeapon, GML_config);
				curLevel = static_cast<int>(lround(getInstanceVariable(playerWeaponConfig, GML_level).m_Real));
			}
			itemDataList.push_back(sandboxShopItemData(std::string(curWeaponName.AsString()), static_cast<int>(lround(optionIcon.AsReal())), curLevel, static_cast<int>(lround(maxLevel.m_Real)), false, SANDBOXITEMTYPE_Weapon));
		}

		RValue weaponCollabs = getInstanceVariable(Self, GML_weaponCollabs);
		RValue weaponCollabNames = g_ModuleInterface->CallBuiltin("struct_get_names", { weaponCollabs });
		int weaponCollabsLength = static_cast<int>(g_ModuleInterface->CallBuiltin("array_length", { weaponCollabNames }).m_Real);
		for (int i = 0; i < weaponCollabsLength; i++)
		{
			RValue curWeaponName = weaponCollabNames[i];
			RValue curWeapon = g_ModuleInterface->CallBuiltin("variable_struct_get", { weaponCollabs, curWeaponName });
			RValue config = getInstanceVariable(curWeapon, GML_config);
			RValue optionIcon = getInstanceVariable(config, GML_optionIcon);
			int curLevel = 0;
			RValue playerWeapon = g_ModuleInterface->CallBuiltin("ds_map_find_value", { attacks, curWeaponName });
			if (playerWeapon.m_Kind != VALUE_UNDEFINED)
			{
				RValue playerWeaponConfig = getInstanceVariable(playerWeapon, GML_config);
				curLevel = static_cast<int>(lround(getInstanceVariable(playerWeaponConfig, GML_level).m_Real));
			}
			itemDataList.push_back(sandboxShopItemData(std::string(curWeaponName.AsString()), static_cast<int>(lround(optionIcon.AsReal())), curLevel, 1, false, SANDBOXITEMTYPE_Collab));
		}

		RValue itemsMap = getInstanceVariable(Self, GML_ITEMS);
		RValue items = getInstanceVariable(Self, GML_items);
		RValue unlockedItems = g_ModuleInterface->CallBuiltin("ds_map_find_value", { playerSave, "unlockedItems" });
		int unlockedItemsLength = static_cast<int>(g_ModuleInterface->CallBuiltin("array_length", { unlockedItems }).m_Real);
		for (int i = 0; i < unlockedItemsLength; i++)
		{
			RValue curItemsName = unlockedItems[i];
			RValue curItem = g_ModuleInterface->CallBuiltin("ds_map_find_value", { itemsMap, curItemsName });
			RValue optionIconNormal = getInstanceVariable(curItem, GML_optionIcon_Normal);
			RValue maxLevel = getInstanceVariable(curItem, GML_maxLevel);
			RValue optionIconSuper = getInstanceVariable(curItem, GML_optionIcon_Super);
			bool canSuper = optionIconSuper.m_Kind != VALUE_UNDEFINED && static_cast<int>(optionIconSuper.AsReal()) != sprBulletTempIndex;
			int curLevel = static_cast<int>(lround(getInstanceVariable(curItem, GML_level).m_Real)) + 1;
			if (canSuper)
			{
				bool isSuper = curLevel >= static_cast<int>(lround(maxLevel.m_Real) + 1);
				itemDataList.push_back(sandboxShopItemData(std::string(curItemsName.AsString()), static_cast<int>(lround(optionIconSuper.AsReal())), isSuper, 1, true, SANDBOXITEMTYPE_Item));
				if (isSuper)
				{
					curLevel = 0;
				}
			}
			itemDataList.push_back(sandboxShopItemData(std::string(curItemsName.AsString()), static_cast<int>(lround(optionIconNormal.AsReal())), curLevel, lround(maxLevel.m_Real), false, SANDBOXITEMTYPE_Item));
		}

		RValue availableStickers = g_ModuleInterface->CallBuiltin("variable_global_get", { "availableStickers" });
		RValue stickersMap = getInstanceVariable(Self, GML_STICKERS);
		RValue stickersKeysArr = g_ModuleInterface->CallBuiltin("ds_map_keys_to_array", { stickersMap });
		int stickersKeysLength = static_cast<int>(g_ModuleInterface->CallBuiltin("array_length", { stickersKeysArr }).m_Real);
		for (int i = 0; i < stickersKeysLength; i++)
		{
			RValue curStickerName = stickersKeysArr[i];
			RValue curSticker = g_ModuleInterface->CallBuiltin("ds_map_find_value", { stickersMap, curStickerName });
			RValue optionIcon = getInstanceVariable(curSticker, GML_optionIcon);
			int level = static_cast<int>(!g_ModuleInterface->CallBuiltin("array_contains", { availableStickers, curStickerName }).AsBool());
			itemDataList.push_back(sandboxShopItemData(std::string(curStickerName.AsString()), static_cast<int>(lround(optionIcon.AsReal())), level, 1, false, SANDBOXITEMTYPE_Stamp));
		}
		
		// Handle drawing sandbox item menu
		for (int i = curSandboxMenuPage * 16; i < itemDataList.size() && i < (curSandboxMenuPage + 1) * 16; i++)
		{
			std::string curWeaponName = itemDataList[i].itemName;
			int maxLevel = itemDataList[i].maxLevel;
			int optionIcon = itemDataList[i].optionIcon;
			int curLevel = itemDataList[i].curLevel;
			int rowIndex = (i - curSandboxMenuPage * 16) / 4;
			int colIndex = i % 4;
			int curXPos = 279 + colIndex * 90;
			int curYPos = 80 + rowIndex * 45;

			g_ModuleInterface->CallBuiltin("draw_set_alpha", { 0.7 });
			g_ModuleInterface->CallBuiltin("draw_set_color", { 0 });
			g_ModuleInterface->CallBuiltin("draw_rectangle", { curXPos - 18, curYPos - 18 + 30, curXPos + 18, curYPos + 18 + 30, false});
			g_ModuleInterface->CallBuiltin("draw_set_alpha", { 1 });
			g_ModuleInterface->CallBuiltin("draw_sprite", { sprShopItemBGIndex, 0, curXPos, curYPos + 30 });
			g_ModuleInterface->CallBuiltin("draw_sprite", { optionIcon, 0, curXPos, curYPos + 30 });
			g_ModuleInterface->CallBuiltin("draw_sprite", { sprShopIconIndex, 0, curXPos, curYPos + 30 });
			g_ModuleInterface->CallBuiltin("draw_sprite", { sprShopLevelsEmptyIndex, maxLevel, curXPos, curYPos + 30 });
			g_ModuleInterface->CallBuiltin("draw_sprite", { sprShopLevelsFilledIndex, curLevel, curXPos, curYPos + 30 });

			RValue** args = new RValue*[4];
			RValue mouseOverType = "shopIcon";
			RValue curButtonX = curXPos;
			RValue curButtonY = curYPos + 30;
			args[0] = &mouseOverType;
			args[1] = &curButtonX;
			args[2] = &curButtonY;
			RValue returnVal;
			origMouseOverButtonScript(Self, nullptr, returnVal, 3, args);

			cursorLifetime++;
			if (cursorLifetime >= 62832)
			{
				cursorLifetime = 0;
			}

			if (selectedItemIndex == -1 && returnVal.AsBool() || selectedItemIndex == i)
			{
				g_ModuleInterface->CallBuiltin("draw_sprite", { sprShopIconSelectedIndex, 0, curXPos, curYPos + 30 });
				g_ModuleInterface->CallBuiltin("draw_set_alpha", { .05 + abs(.15 * sin(cursorLifetime / 100.0)) });
				g_ModuleInterface->CallBuiltin("draw_rectangle_color", { curXPos - 21, curYPos - 21 + 30, curXPos + 65, curYPos + 21 + 30, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0 });
				g_ModuleInterface->CallBuiltin("draw_set_alpha", { 1 });
			}

			if (selectedItemIndex == -1 && returnVal.AsBool())
			{
				RValue actionOnePressed = getInstanceVariable(inputManager, GML_actionOnePressed);
				RValue isIconSelected = g_ModuleInterface->CallBuiltin("mouse_check_button_pressed", { 1 });
				if (actionOnePressed.AsBool() || isIconSelected.AsBool())
				{
					selectedItemIndex = i;
					buyOptionIndex = 0;
				}
			}
		}

		int selectedColor[2] = { 0xFFFFFF, 0 };
		if (selectedItemIndex != -1)
		{
			RValue textContainer = g_ModuleInterface->CallBuiltin("variable_global_get", { "TextContainer" });
			RValue shopItemButtons = getInstanceVariable(textContainer, GML_shopItemButtons);
			RValue selectedLanguage = getInstanceVariable(shopItemButtons, GML_selectedLanguage);
			for (int i = 0; i < 2; i++)
			{
				if (i == 1 && itemDataList[selectedItemIndex].itemType == SANDBOXITEMTYPE_Stamp)
				{
					break;
				}
				int buttonPosX = 400 + i * 90;
				int buttonPosY = 310;
				int curColor = selectedColor[static_cast<int>(buyOptionIndex == i)];
				g_ModuleInterface->CallBuiltin("draw_sprite_ext", { sprHudShopButtonIndex, buyOptionIndex == i, buttonPosX, buttonPosY, 1, 1, 0, 0xFFFFFF, 1 });
				g_ModuleInterface->CallBuiltin("draw_set_halign", { 1 });
				g_ModuleInterface->CallBuiltin("draw_text_color", { buttonPosX, buttonPosY - 5, selectedLanguage[i], curColor, curColor, curColor, curColor, 1 });

				RValue** args = new RValue*[4];
				RValue mouseOverType = "short";
				RValue curButtonX = buttonPosX;
				RValue curButtonY = buttonPosY;
				args[0] = &mouseOverType;
				args[1] = &curButtonX;
				args[2] = &curButtonY;
				RValue returnVal;
				origMouseOverButtonScript(Self, nullptr, returnVal, 3, args);
				if (returnVal.AsBool())
				{
					buyOptionIndex = i;
					RValue actionOnePressed = getInstanceVariable(inputManager, GML_actionOnePressed);
					RValue isIconSelected = g_ModuleInterface->CallBuiltin("mouse_check_button_pressed", { 1 });
					if (actionOnePressed.AsBool() || isIconSelected.AsBool())
					{
						handleSandboxItemMenuInteract(Self, itemDataList[selectedItemIndex], buyOptionIndex);
					}
				}
			}
		}

		// Draw Next and Prev buttons
		for (int i = 0; i < 2; i++)
		{
			if (i == 0 && curSandboxMenuPage == 0 || i == 1 && curSandboxMenuPage == (itemDataList.size() - 1) / 16)
			{
				continue;
			}
			int buttonPosX = 300 + i * 270;
			int buttonPosY = 310;
			bool isButtonMouseOver = false;
			RValue** args = new RValue*[4];
			RValue mouseOverType = "short";
			RValue curButtonX = buttonPosX;
			RValue curButtonY = buttonPosY;
			args[0] = &mouseOverType;
			args[1] = &curButtonX;
			args[2] = &curButtonY;
			RValue returnVal;
			origMouseOverButtonScript(Self, nullptr, returnVal, 3, args);
			if (returnVal.AsBool())
			{
				isButtonMouseOver = true;
				RValue actionOnePressed = getInstanceVariable(inputManager, GML_actionOnePressed);
				RValue isIconSelected = g_ModuleInterface->CallBuiltin("mouse_check_button_pressed", { 1 });
				if (actionOnePressed.AsBool() || isIconSelected.AsBool())
				{
					if (i == 0)
					{
						curSandboxMenuPage--;
						selectedItemIndex = -1;
					}
					else if (i == 1)
					{
						curSandboxMenuPage++;
						selectedItemIndex = -1;
					}
				}
			}

			int curColor = selectedColor[static_cast<int>(isButtonMouseOver)];
			std::string buttonText = i == 0 ? "Prev" : "Next";
			g_ModuleInterface->CallBuiltin("draw_sprite_ext", { sprHudShopButtonIndex, isButtonMouseOver, buttonPosX, buttonPosY, 1, 1, 0, 0xFFFFFF, 1 });
			g_ModuleInterface->CallBuiltin("draw_set_halign", { 1 });
			g_ModuleInterface->CallBuiltin("draw_text_color", { buttonPosX, buttonPosY - 5, buttonText, curColor, curColor, curColor, curColor, 1 });
		}

		// Handle sandbox option menu
		for (int i = 0; i < sandboxOptionList.size(); i++)
		{
			int xPos = sandboxOptionList[i]->xPos;
			int yPos = sandboxOptionList[i]->yPos;
			bool isButtonMouseOver = false;
			RValue** args = new RValue*[4];
			RValue mouseOverType = "long";
			RValue curButtonX = xPos + 90;
			RValue curButtonY = yPos;
			RValue screenSize = 2;
			args[0] = &mouseOverType;
			args[1] = &curButtonX;
			args[2] = &curButtonY;
			args[3] = &screenSize;
			RValue returnVal;
			origMouseOverButtonScript(Self, nullptr, returnVal, 4, args);
			if (returnVal.AsBool())
			{
				isButtonMouseOver = true;
				RValue actionOnePressed = getInstanceVariable(inputManager, GML_actionOnePressed);
				RValue isIconSelected = g_ModuleInterface->CallBuiltin("mouse_check_button_pressed", { 1 });
				if (actionOnePressed.AsBool() || isIconSelected.AsBool())
				{
					if (sandboxOptionList[i]->sandboxMenuDataType == SANDBOXMENUDATATYPE_CheckBox)
					{
						sandboxCheckBox* checkBox = reinterpret_cast<sandboxCheckBox*>(sandboxOptionList[i]);
						checkBox->isChecked = !checkBox->isChecked;
						// Enable Debug option
						if (i == 2)
						{
							g_ModuleInterface->CallBuiltin("variable_global_set", { "debug", checkBox->isChecked });
						}
					}
					else if (sandboxOptionList[i]->sandboxMenuDataType == SANDBOXMENUDATATYPE_Button)
					{
						sandboxButton* button = reinterpret_cast<sandboxButton*>(sandboxOptionList[i]);
						button->onClickFunc();
					}
				}
			}
			int curColor = selectedColor[static_cast<int>(isButtonMouseOver)];
			g_ModuleInterface->CallBuiltin("draw_sprite_ext", { sprHudOptionButtonIndex, isButtonMouseOver, xPos + 90, yPos, 1, 1, 0, 0xFFFFFF, 1 });
			if (sandboxOptionList[i]->sandboxMenuDataType == SANDBOXMENUDATATYPE_CheckBox)
			{
				sandboxCheckBox* checkBox = reinterpret_cast<sandboxCheckBox*>(sandboxOptionList[i]);
				g_ModuleInterface->CallBuiltin("draw_sprite_ext", { sprHudToggleButtonIndex, isButtonMouseOver * 2 + checkBox->isChecked, xPos + 10 + 11, yPos + 11 + 2, 1, 1, 0, 0xFFFFFF, 1 });
			}
			g_ModuleInterface->CallBuiltin("draw_text_color", { xPos + 100, yPos + 9, sandboxOptionList[i]->text, curColor, curColor, curColor, curColor, 1});
		}
	}

	if (!paused.AsBool() && !isInSandboxMenu)
	{
		sandboxCheckBox* checkBox = reinterpret_cast<sandboxCheckBox*>(sandboxOptionList[1]);
		if (checkBox->isChecked)
		{
			if (dpsNumFrameCount >= 10)
			{
				dpsNumFrameCount = 0;
				damageList.clear();
				for (auto& damagePerFramePair : damagePerFrameMap)
				{
					double totalDamage = 0;
					int count = 0;
					for (auto damage : damagePerFramePair.second.damageQueue)
					{
						totalDamage += damage;
						count++;
					}
					damageList.push_back(std::pair<double, std::string>(totalDamage / count * 60, damagePerFramePair.first));
				}
				sort(damageList.begin(), damageList.end());
			}

			for (int i = 0; i < 8 && i < damageList.size(); i++)
			{
				int curIndex = static_cast<int>(damageList.size()) - i - 1;
				int spriteIndex = getSpriteIndexFromAttackName(damageList[curIndex].second);
				g_ModuleInterface->CallBuiltin("draw_sprite", { spriteIndex, 0, 30, 110 + i * 30 });
				g_ModuleInterface->CallBuiltin("draw_set_halign", { 0 });
				g_ModuleInterface->CallBuiltin("draw_text_color", { 30 + 30, 110 + i * 30, std::format("{:.5e}", damageList[curIndex].first), 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 1 });
				if (spriteIndex == sprUnknownIconButtonIndex)
				{
					std::string text = "UNKNOWN ID";
					if (!damageList[curIndex].second.empty())
					{
						text = damageList[curIndex].second;
					}
					g_ModuleInterface->CallBuiltin("draw_text_color", { 30 + 100, 110 + i * 30, text, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 1 });
				}
			}
		}
	}
	dpsNumFrameCount++;
}

void PlayerMouse54Before(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (isInSandboxMenu)
	{
		callbackManagerInterfacePtr->CancelOriginalFunction();
	}
}

void PlayerManagerAlarm0Before(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	g_ModuleInterface->CallBuiltin("variable_global_set", { "currentRunMoneyGained", 0 });
	g_ModuleInterface->CallBuiltin("variable_global_set", { "haluLevel", 0 });
}

void BaseMobDrawAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	sandboxCheckBox* checkBox = reinterpret_cast<sandboxCheckBox*>(sandboxOptionList[3]);
	if (checkBox->isChecked)
	{
		CInstance* Self = std::get<0>(Args);
		RValue xPos = getInstanceVariable(Self, GML_x);
		RValue yPos = getInstanceVariable(Self, GML_y);
		RValue curHP = getInstanceVariable(Self, GML_currentHP);
		RValue maxHP = getInstanceVariable(Self, GML_HP);
		RValue atk = getInstanceVariable(Self, GML_ATK);
		RValue spd = getInstanceVariable(Self, GML_SPD);
		std::string text = std::format("HP: {} / {}", static_cast<int>(lround(curHP.AsReal())), static_cast<int>(lround(maxHP.AsReal())));
		g_ModuleInterface->CallBuiltin("draw_set_halign", { 1 });
		drawTextOutline(Self, xPos.AsReal(), yPos.AsReal() + 5, text, 1, 0x000000, 14, 0, 400, 0xFFFFFF, 1);
		text = std::format("ATK: {}", static_cast<int>(lround(atk.AsReal())));
		drawTextOutline(Self, xPos.AsReal(), yPos.AsReal() + 15, text, 1, 0x000000, 14, 0, 400, 0xFFFFFF, 1);
		text = std::format("SPD: {}", static_cast<int>(lround(spd.AsReal())));
		drawTextOutline(Self, xPos.AsReal(), yPos.AsReal() + 25, text, 1, 0x000000, 14, 0, 400, 0xFFFFFF, 1);
	}
}