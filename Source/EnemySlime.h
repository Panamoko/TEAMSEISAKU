#pragma once

#include "System/Model.h"
#include "Enemy.h"
#include "ProjectileManager.h"
#include "Editor.h"
#include "Animator.h"

class Player;

// スライム
class EnemySlime : public Enemy
{
public:
	EnemySlime(const char* modelPath = "Data/Model/chara/teki.mdl");
	~EnemySlime() override;

	// 更新処理
	void Update(float elapsedTime) override;

	// 描画処理
	void Render(const RenderContext& rc, ModelRenderer* renderer) override;

	//デバッグプリミティブ描画
	void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)override;

	//縄張り設定
	void SetTerritory(const DirectX::XMFLOAT3& origin, float range);

	//死亡したときに呼ばれる
	void OnDead() override;

	//衝突処理
	void OnCollision(GameObject* object) override;


protected:
	//ターゲット位置をランダム設定
	void SetRandomTargerPosition();

	//目標地点へ移動
	void MoveToTarget(float elapsedTime, float moveSpeedRate, float turnSpeedRate);

	//徘徊ステートへ偏移
	void SetWanderState();

	//徘徊ステート更新処理
	void UpdateWanderState(float elapsedTime);

	//待機ステートへ偏移
	void SetIdleState();

	//待機ステート更新処理
	void UpdateIdleState(float elapsedTime);

	//プレイヤー索敵
	Player* SearchPlayer();

	//攻撃ステートへ偏移
	virtual void SetAttackState(Player* target);

	//攻撃ステート更新処理
	virtual void UpdateAttackState(float elapsedTime);



protected:
	//状態一覧
	enum class State
	{
		//徘徊状態
		Wander,
		//待機状態
		Idle,
		//攻撃状態
		Attack
	};


	//std::vector<std::unique_ptr<Model>> models;

	//Model* model = nullptr;
	//現在の状態
	State				state = State::Wander;
	//目標地点
	DirectX::XMFLOAT3	targetPosition = { 0,0,0 };
	//領域範囲
	DirectX::XMFLOAT3	territoryOrigin = { 0,0,0 };
	//領域半径
	float				territoryRange = 10.0f;
	//移動速度
	float				moveSpeed = 2.0f;
	//回転速度
	float				turnSpeed = DirectX::XMConvertToRadians(360);
	//状態維持時間
	float				stateTimer = 0.0f;
	//索敵範囲
	float				searchRange = 5.0f;

	bool isAttackFired = false; // 攻撃判定済みフラグ

	// ターゲット中のプレイヤー
	Player* targetPlayer = nullptr;

	ProjectileManager	projectileManager;

	Animator animator;
private:

	editor game_editor;
	std::vector<std::unique_ptr<SpriteObject>> sprites2d;

	Model* slimeModel = nullptr;
	CylinderCollider* cylinder;
};
