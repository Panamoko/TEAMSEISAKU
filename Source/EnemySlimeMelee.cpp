#include "EnemySlimeMelee.h"
#include "Player.h"
#include "MathUtils.h" 
#include "ProjectileStraite.h"

// コンストラクタ
EnemySlimeMelee::EnemySlimeMelee()
	: EnemySlime("Data/Model/Slime/Slime_R2.mdl")
{
	class_name = "EnemySlimeMelee";

	maxHealth = 40;
	health = maxHealth;

	// 生成直後にすぐ索敵が走るようにタイマーをリセット、または少しずらす
	targetUpdateTimer = MathUtils::RandomRenge(0.0f, 0.2f);
}

// デストラクタ
EnemySlimeMelee::~EnemySlimeMelee()
{
}

// 攻撃ステートへ偏移
void EnemySlimeMelee::SetAttackState(Character* target)
{
	// 基底クラスの処理を呼ぶ
	EnemySlime::SetAttackState(target);
	// ★削除: animator.Play("NIC_Fwd_Run", true);

	// タイマーリセット
	motionTimer = 0.0f;
}

// 攻撃ステート更新処理
void EnemySlimeMelee::UpdateAttackState(float elapsedTime)
{
	// 1. 現在のターゲットの生存確認
	if (targetCharacter)
	{
		if (targetCharacter->GetHealth() <= 0)
		{
			targetCharacter = nullptr;
			targetUpdateTimer = 0.0f;
		}
	}

	// 2. 定期的なターゲット再評価
	if (targetUpdateTimer <= 0.0f || !targetCharacter)
	{
		targetUpdateTimer = 0.5f;
		Character* newTarget = SearchTarget();

		if (newTarget)
		{
			targetCharacter = newTarget;
		}
		else if (!targetCharacter)
		{
			SetIdleState();
			return;
		}
	}

	if (!targetCharacter)
	{
		SetIdleState();
		return;
	}

	// --- 以下、攻撃行動ロジック ---

	targetPosition = targetCharacter->GetPosition();

	// プレイヤーとのXZ平面の距離を計算
	float vx = targetPosition.x - position.x;
	float vz = targetPosition.z - position.z;
	float distSq = vx * vx + vz * vz;

	// 距離に応じた行動分岐
	if (distSq > (attackRange * attackRange))
	{
		// 範囲外：追いかける
		// ★削除: animator.Play("NIC_Fwd_Run", true);

		// 移動できなければ諦めてターゲット解除
		if (!MoveToTarget(elapsedTime, 1.0f, 1.0f))
		{
			targetCharacter = nullptr;
			SetIdleState();
			return;
		}

		isAttackFired = false;
		motionTimer = 0.0f; // 追いかけている間は攻撃モーションタイマーを進めない（またはリセット）
	}
	else
	{
		// 範囲内：攻撃
		// ★削除: animator.Play("NIC_Attack", true);

		// 攻撃中は移動しない（旋回のみ）
		MoveToTarget(elapsedTime, 0.0f, 1.0f);

		// ★修正: タイマーベースの攻撃判定
		motionTimer += elapsedTime;

		const float attackCycle = 1.0f;  // 攻撃間隔
		const float hitTime = 0.4f;      // ダメージ発生タイミング

		if (!isAttackFired && motionTimer >= hitTime)
		{
			const float invincibleTime = 0.5f;
			targetCharacter->ApplyDamage(attackDamage, invincibleTime);

			DirectX::XMFLOAT3 dir;
			dir.x = sinf(angle.y);
			dir.y = 0.0f;
			dir.z = cosf(angle.y);

			DirectX::XMFLOAT3 pos;
			pos.x = position.x;
			pos.y = position.y + height * 0.5f;
			pos.z = position.z;

			ProjectileStraite* projectile = new ProjectileStraite(&projectileManager, "Data/Model/Sword/RedSword.mdl", GameObject::Type::EnemyAttack);
			projectile->type = Type::EnemyAttack;
			projectile->Launch(dir, pos);

			isAttackFired = true;
		}

		// サイクル完了でリセット
		if (motionTimer >= attackCycle)
		{
			motionTimer -= attackCycle;
			isAttackFired = false;
		}
	}

	// 索敵範囲外チェック
	if (distSq > (searchRange * searchRange))
	{
		SetIdleState();
		targetCharacter = nullptr;
	}
}

REGISTER_GAMEOBJECT(EnemySlimeMelee);