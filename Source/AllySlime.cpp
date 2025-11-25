//
// AllySlime.cpp
//

#include "AllySlime.h"
#include "Player.h"              // プレイヤー位置や方向を取得して隊列アンカーに反映
#include "EnemyManager.h"        // 敵の取得に使用
#include "ProjectileStraite.h"   // 味方が撃つ直線弾を生成するために使用
#include "ModelManager.h"        // モデル読み込み
#include "Collision.h"           // 各種衝突判定ユーティリティ
#include "GimmicManager.h"        // BreakWallとCoreを取得するため
#include "Gimmic_BreakWall.h"    // BreakWall判定用
#include "Core.h"                // コア判定用
#include <cmath>
#include <cfloat>

using namespace DirectX;

// 以降で使いやすいように XMFLOAT3 の演算子を簡単に定義
static inline XMFLOAT3 operator+(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
static inline XMFLOAT3 operator-(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
static inline XMFLOAT3 operator*(const XMFLOAT3& a, float s) { return { a.x * s, a.y * s, a.z * s }; }
// --- コンストラクタ（必要な初期化をまとめて実施） ---
AllySlime::AllySlime(int formationIndex)
    : index(formationIndex)
{
    // モデルは敵スライムと共通のものを小さくして利用
    slimeModel = ModelManager::Instance().Load("Data/Model/Slime/suraimukari.mdl");
    scale = { 0.002f, 0.002f, 0.002f }; // モデルの縮尺を調整
    radius = 0.5f;                      // 当たり判定半径（敵と共通）
    height = 1.0f;                      // 当たり判定の高さ

    // 初期位置はプレイヤー付近（正確な配置は UpdateAnchor で決定）
    {
        const Player& ref = (leader ? *leader : Player::Instance());
        position = ref.GetPosition();
    }
    UpdateTransform();
}

void AllySlime::UpdateAnchor()
{
    // プレイヤーの位置と向き（Y=ヨー角）を取得
    const Player& ref = (leader ? *leader : Player::Instance());
    const XMFLOAT3& p = ref.GetPosition();
    const XMFLOAT3& a = ref.GetAngle();

    // 座標系前提: 前方向=+Z を基準に、プレイヤー向きから前/右ベクトルを算出
    XMFLOAT3 fwd = { std::sinf(a.y), 0.0f,  std::cosf(a.y) };
    XMFLOAT3 rgt = { std::cosf(a.y), 0.0f, -std::sinf(a.y) };

    // 隊列の index から 縦（row）/横（col）を算出
    const int row = index / rowWidth;  // 縦方向の段（後方に並ぶほど値が大きい）
    const int col = index % rowWidth;  // 横方向の並び（左から右）

    // 中央から左右に配置するため -(rowWidth-1)/2..+(rowWidth-1)/2 にオフセット
    const float rightOffset = (col - (rowWidth - 1) * 0.5f) * lateralSpacing;
    const float backOffset = (row + 1) * followDistance; // プレイヤーの背後に並ぶので +back = -fwd 方向

    // 目標アンカー座標 = プレイヤー位置 + 右方向 * rightOffset - 前方向 * backOffset
    anchor = p + (rgt * rightOffset) + (fwd * (-backOffset));
    anchor.y = p.y; // 高さはプレイヤーと同じ（必要に応じて地形追従などを追加）
}

void AllySlime::AutoAttackUpdate(float elapsedTime)
{
    if (!autoAttackEnabled) return;

    // クールダウン処理
    if (autoAttackTimer > 0.0f) {
        autoAttackTimer -= elapsedTime;
        return;
    }

    // 射程内の敵をサーチ
    EnemyManager& em = EnemyManager::Instance();
    const XMFLOAT3 pos = { position.x, position.y + height * 0.5f, position.z };
    const float rangeSq = autoAttackRange * autoAttackRange;

    float bestDistSq = FLT_MAX;
    XMFLOAT3 bestTarget = { 0, 0, 0 };
    bool hasTarget = false;

    const int enemyCount = em.GetEnemyCount();
    for (int i = 0; i < enemyCount; ++i) {
        std::shared_ptr<Enemy> enemy = em.GetEnemy(i);
        if (!enemy) continue;

        // 敵の中心位置と高さを考慮して距離を算出（Y は中心を合わせる）
        const XMFLOAT3& ep = enemy->GetPosition();
        float dx = ep.x - position.x;
        float dy = (ep.y + enemy->GetHeight() * 0.5f) - (position.y + height * 0.5f);
        float dz = ep.z - position.z;
        const float distSq = dx * dx + dy * dy + dz * dz;

        if (distSq <= rangeSq && distSq < bestDistSq) {
            bestDistSq = distSq;
            bestTarget = ep;
            bestTarget.y += enemy->GetHeight() * 0.5f;
            hasTarget = true;
        }
    }
    GimmicManager& gm = GimmicManager::Instance();
    auto& gimmicks = gm.GetAll();
    for (auto& gimmic : gimmicks) {
        if (!gimmic) continue;
        if (!gimmic->IsActive()) continue;

        bool isTarget = false;
        float targetHeight = 0.0f;

        std::shared_ptr<GimmicBase> gimmicPtr = gimmic; // 参照を保持
        if (!gimmicPtr) continue;

        if (gimmicPtr->class_name == "Gimmic_BreakWall") {
            Gimmic_BreakWall* breakWall = dynamic_cast<Gimmic_BreakWall*>(gimmicPtr.get());
            if (breakWall && !breakWall->IsBroken()) {
                isTarget = true;
                targetHeight = gimmicPtr->scale.y * 2.0f;
            }
        }
        else if (gimmicPtr->class_name == "Core") {
            Core* core = dynamic_cast<Core*>(gimmicPtr.get());
            if (core && core->GetHP() > 0.0f) {
                isTarget = true;
                targetHeight = 7.0f;
            }
        }

        if (isTarget) {
            const XMFLOAT3& gp = gimmicPtr->position;
            float dx = gp.x - position.x;
            float dy = (gp.y + targetHeight * 0.5f) - (position.y + height * 0.5f);
            float dz = gp.z - position.z;
            const float distSq = dx * dx + dy * dy + dz * dz;

            if (distSq <= rangeSq && distSq < bestDistSq) {
                bestDistSq = distSq;
                bestTarget = gp;
                bestTarget.y += targetHeight * 0.5f;
                hasTarget = true;
            }
        }
    }

    if (!hasTarget) return; // 射程内にターゲットがいない

    // 発射位置（スライムの頭上付近）
    

    // 敵との方向ベクトルを計算
    XMFLOAT3 dir = { bestTarget.x - pos.x, bestTarget.y - pos.y, bestTarget.z - pos.z };
    const float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (len < 0.001f) return;
    dir.x /= len; dir.y /= len; dir.z /= len;

    // 弾を生成して発射（ProjectileStraite を利用）
    auto* proj = new ProjectileStraite(&projectileManager); // 管理はメンバの projectileManager が担当
    proj->Launch(dir, pos);

    // クールダウンを再スタート
    autoAttackTimer = autoAttackInterval;
}

void AllySlime::CollisionProjectilesVsEnemies()
{
    // 味方の弾 vs 敵の衝突判定
    EnemyManager& em = EnemyManager::Instance();

    const int projectileCount = projectileManager.GetProjectileCount();
    const int enemyCount = em.GetEnemyCount();

    for (int i = 0; i < projectileCount; ++i) {
        Projectile* projectile = projectileManager.GetProjectile(i);
        if (!projectile) continue; // 既に破棄されたスロットは無視

        for (int j = 0; j < enemyCount; ++j) {
            std::shared_ptr<Enemy> enemy = em.GetEnemy(j);
            if (!enemy) continue;

            XMFLOAT3 outPos;
            // 弾（球）と敵（円柱）の衝突判定: Collision ユーティリティを使用
            const bool hit = Collision::IntersectSphereVsCylinder(
                projectile->GetPosition(),
                projectile->GetRadius(),
                enemy->GetPosition(),
                enemy->GetRadius(),
                enemy->GetHeight(),
                outPos);

            if (!hit) continue;

            // ヒット時にダメージ（無敵 0.5s は Player と合わせる）
            if (enemy->ApplyDamage(1, 0.5f)) {
                // ノックバック（XZ 平面中心）を付与
                XMFLOAT3 impulse{};
                const float power = 10.0f;
                const XMFLOAT3& e = enemy->GetPosition();
                const XMFLOAT3& p = projectile->GetPosition();
                float vx = e.x - p.x;
                float vz = e.z - p.z;
                const float lenXZ = std::sqrt(vx * vx + vz * vz);
                if (lenXZ > 0.0001f) { vx /= lenXZ; vz /= lenXZ; }
                impulse.x = vx * power;
                impulse.y = power * 0.5f; // 上方向にも少し付与
                impulse.z = vz * power;
                enemy->AddImpulse(impulse);
            }

            // 弾は一度当たったら破棄
            projectile->Destroy();
            break;
        }

        // 2) BreakWall と Core との衝突判定
        if (!projectile) continue;

        GimmicManager& gm = GimmicManager::Instance();
        auto& gimmicks = gm.GetAll();
        for (auto& gimmic : gimmicks) {
            if (!gimmic) continue;
            if (!gimmic->IsActive()) continue;

            std::shared_ptr<GimmicBase> gimmicPtr = gimmic; // 共有ポインタを保持
            if (!gimmicPtr) continue;

            if (gimmicPtr->class_name == "Gimmic_BreakWall") {
                Gimmic_BreakWall* breakWall = dynamic_cast<Gimmic_BreakWall*>(gimmicPtr.get());
                if (!breakWall || breakWall->IsBroken()) continue;

                if (gimmicPtr->collider && gimmicPtr->collider->type == ColliderType::OBB) {
                    OBB* obb = static_cast<OBB*>(gimmicPtr->collider.get());
                    XMFLOAT3 outMTD;
                    if (Collision::IntersectSphereVsOBB(
                        projectile->GetPosition(),
                        projectile->GetRadius(),
                        *obb,
                        &outMTD))
                    {
                        breakWall->OnCollision(projectile);
                        projectile->Destroy();
                        break;
                    }
                }
            }
            else if (gimmicPtr->class_name == "Core") {
                Core* core = dynamic_cast<Core*>(gimmicPtr.get());
                if (!core || core->GetHP() <= 0.0f) continue;

                if (gimmicPtr->collider && gimmicPtr->collider->type == ColliderType::Cylinder) {
                    CylinderCollider* cylinder = static_cast<CylinderCollider*>(gimmicPtr->collider.get());
                    XMFLOAT3 outPos;
                    if (Collision::IntersectSphereVsCylinder(
                        projectile->GetPosition(),
                        projectile->GetRadius(),
                        cylinder->center,
                        cylinder->radius,
                        cylinder->height,
                        outPos))
                    {
                        core->OnCollision(projectile);
                        projectile->Destroy();
                        break;
                    }
                }
            }
        }
    }
}

void AllySlime::Update(float elapsedTime)
{
    // 1) 隊列アンカー（プレイヤー周辺の基準位置）を更新
    UpdateAnchor();

    // 2) 目標アンカーへ移動するための方向ベクトルを算出（XZ 平面のみ）
    float vx = anchor.x - position.x;
    float vz = anchor.z - position.z;
    float d = std::sqrt(vx * vx + vz * vz);
    if (d > 0.0001f) { vx /= d; vz /= d; }
    else { vx = vz = 0.0f; }

    // 3) Character API を用いて移動／回転処理を実行
    Move(elapsedTime, vx, vz, moveSpeed);
    Turn(elapsedTime, vx, vz, turnSpeed);

    // 4) 自動攻撃（クールタイム管理やターゲット探索）
    AutoAttackUpdate(elapsedTime);

    // 5) 速度・無敵タイマー・ワールド変換などの更新（Character 側の処理）
    UpdateVelocity(elapsedTime);
    UpdateInvincibleTimer(elapsedTime);
    UpdateTransform();

    // 6) 弾の更新と敵・ギミックとの衝突判定
    projectileManager.Update(elapsedTime);
    CollisionProjectilesVsEnemies();
}

void AllySlime::Render(const RenderContext& rc, ModelRenderer* renderer)
{
    // メインモデルの描画（Lambert シェーダを使用）
    renderer->Render(rc, transform, slimeModel, ShaderId::Lambert);

    // 弾の描画
    projectileManager.Render(rc, renderer);
}

void AllySlime::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
    // Character 基底クラスのデバッグ描画（当たり判定など）
    Character::RenderDebugPrimitive(rc, renderer);

    // 弾のデバッグ描画
    projectileManager.RenderDebugPrimitive(rc, renderer);

    // 射程の可視化（ON のときのみ）: 緑のワイヤーシリンダ
    if (autoAttackEnabled) {
        const XMFLOAT3 center = { position.x, position.y, position.z };
        renderer->RenderCylinder(rc, center, autoAttackRange, height, XMFLOAT4(0, 1, 0, 0.2f));
    }

    // 追従アンカーの目印（黄色い球）
    renderer->RenderSphere(rc, anchor, 0.15f, XMFLOAT4(1, 1, 0, 1));
}
