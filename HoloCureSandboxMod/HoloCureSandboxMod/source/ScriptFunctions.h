#pragma once
#include "ModuleMain.h"
#include <Aurie/shared.hpp>
#include <YYToolkit/YYTK_Shared.hpp>
#include <queue>

struct damageData
{
	std::deque<double> damageQueue;
	double curFrameDamage;
};

struct onHitEffectData
{
	std::string name;
	double beforeApplyDamage;
	double afterApplyDamage;

	onHitEffectData() : beforeApplyDamage(0), afterApplyDamage(0)
	{
	}

	onHitEffectData(std::string name, double beforeApplyDamage, double afterApplyDamage) : name(name), beforeApplyDamage(beforeApplyDamage), afterApplyDamage(afterApplyDamage)
	{
	}
};

struct trackTakeDamageData
{
	int frameNum = 0;
	std::string attackID;
	
	std::vector<onHitEffectData> onHitEffectList;
};

RValue& CanSubmitScoreFuncBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& ApplyDamageBaseMobCreateBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& GameOverPlayerManagerCreateBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& InitializeCharacterPlayerManagerCreateBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& DoAchievementBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& TakeDamageBaseMobCreateBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& TakeDamageBaseMobCreateAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& ApplyOnHitEffectsBaseMobCreateBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& ApplyOnHitEffectsBaseMobCreateAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);