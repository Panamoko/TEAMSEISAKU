#pragma once
#include "GameObject.h"
#include "ModelManager.h"

class GimmicBase :public GameObject
{
public:
	GimmicBase() {
		model = ModelManager::Instance().Load("Data/Model/bilud/takadai.mdl");
		model = ModelManager::Instance().Load("Data/Model/bilud/ie.mdl");

		class_name = "GimmicBase";
		type = Type::Gimmic;
		weight = 5.0f;
	}

	virtual ~GimmicBase() = default;

	//イベント関数
	virtual void OnCollision(GameObject* objcts) override {};
	//ギミック更新処理
	virtual void Update(float elapsedTime) override {};

	//ゲッター
	bool IsActive() const { return isActive; }
	bool IsTriggered() const { return isTriggered; }

	//セッター
	void SetActive(bool active) { isActive = active; }
	void SetTriggered(bool triggered) { isTriggered = triggered; }

protected:
	bool isActive = true;//ギミックの状態
	bool isTriggered = false;//ギミックの発動状態
};

