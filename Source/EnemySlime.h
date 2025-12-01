#pragma once

#include "System/Model.h"
#include "Enemy.h"
#include "ProjectileManager.h"
#include "Editor.h"
#include "Animator.h"
#include "AStar.h"
#include "GridMap.h"

class Player;

// スライム
class EnemySlime : public Enemy
{
public:
	EnemySlime(const char* modelPath = "Data/Model/chara/nico.mdl");
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

	// グリッドマップをセットする関数
	void SetGridMap(const GridMap* map) { gridMap = map; }

protected:
	//ターゲット位置をランダム設定
	void SetRandomTargerPosition();

	// プレイヤーと味方が多いエリアを計算して目標地点を返す
	DirectX::XMFLOAT3 GetStrategicTargetPosition();

	//目標地点へ移動
	void MoveToTarget(float elapsedTime, float moveSpeedRate, float turnSpeedRate);

	// 分離行動（重なり防止）の計算
	void ApplySeparationForce(float elapsedTime);

	//徘徊ステートへ偏移
	virtual void SetWanderState();

	//徘徊ステート更新処理
	void UpdateWanderState(float elapsedTime);

	//待機ステートへ偏移
	void SetIdleState();

	//待機ステート更新処理
	virtual void UpdateIdleState(float elapsedTime);

	//プレイヤー索敵
	Character* SearchTarget();

	//攻撃ステートへ偏移
	virtual void SetAttackState(Character* target);

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
	Character* targetCharacter= nullptr;

	ProjectileManager	projectileManager;

	Animator animator;

	AStar aStar;
	std::vector<std::pair<int, int>> currentPath; // 現在の経路
	int pathIndex = 0; // 次に向かうノードのインデックス
	float pathRecalcTimer = 0.0f; // 再計算用タイマー
	const GridMap* gridMap = nullptr; // マップへの参照

	// ターゲット検索の更新タイマー
	float targetUpdateTimer = 0.0f;
private:

	editor game_editor;
	std::vector<std::unique_ptr<GameSprite>> sprites2d;

	Model* slimeModel = nullptr;
	CylinderCollider* cylinder;

	std::unique_ptr<Model> uniqueModel;
};
