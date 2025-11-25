// =============================================
// AllySlimeHoming.cpp
// =============================================
#include "AllySlimeHoming.h"
#include "Player.h"
#include "EnemyManager.h"
#include "ProjectileHoming.h"
#include "ModelManager.h"
#include "Collision.h"
#include "GimmicManager.h"       // ギミック管理
#include "Gimmic_BreakWall.h"    // 壁クラス
#include "Core.h"                // コアクラス          
#include <cfloat>
#include <cmath>

using namespace DirectX;

// 演算子オーバーロード（ベクトル計算用）
static inline XMFLOAT3 operator+(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x + b.x,a.y + b.y,a.z + b.z }; }
static inline XMFLOAT3 operator-(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x - b.x,a.y - b.y,a.z - b.z }; }
static inline XMFLOAT3 operator*(const XMFLOAT3& a, float s) { return { a.x * s,a.y * s,a.z * s }; }

AllySlimeHoming::AllySlimeHoming(int formationIndex)
    : index(formationIndex)
{
    slimeModel = ModelManager::Instance().Load("Data/Model/Slime/Slime_R.mdl");

    scale = { 0.002f, 0.002f, 0.002f };
    radius = 0.5f;
    height = 1.0f;

    const Player& ref = (leader ? *leader : Player::Instance());
    position = ref.GetPosition();
    UpdateTransform();
}

void AllySlimeHoming::UpdateAnchor()
{
    const Player& ref = (leader ? *leader : Player::Instance());
    const XMFLOAT3& p = ref.GetPosition();
    const XMFLOAT3& a = ref.GetAngle();

    XMFLOAT3 fwd = { std::sinf(a.y), 0.0f,  std::cosf(a.y) };
    XMFLOAT3 rgt = { std::cosf(a.y), 0.0f, -std::sinf(a.y) };

    const int   row = index / rowWidth;
    const int   col = index % rowWidth;
    const float rightOffset = (col - (rowWidth - 1) * 0.5f) * lateralSpacing;
    const float backOffset = (row + 1) * followDistance;

    anchor = p + (rgt * rightOffset) + (fwd * (-backOffset));
    anchor.y = p.y;
}

// ----------------------------------------------------------------------------
// ターゲット検索：敵だけでなく壁やコアも候補に入れる
// ----------------------------------------------------------------------------
void AllySlimeHoming::AutoAttackUpdate(float elapsedTime)
{
    if (!autoAttackEnabled) return;

    if (autoAttackTimer > 0.0f) {
        autoAttackTimer -= elapsedTime;
        return;
    }

    // --- ターゲット検索開始 ---
    EnemyManager& em = EnemyManager::Instance();
    const int enemyCount = em.GetEnemyCount();

    // 発射位置（スライムの中心あたり）
    const XMFLOAT3 muzzle = { position.x, position.y + height * 0.5f, position.z };
    const float rangeSq = autoAttackRange * autoAttackRange;

    float bestDistSq = FLT_MAX;
    XMFLOAT3 bestTarget = { 0, 0, 0 };
    bool hasTarget = false;

    // 1. 敵を検索
    for (int i = 0; i < enemyCount; ++i) {
        auto e = em.GetEnemy(i);
        if (!e) continue;
        XMFLOAT3 q = e->GetPosition(); q.y += e->GetHeight() * 0.5f;
        float dx = q.x - muzzle.x, dy = q.y - muzzle.y, dz = q.z - muzzle.z;
        float d2 = dx * dx + dy * dy + dz * dz;

        // 射程内かつ、今までで一番近ければ更新
        if (d2 <= rangeSq && d2 < bestDistSq) {
            bestDistSq = d2;
            bestTarget = q;
            hasTarget = true;
        }
    }

    // 2. ギミック（壁・コア）を検索
    // ※ 敵よりも壁が近ければ、ターゲットを壁に上書きします
    GimmicManager& gm = GimmicManager::Instance();
    auto& gimmicks = gm.GetAll();
    for (auto& gimmic : gimmicks) {
        if (!gimmic || !gimmic->IsActive()) continue;

        bool isTarget = false;
        float targetHeight = 0.0f;

        std::shared_ptr<GimmicBase> gimmicPtr = gimmic;

        // 壁 (Gimmic_BreakWall) の判定
        if (gimmicPtr->class_name == "Gimmic_BreakWall") {
            Gimmic_BreakWall* breakWall = dynamic_cast<Gimmic_BreakWall*>(gimmicPtr.get());
            if (breakWall && !breakWall->IsBroken()) {
                isTarget = true;
                targetHeight = gimmicPtr->scale.y * 2.0f; // 見た目の高さを考慮
            }
        }
        // コア (Core) の判定
        else if (gimmicPtr->class_name == "Core") {
            Core* core = dynamic_cast<Core*>(gimmicPtr.get());
            if (core && core->GetHP() > 0.0f) {
                isTarget = true;
                targetHeight = 7.0f; // Core.cpp で Cylinder height=7.0 と定義されている
            }
        }

        if (isTarget) {
            const XMFLOAT3& gp = gimmicPtr->position;
            float dx = gp.x - muzzle.x;
            float dy = (gp.y + targetHeight * 0.5f) - muzzle.y; // 中心を狙う
            float dz = gp.z - muzzle.z;
            const float distSq = dx * dx + dy * dy + dz * dz;

            // 射程内 かつ 今までの最有力候補（敵含む）より近ければ更新
            if (distSq <= rangeSq && distSq < bestDistSq) {
                bestDistSq = distSq;
                bestTarget = gp;
                bestTarget.y += targetHeight * 0.5f;
                hasTarget = true;
            }
        }
    }

    // ターゲットがいなければ終了
    if (!hasTarget) return;

    // 発射処理
    XMFLOAT3 dir = { bestTarget.x - muzzle.x, bestTarget.y - muzzle.y, bestTarget.z - muzzle.z };
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (len < 0.001f) return;
    dir.x /= len; dir.y /= len; dir.z /= len;

    auto* proj = new ProjectileHoming(&projectileManager);
    proj->Launch(dir, muzzle, bestTarget);

    autoAttackTimer = autoAttackInterval;
}

// ----------------------------------------------------------------------------
// 衝突判定：敵・壁・コアすべてと判定を行う
// ----------------------------------------------------------------------------
void AllySlimeHoming::CollisionProjectilesVsEnemies()
{
    EnemyManager& em = EnemyManager::Instance();
    GimmicManager& gm = GimmicManager::Instance();

    const int projectileCount = projectileManager.GetProjectileCount();
    const int enemyCount = em.GetEnemyCount();
    auto& gimmicks = gm.GetAll();

    for (int i = 0; i < projectileCount; ++i) {
        Projectile* projectile = projectileManager.GetProjectile(i);
        if (!projectile) continue;

        bool isHit = false;

        // ---------------------------------------------------
        // 1. 対エネミー (既存の処理)
        // ---------------------------------------------------
        for (int j = 0; j < enemyCount; ++j) {
            auto enemy = em.GetEnemy(j);
            if (!enemy) continue;

            XMFLOAT3 outPos;
            // Sphere vs Cylinder 判定
            const bool hit = Collision::IntersectSphereVsCylinder(
                projectile->GetPosition(), projectile->GetRadius(),
                enemy->GetPosition(), enemy->GetRadius(), enemy->GetHeight(),
                outPos
            );
            if (!hit) continue;

            enemy->ApplyDamage(1, 0.5f);
            projectile->Destroy();
            isHit = true;
            break;
        }
        if (isHit) continue; // 敵に当たったら次の弾へ

        // ---------------------------------------------------
        // 2. 対ギミック (壁・コア)
        // ---------------------------------------------------
        for (auto& gimmic : gimmicks) {
            if (!gimmic || !gimmic->IsActive()) continue;

            std::shared_ptr<GimmicBase> gimmicPtr = gimmic;

            // ★重要: 壁やコアは Type::PlayerAttack でないとダメージが入らない設計のため、
            // 一時的に弾丸のタイプを「攻撃」に変更して衝突を通知する。
            Type originalType = projectile->type;
            projectile->type = Type::PlayerAttack;

            // --- 壁 (OBB) との判定 ---
            if (gimmicPtr->class_name == "Gimmic_BreakWall") {
                Gimmic_BreakWall* breakWall = dynamic_cast<Gimmic_BreakWall*>(gimmicPtr.get());
                // 壊れていない壁のみ
                if (breakWall && !breakWall->IsBroken()) {
                    if (gimmicPtr->collider && gimmicPtr->collider->type == ColliderType::OBB) {
                        OBB* obb = static_cast<OBB*>(gimmicPtr->collider.get());
                        DirectX::XMFLOAT3 outMTD;

                        // Sphere vs OBB 判定
                        if (Collision::IntersectSphereVsOBB(
                            projectile->GetPosition(),
                            projectile->GetRadius(),
                            *obb,
                            &outMTD))
                        {
                            breakWall->OnCollision(projectile); // ダメージ処理
                            projectile->Destroy();
                            isHit = true;
                        }
                    }
                }
            }
            // --- コア (Cylinder) との判定 ---
            else if (gimmicPtr->class_name == "Core") {
                Core* core = dynamic_cast<Core*>(gimmicPtr.get());
                // HPがあるコアのみ
                if (core && core->GetHP() > 0.0f) {
                    if (gimmicPtr->collider && gimmicPtr->collider->type == ColliderType::Cylinder) {
                        CylinderCollider* cylinder = static_cast<CylinderCollider*>(gimmicPtr->collider.get());
                        DirectX::XMFLOAT3 outPos;

                        // Sphere vs Cylinder 判定
                        if (Collision::IntersectSphereVsCylinder(
                            projectile->GetPosition(),
                            projectile->GetRadius(),
                            cylinder->center,
                            cylinder->radius,
                            cylinder->height,
                            outPos))
                        {
                            core->OnCollision(projectile); // ダメージ処理
                            projectile->Destroy();
                            isHit = true;
                        }
                    }
                }
            }

            // タイプを元に戻す（念のため）
            projectile->type = originalType;

            if (isHit) break; // どれかに当たったらループ抜け
        }
    }
}

void AllySlimeHoming::Update(float elapsedTime)
{
    UpdateAnchor();

    float vx = anchor.x - position.x;
    float vz = anchor.z - position.z;
    float d = std::sqrt(vx * vx + vz * vz);
    if (d > 0.0001f) { vx /= d; vz /= d; }
    else { vx = vz = 0.0f; }

    Move(elapsedTime, vx, vz, moveSpeed);
    Turn(elapsedTime, vx, vz, turnSpeed);

    AutoAttackUpdate(elapsedTime);

    UpdateVelocity(elapsedTime);
    UpdateInvincibleTimer(elapsedTime);
    UpdateTransform();

    projectileManager.Update(elapsedTime);
    CollisionProjectilesVsEnemies(); // ★ここで全ての判定を実行
}

void AllySlimeHoming::Render(const RenderContext& rc, ModelRenderer* renderer)
{
    renderer->Render(rc, transform, slimeModel, ShaderId::Lambert);
    projectileManager.Render(rc, renderer);
}

void AllySlimeHoming::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
    Character::RenderDebugPrimitive(rc, renderer);
    projectileManager.RenderDebugPrimitive(rc, renderer);

    if (autoAttackEnabled) {
        const XMFLOAT3 center = { position.x, position.y, position.z };
        renderer->RenderCylinder(rc, center, autoAttackRange, height, XMFLOAT4(0, 1, 0, 0.2f));
    }
    renderer->RenderSphere(rc, anchor, 0.15f, XMFLOAT4(1, 1, 0, 1));
}