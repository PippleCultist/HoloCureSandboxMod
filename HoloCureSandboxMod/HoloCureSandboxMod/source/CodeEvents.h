#pragma once
#include "ModuleMain.h"
#include <Aurie/shared.hpp>
#include <YYToolkit/YYTK_Shared.hpp>
#include "nlohmann/json.hpp"

using menuFunc = void (*)(void);

enum SandboxMenuDataType
{
	SANDBOXMENUDATATYPE_NONE,
	SANDBOXMENUDATATYPE_Button,
	SANDBOXMENUDATATYPE_CheckBox,
};

struct sandboxMenuData
{
	int xPos;
	int yPos;
	std::string text;
	SandboxMenuDataType sandboxMenuDataType;

	sandboxMenuData(int xPos, int yPos, std::string text, SandboxMenuDataType sandboxMenuDataType) : xPos(xPos), yPos(yPos), text(text), sandboxMenuDataType(sandboxMenuDataType)
	{
	}
};

struct sandboxCheckBox : sandboxMenuData
{
	bool isChecked;

	sandboxCheckBox(int xPos, int yPos, bool isChecked, std::string text) : sandboxMenuData(xPos, yPos, text, SANDBOXMENUDATATYPE_CheckBox),
		isChecked(isChecked)
	{
	}
};

struct sandboxButton : sandboxMenuData
{
	menuFunc onClickFunc;

	sandboxButton(int xPos, int yPos, menuFunc onClickFunc, std::string text) : sandboxMenuData(xPos, yPos, text, SANDBOXMENUDATATYPE_Button),
		onClickFunc(onClickFunc)
	{
	}
};

void framePauseThreadHandler();

void PlayerManagerStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void PlayerManagerStepAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void PlayerManagerDraw64After(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void PlayerMouse54Before(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void PlayerManagerAlarm0Before(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void BaseMobDrawAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);

struct playerStatLevels
{
	int hp;
	int atk;
	int spd;
	int crit;
	int pickupRange;
	int haste;
};

struct playerBuffData
{
	std::string buffName;
	int duration;
	int stacks = -1;
	int maxStacks = -1;
	int buffIcon = -1;
};

struct playerItemData
{
	std::string itemName;
	int level;
	int maxLevel;
	bool canSuper;
	playerItemData() : level(-1), maxLevel(-1), canSuper(false)
	{
	}

	playerItemData(std::string itemName, int level, int maxLevel, bool canSuper) : itemName(itemName), level(level), maxLevel(maxLevel), canSuper(canSuper)
	{
	}
};

struct playerWeaponData
{
	std::string weaponName;
	int level;
	int maxLevel;
	bool isCollab;
	playerWeaponData() : level(-1), maxLevel(-1), isCollab(false)
	{
	}

	playerWeaponData(std::string weaponName, int level, int maxLevel, bool isCollab) : weaponName(weaponName), level(level), maxLevel(maxLevel), isCollab(isCollab)
	{
	}
};

struct gameData
{
	int time;
	std::vector<playerItemData> itemDataList;
	std::vector<playerWeaponData> weaponDataList;
	// stamps
	playerStatLevels statLevels;
	std::vector<playerBuffData> buffDataList;
};

void to_json(nlohmann::json& outputJson, const gameData& inputGameData);
void from_json(const nlohmann::json& inputJson, gameData& outputGameData);
void to_json(nlohmann::json& outputJson, const playerStatLevels& inputStatLevels);
void from_json(const nlohmann::json& inputJson, playerStatLevels& outputStatLevels);
void to_json(nlohmann::json& outputJson, const playerBuffData& inputBuffData);
void from_json(const nlohmann::json& inputJson, playerBuffData& outputBuffData);
void to_json(nlohmann::json& outputJson, const playerWeaponData& inputWeaponData);
void from_json(const nlohmann::json& inputJson, playerWeaponData& outputWeaponData);
void to_json(nlohmann::json& outputJson, const playerItemData& inputItemData);
void from_json(const nlohmann::json& inputJson, playerItemData& outputItemData);