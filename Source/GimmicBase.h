#pragma once
#include "GameObject.h"

class GimmicBase :public GameObject
{
public:
	GimmicBase() {
		class_name = "GimmicBase";
		type = Type::Gimmic;
	}

	//イベント関数
	virtual void OnTrigger(GameObject* objcts) {};

	//ギミック更新処理
	virtual void Update(float elaspdTime) override {};

protected:
	bool isActive = true;//ギミックの状態
	bool isTriggered = false;//ギミックの発動状態
};

