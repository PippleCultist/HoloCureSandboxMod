#pragma once
#include "ModuleMain.h"
#include <Aurie/shared.hpp>
#include <YYToolkit/shared.hpp>

struct sandboxCheckBox
{
	int xPos;
	int yPos;
	bool isChecked;
	std::string text;

	sandboxCheckBox(int xPos, int yPos, bool isChecked, std::string text) : xPos(xPos), yPos(yPos), isChecked(isChecked), text(text)
	{
	}
};

void PlayerManagerStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void PlayerManagerDraw64After(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void EnemyStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);