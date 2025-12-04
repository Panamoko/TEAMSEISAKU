#include "EnemySlimeMelee.h"
#include "Player.h"
#include "MathUtils.h" 
#include "ProjectileStraite.h"

// コンストラクタ
EnemySlimeMelee::EnemySlimeMelee()
    : EnemySlime("Data/Model/chara/nico3.mdl")
{
    class_name = "EnemySlimeMelee";
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
    animator.Play("NIC_Fwd_Run", true);
}

// 攻撃ステート更新処理
void EnemySlimeMelee::UpdateAttackState(float elapsedTime)
{
    // 1. 現在のターゲットの生存確認
    if (targetCharacter)
    {
        // ターゲットが死んでいたら（HP<=0）、ターゲットを外して即座に再索敵させる
        if (targetCharacter->GetHealth() <= 0)
        {
            targetCharacter = nullptr;
            targetUpdateTimer = 0.0f; // タイマーを0にして下のif文で即座に索敵させる
        }
    }

    // 2. 定期的なターゲット再評価 (0.5秒ごと または ターゲット不在時)
    if (targetUpdateTimer <= 0.0f || !targetCharacter)
    {
        targetUpdateTimer = 0.5f;

        // 新しいターゲットを探す (Ally優先ロジックは SearchTarget 内にある)
        Character* newTarget = SearchTarget();

        if (newTarget)
        {
            // 新しいターゲットが見つかったら更新
            targetCharacter = newTarget;
        }
        else if (!targetCharacter)
        {
            // ターゲットがいなくて、新規も見つからないなら待機へ戻る
            SetIdleState();
            return;
        }
    }

    // 念のためターゲット有効チェック
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
        animator.Play("NIC_Fwd_Run", true);
        MoveToTarget(elapsedTime, 1.0f, 1.0f);
        isAttackFired = false;
    }
    else
    {
        // 範囲内：攻撃
        animator.Play("NIC_Attack", true);
        MoveToTarget(elapsedTime, 0.0f, 1.0f);

        float currentAnimTime = animator.GetCurrentSeconds();
        float fireTimingSeconds = 12.0f / 30.0f;

        if (!isAttackFired && currentAnimTime >= fireTimingSeconds)
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

        if (currentAnimTime < fireTimingSeconds && isAttackFired)
        {
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