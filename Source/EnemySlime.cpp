#include "MathUtils.h"

#include "EnemySlime.h"
#include "ProjectileStraite.h"
#include "Player.h"
#include "Collision.h"
#include <cfloat>

// コンストラクタ
EnemySlime::EnemySlime(const char* modelPath)
{
	class_name = "EnemySlime";

	// ModelManager からスライムモデル取得
	model = ModelManager::Instance().Load(modelPath);
	//models.push_back(std::make_unique<Model>("Data/Model/Slime/Slime.mdl"));

	if (model) {
		animator.SetModel(model);
		animator.Play("NIC_Idle", true); // 最初はIdle
	}

	// モデルが大きいのでスケーリング
	scale.x = scale.y = scale.z = 0.01f;

	//幅・高さ設定
	radius = 0.5f;
	height = 1.0f;

	type = Type::Enemy;
	collider = std::make_unique<CylinderCollider>();
	collider->type = ColliderType::Cylinder;
	collider->owner = this;
	cylinder = static_cast<CylinderCollider*>(collider.get());

	cylinder->height = height;
	cylinder->radius = radius;

	CollisionManager::Instance().AddObject(this);

	if (!objects.empty())
	{
		auto& obj = *objects[0];
		obj.dirty = true;
		if (obj.dirty)
		{
			set_position(obj.position);
			set_rotation(obj.angle);
			set_scale(obj.scale);

			obj.dirty = false;
		}
	}


	//徘徊ステートへ偏移
	SetWanderState();
}

// デストラクタ
EnemySlime::~EnemySlime()
{
	//delete model;
}

// 更新処理
void EnemySlime::Update(float elapsedTime)
{
	cylinder->center = position;

	//ステート毎の更新処理
	switch (state)
	{
	case State::Wander:
		UpdateWanderState(elapsedTime);
		break;
	case State::Idle:
		UpdateIdleState(elapsedTime);
		break;
	case State::Attack:
		UpdateAttackState(elapsedTime);
		break;
	}

	animator.Update(elapsedTime);

	//速力処理更新
	UpdateVelocity(elapsedTime);

	//弾丸更新処理
	projectileManager.Update(elapsedTime);

	// 無敵時間更新
	UpdateInvincibleTimer(elapsedTime);

	if (!objects.empty())
	{
		auto& obj = *objects[0];

		if (obj.dirty)
		{
			set_position(obj.position);
			set_rotation(obj.angle);
			set_scale(obj.scale);

			obj.dirty = false;
		}
	}

	// オブジェクト行列を更新
	UpdateTransform();

	// モデル行列更新
	ModelManager::Instance().UpdateAllTransforms();
}

// 描画処理
void EnemySlime::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	// モデル描画
	
	//for (auto& obj : objects)
	//{
	//	if (!obj || !obj->model) continue;
	//	renderer->Render(rc, transform, model, ShaderId::Lambert);
	//}
	

	renderer->Render(rc, transform, model, ShaderId::Lambert);

	//弾丸描画処理
	projectileManager.Render(rc, renderer);
}

//デバッグプリミティブ描画
void EnemySlime::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
	//基底クラスのデバッグプリミティブ描画
	Enemy::RenderDebugPrimitive(rc, renderer);

	//縄張り範囲をデバッグ円柱描画
	renderer->RenderCylinder(
		rc,
		territoryOrigin,
		territoryRange,
		1.0f,
		DirectX::XMFLOAT4(0, 1, 0, 1));

	//ターゲット位置をデバッグ球描画
	renderer->RenderSphere(
		rc,
		targetPosition,
		1.0f,
		DirectX::XMFLOAT4(1, 1, 0, 1));

	//索敵範囲をデバッグ円柱描画
	renderer->RenderCylinder(
		rc,
		position,
		searchRange,
		1.0f,
		DirectX::XMFLOAT4(1, 0, 0, 1));
}

//縄張り設定
void EnemySlime::SetTerritory(const DirectX::XMFLOAT3& origin, float range)
{
	territoryOrigin = origin;
	territoryRange = range;

	if (state == State::Wander)
	{
		SetRandomTargerPosition();
	}
}

//ターゲット位置をランダム設定
void EnemySlime::SetRandomTargerPosition()
{
	float theta = MathUtils::RandomRenge(-DirectX::XM_PI, DirectX::XM_PI);
	float range = MathUtils::RandomRenge(0.0f, territoryRange);
	targetPosition.x = territoryOrigin.x + sinf(theta) * range;
	targetPosition.y = territoryOrigin.y;
	targetPosition.z = territoryOrigin.z + cosf(theta) * range;
}

//目標地点へ移動
void EnemySlime::MoveToTarget(float elapsedTime, float moveSpeedRate, float turnSpeedRate)
{
	//ターゲット方向への進行ベクトルを算出
	float vx = targetPosition.x - position.x;
	float vz = targetPosition.z - position.z;
	float dist = sqrtf(vx * vx + vz * vz);
	vx /= dist;
	vz /= dist;

	//移動処理
	Move(elapsedTime, vx, vz, moveSpeed * moveSpeedRate);
	Turn(elapsedTime, vx, vz, turnSpeed * turnSpeedRate);
}

//徘徊ステートへ偏移
void EnemySlime::SetWanderState()
{
	state = State::Wander;

	//目標地点設定
	SetRandomTargerPosition();

	animator.Play("NIC_Fwd_Run", true);
}

//徘徊ステート更新処理
void EnemySlime::UpdateWanderState(float elapsedTime)
{
	//目標地点までXZ平面での距離判定
	float vx = targetPosition.x - position.x;
	float vz = targetPosition.z - position.z;
	float distSq = vx * vx + vz * vz;
	if (distSq < radius * radius)
	{
		//待機ステートへ偏移
		SetIdleState();
	}

	//目標地点へ移動
	MoveToTarget(elapsedTime, 1.0f, 1.0f);

	//プレイヤー索敵
	if (Player* target = SearchPlayer()) // 戻り値でターゲットを受け取る
	{
		//見つかったら攻撃ステートへ偏移
		SetAttackState(target); // ターゲットを渡す
	}
}

//待機ステートへ偏移
void EnemySlime::SetIdleState()
{
	state = State::Idle;

	//タイマーをランダム設定
	stateTimer = MathUtils::RandomRenge(3.0f, 5.0f);

	animator.Play("NIC_Idle", true);
}

//待機ステート更新処理
void EnemySlime::UpdateIdleState(float elapsedTime)
{
	//タイマー処理
	stateTimer -= elapsedTime;
	if (stateTimer < 0.0f)
	{
		//徘徊ステートへ偏移
		SetWanderState();
	}

	//プレイヤー索敵
	if (Player* target = SearchPlayer()) // 戻り値でターゲットを受け取る
	{
		//見つかったら攻撃ステートへ偏移
		SetAttackState(target); // ターゲットを渡す
	}
};

//プレイヤー索敵
Player* EnemySlime::SearchPlayer()
{
	const auto& allPlayers = Player::GetAllPlayers();
	Player* foundTarget = nullptr;
	float closestDistSq = FLT_MAX; // 3D距離の2乗

	for (const auto* player : allPlayers)
	{
		if (!player) continue;

		const DirectX::XMFLOAT3& playerPositon = player->GetPosition();
		float vx = playerPositon.x - position.x;
		float vy = playerPositon.y - position.y;
		float vz = playerPositon.z - position.z;
		float distSq = (vx * vx + vy * vy + vz * vz);

		// 範囲内か (2乗で比較)
		if (distSq < searchRange * searchRange)
		{
			// 一番近いプレイヤーを記憶
			if (distSq < closestDistSq)
			{
				closestDistSq = distSq;
				foundTarget = const_cast<Player*>(player);
			}
		}
	}
	return foundTarget;
}

//攻撃ステートへ偏移
void EnemySlime::SetAttackState(Player* target)
{
	state = State::Attack;
	stateTimer = 0.0f;
	targetPlayer = target; // ターゲットを記憶

	// アニメーション再生開始
	animator.Play("NIC_Attack", true);

	// フラグをリセット（まだ撃っていない）
	isAttackFired = false;
}

//攻撃ステート更新処理
void EnemySlime::UpdateAttackState(float elapsedTime)
{
	// ターゲットが有効かチェック (デストラクタで解除されるため)
	if (!targetPlayer)
	{
		SetIdleState();
		return;
	}

	//目標地点を "記憶したターゲットの" 位置に設定
	targetPosition = targetPlayer->GetPosition();

	//目標地点へ移動 (旋回のみ)
	MoveToTarget(elapsedTime, 0.0f, 1.0f);

	// 現在のアニメーション再生時間を取得
	float currentAnimTime = animator.GetCurrentSeconds();

	float fireTimingSeconds = 12.0f / 60.0f;

	// まだ撃っていなくて、かつ再生時間が発射タイミングを超えたら発射
	if (!isAttackFired && currentAnimTime >= fireTimingSeconds)
	{
		// --- 弾丸発射処理 (元のコードを使用) ---
		DirectX::XMFLOAT3 dir;
		dir.x = sinf(angle.y);
		dir.y = 0.0f;
		dir.z = cosf(angle.y);

		DirectX::XMFLOAT3 pos;
		pos.x = position.x;
		pos.y = position.y + height * 0.5f;
		pos.z = position.z;

		ProjectileStraite* projectile = new ProjectileStraite(&projectileManager, "Data/Model/Sword/RedSword.mdl");
		projectile->type = Type::EnemyAttack;
		projectile->Launch(dir, pos);
		// ----------------------------------

		// 「撃った」ことにする（連射防止）
		isAttackFired = true;
	}

	// アニメーションがループして時間が0に戻った場合、フラグをリセットして次弾に備える
	if (currentAnimTime < fireTimingSeconds && isAttackFired)
	{
		// 現在時間が発射時間より手前、かつ発射済みフラグが立っている＝ループした
		isAttackFired = false;
	}

	// ターゲットがまだ範囲内にいるか再チェック
	bool targetLost = true;
	const DirectX::XMFLOAT3& playerPositon = targetPlayer->GetPosition();
	float vx = playerPositon.x - position.x;
	float vy = playerPositon.y - position.y;
	float vz = playerPositon.z - position.z;
	float distSq = (vx * vx + vy * vy + vz * vz);

	if (distSq < searchRange * searchRange) // 範囲内
	{
		float distXZ = sqrtf(vx * vx + vz * vz);
		if (distXZ > 1e-6f) {
			float fvx = vx / distXZ;
			float fvz = vz / distXZ;
			float frontX = sinf(angle.y);
			float frontZ = cosf(angle.y);
			float dot = (frontX * fvx) + (frontZ * fvz);
			if (dot > 0.0f) // 前方
			{
				targetLost = false; // ★ 見失っていない
			}
		}
	}

	//プレイヤーを見失ったら
	if (targetLost)
	{
		//待機ステートへ偏移
		SetIdleState();
		targetPlayer = nullptr; // ★ ターゲットクリア
	}
}

//死亡した時に呼ばれる
void EnemySlime::OnDead()
{
	animator.Play("NIC_Death", false);

	CollisionManager::Instance().Remove(this);

	//自身を破棄
	Destroy();
}

void EnemySlime::OnCollision(GameObject* object)
{
	if (fabsf(mtd.x) > 1e-6f || fabsf(mtd.y) > 1e-6f || fabsf(mtd.z) > 1e-6f)
	{
		position.x += mtd.x; position.y += mtd.y; position.z += mtd.z;
	}
}

REGISTER_GAMEOBJECT(EnemySlime);

