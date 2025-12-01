#include "EnemySlimeTurret.h"
#include "ProjectileStraite.h" 
#include "MathUtils.h"    
#include "Player.h"

// コンストラクタ
EnemySlimeTurret::EnemySlimeTurret()
    : EnemySlime("Data/Model/chara/nico2.mdl")
{
    class_name = "EnemySlimeTurret";
    searchRange = 15.0f;
    targetUpdateTimer = MathUtils::RandomRenge(0.0f, 0.2f); // タイマー初期化
    SetIdleState();
}

void EnemySlimeTurret::SetWanderState()
{
    SetIdleState();
}

void EnemySlimeTurret::UpdateIdleState(float elapsedTime)
{
    // 待機中も0.5秒ごとに索敵を行う
    targetUpdateTimer -= elapsedTime;
    if (targetUpdateTimer <= 0.0f)
    {
        targetUpdateTimer = 0.5f;
        if (Character* target = SearchTarget())
        {
            SetAttackState(target);
        }
    }
}

// 攻撃ステートの更新
void EnemySlimeTurret::UpdateAttackState(float elapsedTime)
{
    // 1. ターゲットの生存確認
    if (targetCharacter && targetCharacter->GetHealth() <= 0)
    {
        targetCharacter = nullptr;
        targetUpdateTimer = 0.0f;
    }

    // 2. 再ターゲット (0.5秒ごと)
    if (targetUpdateTimer <= 0.0f || !targetCharacter)
    {
        targetUpdateTimer = 0.5f;
        if (Character* newTarget = SearchTarget())
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

    // 以下、攻撃ロジック
    targetPosition = targetCharacter->GetPosition();

    // 旋回
    MoveToTarget(elapsedTime, 0.0f, 2.0f);

    float currentAnimTime = animator.GetCurrentSeconds();
    float fireTimingSeconds = 12.0f / 30.0f;

    if (!isAttackFired && currentAnimTime >= fireTimingSeconds)
    {
        DirectX::XMFLOAT3 startPos = position;
        startPos.y += height * 0.5f;

        DirectX::XMFLOAT3 targetPos = targetPosition;
        targetPos.y += 1.0f;

        float vx = targetPos.x - startPos.x;
        float vy = targetPos.y - startPos.y;
        float vz = targetPos.z - startPos.z;

        float dist = sqrtf(vx * vx + vy * vy + vz * vz);
        if (dist > 0.0001f)
        {
            vx /= dist; vy /= dist; vz /= dist;
        }

        DirectX::XMFLOAT3 dir = { vx, vy, vz };

        ProjectileStraite* projectile = new ProjectileStraite(&projectileManager, "Data/Model/Sword/RedSword.mdl");
        projectile->type = Type::EnemyAttack;
        projectile->Launch(dir, startPos);

        isAttackFired = true;
    }

    if (currentAnimTime < fireTimingSeconds && isAttackFired)
    {
        isAttackFired = false;
    }

    // 範囲外チェック
    float dx = targetPosition.x - position.x;
    float dy = targetPosition.y - position.y;
    float dz = targetPosition.z - position.z;
    float distSq = dx * dx + dy * dy + dz * dz;

    if (distSq > searchRange * searchRange)
    {
        SetIdleState();
        targetCharacter = nullptr;
    }
}

REGISTER_GAMEOBJECT(EnemySlimeTurret);