#include <Aurie/shared.hpp>
#include <YYToolkit/YYTK_Shared.hpp>
#include "ScriptFunctions.h"
#include "CommonFunctions.h"
#include "CodeEvents.h"

extern CallbackManagerInterface* callbackManagerInterfacePtr;
extern std::vector<sandboxMenuData*> sandboxOptionList;
extern std::deque<trackTakeDamageData> takeDamageDataList;

std::unordered_map<std::string, damageData> damagePerFrameMap;
std::deque<trackTakeDamageData> takeDamageStack;

RValue& CanSubmitScoreFuncBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	ReturnValue.m_Kind = VALUE_BOOL;
	ReturnValue.m_Real = 0;
	callbackManagerInterfacePtr->CancelOriginalFunction();
	return ReturnValue;
}

RValue& ApplyDamageBaseMobCreateBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (getInstanceVariable(Self, GML_isEnemy).ToBoolean())
	{
		double damageAmount = g_ModuleInterface->CallBuiltin("round", { *Args[0] }).m_Real;
		std::string attackID;
		if (numArgs >= 4 && Args[3]->m_Kind == VALUE_STRING)
		{
			attackID = Args[3]->ToString();
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
				attackID = attackIDName.ToString();
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
	exportToJSON(Self);
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

RValue& TakeDamageBaseMobCreateBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	trackTakeDamageData newTakeDamageData;
	RValue timeArr = g_ModuleInterface->CallBuiltin("variable_global_get", { "time" });
	newTakeDamageData.frameNum = ((timeArr[0].ToInt32() * 60 + timeArr[1].ToInt32()) * 60) + timeArr[2].ToInt32();
	std::string attackID = "";
	if (Args[3]->m_Kind == VALUE_STRING)
	{
		attackID = Args[3]->ToString();
	}
	if (attackID.length() == 0)
	{
		RValue attackRValue = *Args[1];
		RValue attackDamageID = getInstanceVariable(attackRValue, GML_attackDamageID);
		if (attackDamageID.m_Kind == VALUE_STRING)
		{
			attackID = attackDamageID.ToString();
		}
		else
		{
			attackID = getInstanceVariable(attackRValue, GML_attackID).ToString();
		}
	}
	newTakeDamageData.attackID = attackID;
	takeDamageStack.push_back(newTakeDamageData);
	return ReturnValue;
}

RValue& TakeDamageBaseMobCreateAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	takeDamageDataList.push_back(takeDamageStack.back());
	takeDamageStack.pop_back();
	return ReturnValue;
}

RValue& ApplyOnHitEffectsBaseMobCreateBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
//	takeDamageStack.back().onHitEffectStack.push_back(applyOnHitEffectWrapper());
	return ReturnValue;
}

RValue& ApplyOnHitEffectsBaseMobCreateAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
//	auto& takeDamageData = takeDamageStack.back();
//	takeDamageData.onHitEffectList.push_back(takeDamageData.onHitEffectStack.back());
//	takeDamageData.onHitEffectStack.pop_back();
	return ReturnValue;
}