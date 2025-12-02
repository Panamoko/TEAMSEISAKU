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

    // --- 攻撃ロジック ---
    targetPosition = targetCharacter->GetPosition();

    // ★修正1: 敵の方向を向く処理
    // MoveToTargetを使わず、直接ターゲットへの方向を計算してTurnを呼ぶ
    float dx = targetPosition.x - position.x;
    float dz = targetPosition.z - position.z;
    // 距離の2乗
    float distSq = dx * dx + dz * dz;

    if (distSq > 0.0001f)
    {
        // 旋回のみ実行 (turnSpeedは親クラスで定義されているものを使用)
        // 必要に応じて倍率(2.0fなど)を調整して旋回速度を変えてください
        Turn(elapsedTime, dx, dz, turnSpeed * 2.0f);
    }

    float currentAnimTime = animator.GetCurrentSeconds();
    float fireTimingSeconds = 12.0f / 30.0f;

    if (!isAttackFired && currentAnimTime >= fireTimingSeconds)
    {
        // ★修正2: 発射位置の調整
        // 現在の向き(angle.y)に基づいて、少し前方にオフセットさせる
        float forwardOffset = 1.5f; // 前にずらす量（モデルに合わせて調整してください）
        float heightOffset = height * 0.6f; // 高さの調整

        DirectX::XMFLOAT3 startPos = position;

        // 向いている方向(sin, cos)を使って座標をずらす
        startPos.x += sinf(angle.y) * forwardOffset;
        startPos.y += heightOffset;
        startPos.z += cosf(angle.y) * forwardOffset;

        // ターゲット位置（少し上を狙う）
        DirectX::XMFLOAT3 targetPos = targetPosition;
        targetPos.y += 0.5f;

        // 発射ベクトル計算
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
        // 弾を大きくしたい場合はここでスケール設定
        // projectile->SetScale({2.0f, 2.0f, 2.0f}); 
        projectile->Launch(dir, startPos);

        isAttackFired = true;
    }

    if (currentAnimTime < fireTimingSeconds && isAttackFired)
    {
        isAttackFired = false;
    }

    // 範囲外チェック
    if (distSq > searchRange * searchRange)
    {
        SetIdleState();
        targetCharacter = nullptr;
    }
}

REGISTER_GAMEOBJECT(EnemySlimeTurret);