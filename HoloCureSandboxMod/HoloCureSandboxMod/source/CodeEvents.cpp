#include "CommonFunctions.h"
#include "CodeEvents.h"
#include "ScriptFunctions.h"

extern std::unordered_map<std::string, damageData> damagePerFrameMap;
extern CallbackManagerInterface* callbackManagerInterfacePtr;

bool isInSandboxMenu = false;
bool prevIsTimePaused = false;
bool prevCanControl = false;
bool prevMouseFollowMode = false;
bool isSandboxMenuButtonPressed = false;

bool isTimePausedButtonPressed = false;
int curSandboxMenuPage = 0;
int selectedItemIndex = -1;

enum sandboxItemType
{
	SANDBOXITEMTYPE_Weapon,
	SANDBOXITEMTYPE_Item,
	SANDBOXITEMTYPE_Collab
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

std::vector<sandboxCheckBox> sandboxOptionList = {
	sandboxCheckBox(10, 90, false, "Immortal Enemies"),
	sandboxCheckBox(10, 125, true, "DPS Tracker"),
	sandboxCheckBox(10, 160, false, "Enable Debug")
};

void PlayerManagerStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	CInstance* Self = std::get<0>(Args);
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
	if ((GetAsyncKeyState('Y') & 0xFFFE) != 0)
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
		if ((GetAsyncKeyState('P') & 0xFFFE) != 0)
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
					setInstanceVariable(item, GML_level, static_cast<double>(curItem.maxLevel - 1));
					setInstanceVariable(item, GML_becomeSuper, true);
					setInstanceVariable(item, GML_optionIcon, getInstanceVariable(item, GML_optionIcon_Super));
					RValue UpdatePlayerMethod = getInstanceVariable(playerManagerInstance, GML_UpdatePlayer);
					RValue UpdatePlayerMethodArr = g_ModuleInterface->CallBuiltin("array_create", { RValue(0.0) });
					g_ModuleInterface->CallBuiltin("method_call", { UpdatePlayerMethod, UpdatePlayerMethodArr });
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
		int optionIcon = static_cast<int>(lround(getInstanceVariable(config, GML_optionIcon).m_Real));
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
			itemDataList.push_back(sandboxShopItemData(std::string(curWeaponName.AsString()), static_cast<int>(lround(optionIcon.m_Real)), curLevel, static_cast<int>(lround(maxLevel.m_Real)), false, SANDBOXITEMTYPE_Weapon));
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
			itemDataList.push_back(sandboxShopItemData(std::string(curWeaponName.AsString()), static_cast<int>(lround(optionIcon.m_Real)), curLevel, 1, false, SANDBOXITEMTYPE_Collab));
		}

		RValue itemsMap = getInstanceVariable(Self, GML_ITEMS);
		RValue items = getInstanceVariable(Self, GML_items);
		RValue unlockedItems = g_ModuleInterface->CallBuiltin("ds_map_find_value", { playerSave, "unlockedItems" });
		int unlockedItemsLength = static_cast<int>(g_ModuleInterface->CallBuiltin("array_length", { unlockedItems }).m_Real);
		for (int i = 0; i < unlockedItemsLength; i++)
		{
			RValue curItemsName = unlockedItems[i];
			RValue curItem = g_ModuleInterface->CallBuiltin("ds_map_find_value", { itemsMap, curItemsName });
			RValue optionIcon = getInstanceVariable(curItem, GML_optionIcon);
			RValue maxLevel = getInstanceVariable(curItem, GML_maxLevel);
			RValue optionIconSuper = getInstanceVariable(curItem, GML_optionIcon_Super);
			bool canSuper = optionIconSuper.m_Kind != VALUE_UNDEFINED && static_cast<int>(optionIconSuper.m_Real) != sprBulletTempIndex;
			int curLevel = static_cast<int>(lround(getInstanceVariable(curItem, GML_level).m_Real)) + 1;
			itemDataList.push_back(sandboxShopItemData(std::string(curItemsName.AsString()), static_cast<int>(lround(optionIcon.m_Real)), curLevel, static_cast<int>(lround(maxLevel.m_Real) + canSuper), canSuper, SANDBOXITEMTYPE_Item));
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
			int xPos = sandboxOptionList[i].xPos;
			int yPos = sandboxOptionList[i].yPos;
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
					sandboxOptionList[i].isChecked = !sandboxOptionList[i].isChecked;
					// Enable Debug option
					if (i == 2)
					{
						g_ModuleInterface->CallBuiltin("variable_global_set", { "debug", sandboxOptionList[i].isChecked });
					}
				}
			}
			int curColor = selectedColor[static_cast<int>(isButtonMouseOver)];
			g_ModuleInterface->CallBuiltin("draw_sprite_ext", { sprHudOptionButtonIndex, isButtonMouseOver, xPos + 90, yPos, 1, 1, 0, 0xFFFFFF, 1 });
			g_ModuleInterface->CallBuiltin("draw_sprite_ext", { sprHudToggleButtonIndex, isButtonMouseOver * 2 + sandboxOptionList[i].isChecked, xPos + 10 + 11, yPos + 11 + 2, 1, 1, 0, 0xFFFFFF, 1});
			g_ModuleInterface->CallBuiltin("draw_text_color", { xPos + 100, yPos + 9, sandboxOptionList[i].text, curColor, curColor, curColor, curColor, 1});
		}
	}

	if (!paused.AsBool() && sandboxOptionList[1].isChecked && !isInSandboxMenu)
	{
		std::vector<std::pair<double, std::string>> damageList;
		for (auto& damagePerFramePair : damagePerFrameMap)
		{
			damageData curDamageData = damagePerFramePair.second;
			double totalDamage = 0;
			int count = 0;
			for (auto damage : curDamageData.damageQueue)
			{
				totalDamage += damage;
				count++;
			}
			damageList.push_back(std::pair<double, std::string>(totalDamage / count * 60, damagePerFramePair.first));
		}
		sort(damageList.begin(), damageList.end());
		for (int i = 0; i < 8 && i < damageList.size(); i++)
		{
			int curIndex = static_cast<int>(damageList.size()) - i - 1;
			int spriteIndex = getSpriteIndexFromAttackName(damageList[curIndex].second);
			g_ModuleInterface->CallBuiltin("draw_sprite", { spriteIndex, 0, 30, 110 + i * 30});
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

void PlayerMouse54Before(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (isInSandboxMenu)
	{
		callbackManagerInterfacePtr->CancelOriginalFunction();
	}
}