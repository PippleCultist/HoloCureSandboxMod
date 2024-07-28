#include <Aurie/shared.hpp>
#include <YYToolkit/shared.hpp>
#include "ScriptFunctions.h"
#include "CommonFunctions.h"
#include "CodeEvents.h"

extern CallbackManagerInterface* callbackManagerInterfacePtr;
extern std::vector<sandboxCheckBox> sandboxOptionList;

std::unordered_map<int, damageData> damagePerFrameMap;

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
		// Give enemies shield equal to the amount of damage they will take if immortal enemies is turned on
		if (sandboxOptionList[0].isChecked)
		{
			double damageAmount = g_ModuleInterface->CallBuiltin("round", { *Args[0] }).m_Real;
			RValue attackID = *Args[1];
			RValue attackIDName = getInstanceVariable(attackID, GML_attackID);
			RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
			RValue attackIndex = getInstanceVariable(attackController, GML_attackIndex);
			RValue curWeapon = g_ModuleInterface->CallBuiltin("ds_map_find_value", { attackIndex, attackIDName });
			RValue config = getInstanceVariable(curWeapon, GML_config);
			RValue optionIcon = getInstanceVariable(config, GML_optionIcon);

			int spriteIndex = static_cast<int>(lround(optionIcon.m_Real));
			if (damageAmount > 0)
			{
				setInstanceVariable(Self, GML_shieldHP, getInstanceVariable(Self, GML_shieldHP).m_Real + damageAmount);
			}
			auto damagePerFrameFind = damagePerFrameMap.find(spriteIndex);
			if (damagePerFrameFind == damagePerFrameMap.end())
			{
				damagePerFrameMap[spriteIndex] = damageData();
			}
			damagePerFrameMap[spriteIndex].curFrameDamage += damageAmount;
		}
	}
	return ReturnValue;
}