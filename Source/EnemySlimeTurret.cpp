#include "EnemySlimeTurret.h"
#include "ProjectileStraite.h" 
#include "MathUtils.h"    
#include "Player.h"

// コンストラクタ
EnemySlimeTurret::EnemySlimeTurret()
    : EnemySlime("Data/Model/chara/nico2.mdl") // 既存の遠距離モデルを使用（必要なら変えてください）
{
    class_name = "EnemySlimeTurret";

    // 索敵範囲を少し広めにする（高低差があるため）
    searchRange = 15.0f;

    // 最初から待機状態にする
    SetIdleState();
}

// 徘徊ステートへの遷移（無効化）
void EnemySlimeTurret::SetWanderState()
{
    // 徘徊しようとしたら、強制的にIdle（待機）に戻す
    SetIdleState();
}

// 待機ステートの更新（移動ロジックを削除）
void EnemySlimeTurret::UpdateIdleState(float elapsedTime)
{
    // 親クラス(EnemySlime)ではここでタイマー減算してWanderへ移行するが、
    // このクラスでは「索敵」だけを行う。

    // プレイヤー索敵
    if (Player* target = SearchPlayer())
    {
        // 見つかったら攻撃ステートへ
        SetAttackState(target);
    }

    // ※ ここでタイマーによる SetWanderState() を呼ばないことで、永続的に待機させる
}

// 攻撃ステートの更新
void EnemySlimeTurret::UpdateAttackState(float elapsedTime)
{
    // ターゲットがいなくなったら待機へ
    if (!targetPlayer)
    {
        SetIdleState();
        return;
    }

    // ターゲット座標更新
    targetPosition = targetPlayer->GetPosition();

    // 1. 旋回処理（移動速度0.0fで回転だけさせる）
    // 高低差があっても、XZ平面で相手の方を向く必要があります
    MoveToTarget(elapsedTime, 0.0f, 2.0f); // 第3引数は回転速度倍率(少し早めに補正)

    // 2. アニメーションと発射タイミングの制御
    float currentAnimTime = animator.GetCurrentSeconds();
    // 発射タイミング（動画に合わせて調整。例: 12フレーム目）
    float fireTimingSeconds = 12.0f / 30.0f; // 元モデルが30FPSの場合

    // まだ撃っておらず、かつタイミングを超えたら発射
    if (!isAttackFired && currentAnimTime >= fireTimingSeconds)
    {
        // --- 3次元的な発射方向を計算 ---
        DirectX::XMFLOAT3 startPos = position;
        startPos.y += height * 0.5f; // エネミーの腰の高さあたりから

        DirectX::XMFLOAT3 targetPos = targetPosition;
        targetPos.y += 1.0f; // プレイヤーの足元ではなく少し上を狙う

        // ベクトル計算 (Target - Start)
        float vx = targetPos.x - startPos.x;
        float vy = targetPos.y - startPos.y;
        float vz = targetPos.z - startPos.z;

        // 正規化（長さを1にする）
        float dist = sqrtf(vx * vx + vy * vy + vz * vz);
        if (dist > 0.0001f)
        {
            vx /= dist;
            vy /= dist;
            vz /= dist;
        }

        DirectX::XMFLOAT3 dir = { vx, vy, vz };

        // 弾丸生成
        ProjectileStraite* projectile = new ProjectileStraite(&projectileManager, "Data/Model/Sword/RedSword.mdl");
        projectile->type = Type::EnemyAttack;
        projectile->Launch(dir, startPos);

        isAttackFired = true;
    }

    // アニメーションがループした場合のリセット処理
    if (currentAnimTime < fireTimingSeconds && isAttackFired)
    {
        isAttackFired = false;
    }

    // 3. 索敵範囲外チェック（シンプルに距離だけで判定）
    // 振り向き中に勝手にIdleに戻らないよう、角度判定(dot積)は行わない
    float dx = targetPosition.x - position.x;
    float dy = targetPosition.y - position.y;
    float dz = targetPosition.z - position.z;
    float distSq = dx * dx + dy * dy + dz * dz;

    // 範囲外に出たら諦める
    if (distSq > searchRange * searchRange)
    {
        SetIdleState();
        targetPlayer = nullptr;
    }
}

REGISTER_GAMEOBJECT(EnemySlimeTurret);