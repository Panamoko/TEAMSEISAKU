//
// AllySlime.cpp
//

#include "AllySlime.h"
#include "Character.h"   // ← ここで入れる（ヘッダからは外した）
#include "Player.h"              // プレイヤー位置・角度を取得して編隊アンカーを出す
#include "EnemyManager.h"        // 敵の取得
#include "ProjectileStraite.h"   // 直進弾（プレイヤーと同じものを使用）
#include "ModelManager.h"        // モデルロード
#include "Collision.h"           // 球×円柱などの当たり判定

#include <cmath>
#include <cfloat>

using namespace DirectX;

// ちょい便利な演算子（XMFLOAT3 は演算子が無いことが多いので自前で用意）
static inline XMFLOAT3 operator+(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
static inline XMFLOAT3 operator-(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
static inline XMFLOAT3 operator*(const XMFLOAT3& a, float s) { return { a.x * s, a.y * s, a.z * s }; }

AllySlime::AllySlime(int formationIndex)
    : index(formationIndex)
{
    // 見た目は敵スライムを流用（EnemySlime と同じパス／スケールに揃える）
    slimeModel = ModelManager::Instance().Load("Data/Model/Slime/suraimukari.mdl");
    scale = { 0.002f, 0.002f, 0.002f }; // モデルが大きい前提のため縮小
    radius = 0.5f;                  // 当たり半径（敵と同等）
    height = 1.0f;                  // 当たり高さ

    // 初期位置はプレイヤーの近く（実際の整列は UpdateAnchor で行う）
    {
        const Player& ref = (leader ? *leader : Player::Instance());
        position = ref.GetPosition();
    }
}

void AllySlime::UpdateAnchor()
{
    // プレイヤーの位置・角度（Y=ヨー角）を取得
    const Player& ref = (leader ? *leader : Player::Instance());
    const XMFLOAT3& p = ref.GetPosition();
    const XMFLOAT3& a = ref.GetAngle();

    // 右手座標系：前方=+Z として、ヨー角から前方/右ベクトルを算出
    XMFLOAT3 fwd = { std::sinf(a.y), 0.0f,  std::cosf(a.y) };
    XMFLOAT3 rgt = { std::cosf(a.y), 0.0f, -std::sinf(a.y) };

    // 自身の index から “行（縦）/列（横）” を決定
    const int row = index / rowWidth;  // 何段目か（奥に行くほど値が大きい）
    const int col = index % rowWidth;  // その段の何列目か（左右）

    // 横方向の中央基準に並べるため、-(rowWidth-1)/2..+(rowWidth-1)/2 に平行移動
    const float rightOffset = (col - (rowWidth - 1) * 0.5f) * lateralSpacing;
    const float backOffset = (row + 1) * followDistance; // プレイヤー“後方”なので +back を -fwd 側に取る

    // 目標アンカー座標：プレイヤー位置 + 右×rightOffset - 前×backOffset
    anchor = p + (rgt * rightOffset) + (fwd * (-backOffset));
    anchor.y = p.y; // 地面基準に固定（段差対応が必要なら別途レイキャストなどを検討）
}

void AllySlime::AutoAttackUpdate(float elapsedTime)
{
    if (!autoAttackEnabled) return;

    // 連射クールダウン
    if (autoAttackTimer > 0.0f) {
        autoAttackTimer -= elapsedTime;
        return;
    }

    // 射程内の“最寄り”の敵を探索
    EnemyManager& em = EnemyManager::Instance();
    const int enemyCount = em.GetEnemyCount();
    if (enemyCount <= 0) return;

    float bestDistSq = FLT_MAX;
    std::shared_ptr<Enemy> bestEnemy = nullptr;

    for (int i = 0; i < enemyCount; ++i) {
        std::shared_ptr<Enemy> enemy = em.GetEnemy(i);
        if (!enemy) continue;

        // 自分中心→敵上半身あたりまでの距離（Yは上半身同士で合わせる）
        const XMFLOAT3& ep = enemy->GetPosition();
        float dx = ep.x - position.x;
        float dy = (ep.y + enemy->GetHeight() * 0.5f) - (position.y + height * 0.5f);
        float dz = ep.z - position.z;
        const float distSq = dx * dx + dy * dy + dz * dz;

        if (distSq <= autoAttackRange * autoAttackRange && distSq < bestDistSq) {
            bestDistSq = distSq;
            bestEnemy = enemy;
        }
    }
    if (!bestEnemy) return; // 射程内にいない

    // 発射位置（胸元くらい）
    const XMFLOAT3 pos = { position.x, position.y + height * 0.5f, position.z };

    // 敵（上半身）方向の正規化ベクトルを算出
    XMFLOAT3 target = bestEnemy->GetPosition();
    target.y += bestEnemy->GetHeight() * 0.5f;
    XMFLOAT3 dir = { target.x - pos.x, target.y - pos.y, target.z - pos.z };
    const float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (len < 0.001f) return;
    dir.x /= len; dir.y /= len; dir.z /= len;

    // 直進弾を生成→発射（Player と同じ ProjectileStraite を使用）
    auto* proj = new ProjectileStraite(&projectileManager); // 管理は自前の projectileManager
    proj->Launch(dir, pos);

    // クールダウン開始
    autoAttackTimer = autoAttackInterval;
}

void AllySlime::CollisionProjectilesVsEnemies()
{
    // 自分の弾 vs 敵の円柱当たり判定
    EnemyManager& em = EnemyManager::Instance();

    const int projectileCount = projectileManager.GetProjectileCount();
    const int enemyCount = em.GetEnemyCount();

    for (int i = 0; i < projectileCount; ++i) {
        Projectile* projectile = projectileManager.GetProjectile(i);
        if (!projectile) continue; // 既に破棄済みスロットなど

        for (int j = 0; j < enemyCount; ++j) {
            std::shared_ptr<Enemy> enemy = em.GetEnemy(j);
            if (!enemy) continue;

            XMFLOAT3 outPos;
            // 球（弾）×円柱（敵）判定：プロジェクトの共通 Collision を使用
            const bool hit = Collision::IntersectSphereVsCylinder(
                projectile->GetPosition(),
                projectile->GetRadius(),
                enemy->GetPosition(),
                enemy->GetRadius(),
                enemy->GetHeight(),
                outPos);

            if (!hit) continue;

            // ヒット：ダメージ（無敵0.5sは Player と揃え）
            if (enemy->ApplyDamage(1, 0.5f)) {
                // 軽いノックバック（XZ 平面基準）
                XMFLOAT3 impulse{};
                const float power = 10.0f;
                const XMFLOAT3& e = enemy->GetPosition();
                const XMFLOAT3& p = projectile->GetPosition();
                float vx = e.x - p.x;
                float vz = e.z - p.z;
                const float lenXZ = std::sqrt(vx * vx + vz * vz);
                if (lenXZ > 0.0001f) { vx /= lenXZ; vz /= lenXZ; }
                impulse.x = vx * power;
                impulse.y = power * 0.5f; // 少し浮かせる
                impulse.z = vz * power;
                enemy->AddImpulse(impulse);
            }

            // 弾は一体に当たったら破棄
            projectile->Destroy();
            break;
        }
    }
}

void AllySlime::Update(float elapsedTime)
{
    // 1) 編隊アンカー（プレイヤー後方の目標）を更新
    UpdateAnchor();

    // 2) 目標アンカーへ移動するための入力ベクトルを算出（XZ のみ）
    float vx = anchor.x - position.x;
    float vz = anchor.z - position.z;
    float d = std::sqrt(vx * vx + vz * vz);
    if (d > 0.0001f) { vx /= d; vz /= d; }
    else { vx = vz = 0.0f; }

    // 3) 既存の Character API を利用して、自然な加速/減速＆旋回で追従
    Move(elapsedTime, vx, vz, moveSpeed);
    Turn(elapsedTime, vx, vz, turnSpeed);

    // 4) 自動攻撃（発射タイミングの管理と弾生成）
    AutoAttackUpdate(elapsedTime);

    // 5) 速度更新・無敵タイマー・ワールド行列の更新（Character 側の標準処理）
    UpdateVelocity(elapsedTime);
    UpdateInvincibleTimer(elapsedTime);
    UpdateTransform();

    // 6) 自分の弾の更新＆敵との当たり判定
    projectileManager.Update(elapsedTime);
    CollisionProjectilesVsEnemies();
}

void AllySlime::Render(const RenderContext& rc, ModelRenderer* renderer)
{
    // 本体モデルの描画（Lambert 等、敵スライムと揃える）
    renderer->Render(rc, transform, slimeModel, ShaderId::Lambert);

    // 自分の弾の描画
    projectileManager.Render(rc, renderer);
}

void AllySlime::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
    // Character 基底のデバッグ当たり（円柱など）
    Character::RenderDebugPrimitive(rc, renderer);

    // 自分の弾のデバッグ描画
    projectileManager.RenderDebugPrimitive(rc, renderer);

    // 射程の可視化（ON時のみ）：緑の半透明円柱
    if (autoAttackEnabled) {
        const XMFLOAT3 center = { position.x, position.y, position.z };
        renderer->RenderCylinder(rc, center, autoAttackRange, height, XMFLOAT4(0, 1, 0, 0.2f));
    }

    // 追従アンカーの目印（黄色の小球）
    renderer->RenderSphere(rc, anchor, 0.15f, XMFLOAT4(1, 1, 0, 1));
}
