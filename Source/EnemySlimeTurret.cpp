#include "EnemySlimeTurret.h"
#include "ProjectileStraite.h" 
#include "ProjectileTurret.h"
#include "MathUtils.h"    
#include "Player.h"

// コンストラクタ
EnemySlimeTurret::EnemySlimeTurret()
    : EnemySlime("Data/Model/Slime/Slime_G2.mdl")
{
    class_name = "EnemySlimeTurret";
    searchRange = 15.0f;

    maxHealth = 60;
    health = maxHealth;

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

    // 敵へのベクトル
    float dx = targetPosition.x - position.x;
    float dz = targetPosition.z - position.z;
    float distSq = dx * dx + dz * dz;

    // A. 旋回処理
    if (distSq > 0.0001f)
    {
        // 旋回速度を上げる (2.0f -> 5.0f)
        // これにより素早くターゲットの方を向くようになります
        Turn(elapsedTime, dx, dz, turnSpeed * 5.0f);
    }

    // B. 角度チェック (しっかり向いているか？)
    // 現在の正面ベクトル (Character::Turnの仕様に合わせる)
    float fx = sinf(angle.y);
    float fz = cosf(angle.y);

    // ターゲットへの正規化ベクトル
    float dist = sqrtf(distSq);
    float tx = (dist > 0.0f) ? dx / dist : 0.0f;
    float tz = (dist > 0.0f) ? dz / dist : 1.0f;

    // 内積計算 (1.0 = 正面, 0.0 = 真横, -1.0 = 真後ろ)
    float dot = fx * tx + fz * tz;

    // 閾値判定 (0.95 ≒ 角度差18度以内 なら発射許可)
    bool isFacing = (dot > 0.95f);

    // 発射タイミング (0.4秒時点)
    const float fireTiming = 0.4f;

    // まだ向いていない場合、発射タイミング前ならタイマーを進めない
    // これにより「振り向くまで撃たない」挙動になります
    if (!isFacing && motionTimer < fireTiming)
    {
        return;
    }

    // タイマー進行
    motionTimer += elapsedTime;

    const float fireInterval = 1.0f; // 発射間隔

    if (!isAttackFired && motionTimer >= fireTiming)
    {
        // 発射位置の計算
        float forwardOffset = 1.5f;
        float heightOffset = height * 0.6f;
        DirectX::XMFLOAT3 startPos = position;
        startPos.x += sinf(angle.y) * forwardOffset;
        startPos.y += heightOffset;
        startPos.z += cosf(angle.y) * forwardOffset;

        // 初期発射ベクトル（向いている方向）
        DirectX::XMFLOAT3 dir = { sinf(angle.y), 0.0f, cosf(angle.y) };

        // ProjectileTurret を生成
        ProjectileTurret* projectile = new ProjectileTurret(&projectileManager);

        // Launchにターゲットを渡す
        projectile->Launch(dir, startPos, targetCharacter);

        isAttackFired = true;
    }

    // 次のサイクルの準備
    if (motionTimer >= fireInterval)
    {
        motionTimer -= fireInterval;
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