#pragma once
#include "GimmicBase.h"

class Gimmic_BreakWall : public GimmicBase
{
public:
	Gimmic_BreakWall();

	//イベント処理
	void OnTrigger(GameObject* objects) override;

	//ギミック更新処理
	void Update(float elapsedTime)override;

private:
	bool isBroken = false;//壊れたかどうか
	float hp = 2.0f;//耐久度
};

