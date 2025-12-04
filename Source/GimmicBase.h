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
		once = true;
	}

	virtual ~GimmicBase() = default;

	//イベント関数
	virtual void OnCollision(GameObject* objcts) override;

	//ギミック更新処理
	virtual void Update(float elapsedTime) override {};

	//ゲッター
	bool IsActive() const { return isActive; }
	bool IsTriggered() const { return isTriggered; }

	//セッター
	void SetActive(bool active) { isActive = active; }
	void SetTriggered(bool triggered) { isTriggered = triggered; }

	void UpdateInvicible(float elapsedTime);

protected:
	bool isActive = true;//ギミックの状態
	bool isTriggered = false;//ギミックの発動状態
	float invincible_timer;//無敵時間
	float hp = 2.0f; // 耐久値
	float max_hp;
	bool once;
};

