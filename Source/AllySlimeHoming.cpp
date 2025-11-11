// =============================================
// AllySlimeHoming.cpp
// AllySlime と同一設計の追尾弾スライム（.cpp）
// =============================================
#include "AllySlimeHoming.h"
#include "AllyTargeting.h"
#include "Player.h"
#include "EnemyManager.h"
#include "ProjectileHoming.h"
#include "ModelManager.h"
#include "Collision.h"
#include <cfloat>
#include <cmath>

using namespace DirectX;

// 便利なベクトル演算
static inline XMFLOAT3 operator+(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x + b.x,a.y + b.y,a.z + b.z }; }
static inline XMFLOAT3 operator-(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x - b.x,a.y - b.y,a.z - b.z }; }
static inline XMFLOAT3 operator*(const XMFLOAT3& a, float s) { return { a.x * s,a.y * s,a.z * s }; }

AllySlimeHoming::AllySlimeHoming(int formationIndex)
    : index(formationIndex)
{
    // モデルは暫定で AllySlime と同じ
    slimeModel = ModelManager::Instance().Load("Data/Model/Slime/Slime_R.mdl");

    // AllySlime に合わせた初期プロパティ
    scale = { 0.002f, 0.002f, 0.002f };
    radius = 0.5f;
    height = 1.0f;

    // 初期位置はリーダー（未設定なら Player::Instance()）
    const Player& ref = (leader ? *leader : Player::Instance());
    position = ref.GetPosition();
    UpdateTransform();
}

void AllySlimeHoming::UpdateAnchor()
{
    const Player& ref = (leader ? *leader : Player::Instance());
    const XMFLOAT3& p = ref.GetPosition();
    const XMFLOAT3& a = ref.GetAngle();

    // 前方=+Z、右=+X（AllySlime と同じ前提）
    XMFLOAT3 fwd = { std::sinf(a.y), 0.0f,  std::cosf(a.y) };
    XMFLOAT3 rgt = { std::cosf(a.y), 0.0f, -std::sinf(a.y) };

    const int   row = index / rowWidth;
    const int   col = index % rowWidth;
    const float rightOffset = (col - (rowWidth - 1) * 0.5f) * lateralSpacing;
    const float backOffset = (row + 1) * followDistance; // 背後へ

    anchor = p + (rgt * rightOffset) + (fwd * (-backOffset));
    anchor.y = p.y;
}

void AllySlimeHoming::AutoAttackUpdate(float elapsedTime)
{
    if (!autoAttackEnabled) return;

    if (autoAttackTimer > 0.0f) {
        autoAttackTimer -= elapsedTime;
        return;
    }

    const DirectX::XMFLOAT3 muzzle = { position.x, position.y + height * 0.5f, position.z };

    // Core > BreakWall > Enemy の順で検索
    AllyTargeting::TargetInfo tgt =
        AllyTargeting::FindBestTarget(position, autoAttackRange, &EnemyManager::Instance());
    if (!tgt) return;

    // 初期向き（ターゲット方向へ）
    DirectX::XMFLOAT3 dir = {
        tgt.pos.x - muzzle.x,
        tgt.pos.y - muzzle.y,
        tgt.pos.z - muzzle.z
    };
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (len < 1e-4f) return;
    dir.x /= len; dir.y /= len; dir.z /= len;

    // 追尾弾（ProjectileHoming）を生成し、ターゲット座標を渡す
    auto* p = new ProjectileHoming(&projectileManager);
    p->Launch(dir, muzzle, tgt.pos);  // ← target に向かって旋回する
    projectileManager.Register(p);

    autoAttackTimer = autoAttackInterval;
}


void AllySlimeHoming::CollisionProjectilesVsEnemies()
{
    EnemyManager& em = EnemyManager::Instance();
    const int projectileCount = projectileManager.GetProjectileCount();
    const int enemyCount = em.GetEnemyCount();

    for (int i = 0; i < projectileCount; ++i) {
        Projectile* projectile = projectileManager.GetProjectile(i);
        if (!projectile) continue;

        for (int j = 0; j < enemyCount; ++j) {
            auto enemy = em.GetEnemy(j);
            if (!enemy) continue;

            XMFLOAT3 outPos;
            const bool hit = Collision::IntersectSphereVsCylinder(
                projectile->GetPosition(), projectile->GetRadius(),
                enemy->GetPosition(), enemy->GetRadius(), enemy->GetHeight(),
                outPos
            );
            if (!hit) continue;

            if (enemy->ApplyDamage(1, 0.5f)) {
                // ヒット演出など（必要なら追加）
            }
            // 破棄挙動は Projectile/Manager 既存仕様に従う
        }
    }
}

void AllySlimeHoming::Update(float elapsedTime)
{
    // 1) 隊列アンカー更新
    UpdateAnchor();

    // 2) アンカーへ追従（AllySlime と同じ書式）
    float vx = anchor.x - position.x;
    float vz = anchor.z - position.z;
    float d = std::sqrt(vx * vx + vz * vz);
    if (d > 0.0001f) { vx /= d; vz /= d; }
    else { vx = vz = 0.0f; }

    Move(elapsedTime, vx, vz, moveSpeed);
    Turn(elapsedTime, vx, vz, turnSpeed);

    // 3) 自動攻撃
    AutoAttackUpdate(elapsedTime);

    // 4) Character の標準更新
    UpdateVelocity(elapsedTime);
    UpdateInvincibleTimer(elapsedTime);
    UpdateTransform();

    // 5) 弾＆当たり
    projectileManager.Update(elapsedTime);
    CollisionProjectilesVsEnemies();
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

    // 射程リング
    if (autoAttackEnabled) {
        const XMFLOAT3 center = { position.x, position.y, position.z };
        renderer->RenderCylinder(rc, center, autoAttackRange, height, XMFLOAT4(0, 1, 0, 0.2f));
    }
    // アンカーマーカー
    renderer->RenderSphere(rc, anchor, 0.15f, XMFLOAT4(1, 1, 0, 1));
}
