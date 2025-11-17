#include "EnemySlimeMelee.h"
#include "Player.h"
#include "MathUtils.h" 
#include "ProjectileStraite.h"

// コンストラクタ
EnemySlimeMelee::EnemySlimeMelee()
    : EnemySlime("Data/Model/chara/nico.mdl")
{

    class_name = "EnemySlimeMelee";

    //// モデルロード (基底クラスの処理と同様)
    //model = new Model("Data/Model/chara/nico.mdl");
    //if (model) {
    //    animator.SetModel(model);
    //    animator.Play("NIC_Idle", true);
    //}
}

// デストラクタ
EnemySlimeMelee::~EnemySlimeMelee()
{
   /* if (model) {
        delete model;
        model = nullptr;
    }*/
}

// 攻撃ステートへ偏移
void EnemySlimeMelee::SetAttackState(Player* target)
{
    // 基底クラスの処理を呼ぶ（ターゲット設定など）
    EnemySlime::SetAttackState(target);

    // 遠くにいるかもしれないので、最初は走りモーションにしておくのが自然
    // （Update内で距離判定して即座に切り替わるので、ここは何もしなくても動きますが念のため）
    animator.Play("NIC_Fwd_Run", true);
}

// 攻撃ステート更新処理
void EnemySlimeMelee::UpdateAttackState(float elapsedTime)
{
    // ターゲットが有効かチェック
    if (!targetPlayer)
    {
        SetIdleState();
        return;
    }

    targetPosition = targetPlayer->GetPosition();
    // 1. プレイヤーとのXZ平面の距離を計算
    float vx = targetPosition.x - position.x;
    float vz = targetPosition.z - position.z;
    float distSq = vx * vx + vz * vz;

    // 2. 距離に応じた行動分岐
    if (distSq > (attackRange * attackRange))
    {
        // --- 範囲外：追いかける ---

        // 走りアニメーションを再生
        // (Animator::Playは、違うアニメに切り替わった時だけ時間を0リセットします)
        animator.Play("NIC_Fwd_Run", true);

        // プレイヤーに接近
        MoveToTarget(elapsedTime, 1.0f, 1.0f);

        // 攻撃モーションではないので、攻撃済みフラグは解除しておく
        isAttackFired = false;
    }
    else
    {
        // --- 範囲内：攻撃 ---

        // 攻撃アニメーションを再生
        animator.Play("NIC_Attack", true);

        // 停止して旋回のみ
        MoveToTarget(elapsedTime, 0.0f, 1.0f);

        // === アニメーション同期攻撃ロジック ===

        float currentAnimTime = animator.GetCurrentSeconds();

        // 発射(ダメージ)タイミング設定
        // 30FPSモデルで24フレーム目、60FPSモデルで48フレーム目付近を想定
        // ★動画に合わせて調整してください
        float fireTimingSeconds = 12.0f / 30.0f;

        // まだ攻撃判定を出しておらず、かつタイミングを超えたら実行
        if (!isAttackFired && currentAnimTime >= fireTimingSeconds)
        {
            // 1. プレイヤーに直接ダメージを与える
            const float invincibleTime = 0.5f;
            targetPlayer->ApplyDamage(attackDamage, invincibleTime);

            // 2. 弾丸発射 (エフェクト代わり、あるいは遠距離攻撃も兼ねる場合)
            //    不要なら削除しても構いません
            DirectX::XMFLOAT3 dir;
            dir.x = sinf(angle.y);
            dir.y = 0.0f;
            dir.z = cosf(angle.y);

            DirectX::XMFLOAT3 pos;
            pos.x = position.x;
            pos.y = position.y + height * 0.5f;
            pos.z = position.z;

            ProjectileStraite* projectile = new ProjectileStraite(&projectileManager);
            projectile->Launch(dir, pos);

            // 攻撃済みフラグを立てる
            isAttackFired = true;
        }

        // アニメーションがループして時間が0に戻った場合のフラグ管理
        if (currentAnimTime < fireTimingSeconds && isAttackFired)
        {
            isAttackFired = false;
        }
    }

    // 3. 索敵範囲外チェック (基底クラスのロジック準拠)
    float vy = targetPosition.y - position.y;
    float distSq3D = (vx * vx + vy * vy + vz * vz);

    if (distSq3D > (searchRange * searchRange))
    {

        SetIdleState();
        targetPlayer = nullptr;
    }
}

// ゲームオブジェクトとして登録
REGISTER_GAMEOBJECT(EnemySlimeMelee);