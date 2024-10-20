#pragma once
#include "ModuleMain.h"
#include <Aurie/shared.hpp>
#include <YYToolkit/shared.hpp>

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
void PlayerManagerDraw64After(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void PlayerMouse54Before(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void PlayerManagerAlarm0Before(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);