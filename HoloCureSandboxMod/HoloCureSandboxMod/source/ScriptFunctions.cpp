#include <Aurie/shared.hpp>
#include <YYToolkit/shared.hpp>
#include "ScriptFunctions.h"
#include "CommonFunctions.h"
#include "CodeEvents.h"

extern CallbackManagerInterface* callbackManagerInterfacePtr;
extern std::vector<sandboxMenuData*> sandboxOptionList;

std::unordered_map<std::string, damageData> damagePerFrameMap;

RValue& CanSubmitScoreFuncBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	ReturnValue.m_Kind = VALUE_BOOL;
	ReturnValue.m_Real = 0;
	callbackManagerInterfacePtr->CancelOriginalFunction();
	return ReturnValue;
}

RValue& ApplyDamageBaseMobCreateBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (getInstanceVariable(Self, GML_isEnemy).AsBool())
	{
		double damageAmount = g_ModuleInterface->CallBuiltin("round", { *Args[0] }).m_Real;
		std::string attackID;
		if (numArgs >= 4 && Args[3]->m_Kind == VALUE_STRING)
		{
			attackID = std::string(Args[3]->AsString());
		}
		else
		{
			attackID = "";
		}

		if (attackID.empty())
		{
			RValue weaponAttackID = *Args[1];
			RValue attackIDName = getInstanceVariable(weaponAttackID, GML_attackID);
			if (attackIDName.m_Kind == VALUE_STRING)
			{
				attackID = std::string(attackIDName.AsString());
			}
		}

		// Give enemies shield equal to the amount of damage they will take if immortal enemies is turned on
		sandboxCheckBox* checkBox = reinterpret_cast<sandboxCheckBox*>(sandboxOptionList[0]);
		if (checkBox->isChecked)
		{
			if (damageAmount > 0)
			{
				setInstanceVariable(Self, GML_shieldHP, getInstanceVariable(Self, GML_shieldHP).m_Real + damageAmount);
			}
		}
		auto damagePerFrameFind = damagePerFrameMap.find(attackID);
		if (damagePerFrameFind == damagePerFrameMap.end())
		{
			damagePerFrameMap[attackID] = damageData();
		}
		damagePerFrameMap[attackID].curFrameDamage += damageAmount;
	}
	return ReturnValue;
}

RValue& GameOverPlayerManagerCreateBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	g_ModuleInterface->CallBuiltin("variable_global_set", { "currentRunMoneyGained", 0 });
	g_ModuleInterface->CallBuiltin("variable_global_set", { "haluLevel", 0 });
	return ReturnValue;
}

RValue& InitializeCharacterPlayerManagerCreateBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	damagePerFrameMap.clear();
	return ReturnValue;
}

RValue& DoAchievementBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	callbackManagerInterfacePtr->CancelOriginalFunction();
	return ReturnValue;
}