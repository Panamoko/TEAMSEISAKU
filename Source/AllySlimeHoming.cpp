// =============================================
// AllySlimeHoming.cpp
// AllySlime �Ɠ���݌v�̒ǔ��e�X���C���i.cpp�j
// =============================================
#include "AllySlimeHoming.h"
#include "Player.h"
#include "EnemyManager.h"
#include "ProjectileHoming.h"
#include "ModelManager.h"
#include "Collision.h"
#include "GimmicManager.h"        // BreakWallとCoreを取得するため
#include "Gimmic_BreakWall.h"    // BreakWall判定用
#include "Core.h"                // Core判定用
#include "Collider.h"            // ColliderType, OBB, CylinderCollider用
#include <cfloat>
#include <cmath>

using namespace DirectX;

// �֗��ȃx�N�g�����Z
static inline XMFLOAT3 operator+(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x + b.x,a.y + b.y,a.z + b.z }; }
static inline XMFLOAT3 operator-(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x - b.x,a.y - b.y,a.z - b.z }; }
static inline XMFLOAT3 operator*(const XMFLOAT3& a, float s) { return { a.x * s,a.y * s,a.z * s }; }

AllySlimeHoming::AllySlimeHoming(int formationIndex)
    : index(formationIndex)
{
    // ���f���͎b��� AllySlime �Ɠ���
    slimeModel = ModelManager::Instance().Load("Data/Model/Slime/Slime_R.mdl");

    // AllySlime �ɍ��킹�������v���p�e�B
    scale = { 0.002f, 0.002f, 0.002f };
    radius = 0.5f;
    height = 1.0f;

    // �����ʒu�̓��[�_�[�i���ݒ�Ȃ� Player::Instance()�j
    const Player& ref = (leader ? *leader : Player::Instance());
    position = ref.GetPosition();
    UpdateTransform();
}

void AllySlimeHoming::UpdateAnchor()
{
    const Player& ref = (leader ? *leader : Player::Instance());
    const XMFLOAT3& p = ref.GetPosition();
    const XMFLOAT3& a = ref.GetAngle();

    // �O��=+Z�A�E=+X�iAllySlime �Ɠ����O��j
    XMFLOAT3 fwd = { std::sinf(a.y), 0.0f,  std::cosf(a.y) };
    XMFLOAT3 rgt = { std::cosf(a.y), 0.0f, -std::sinf(a.y) };

    const int   row = index / rowWidth;
    const int   col = index % rowWidth;
    const float rightOffset = (col - (rowWidth - 1) * 0.5f) * lateralSpacing;
    const float backOffset = (row + 1) * followDistance; // �w���

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

    EnemyManager& em = EnemyManager::Instance();
    const int enemyCount = em.GetEnemyCount();
    const XMFLOAT3 muzzle = { position.x, position.y + height * 0.5f, position.z };
    const float rangeSq = autoAttackRange * autoAttackRange;

    float bestDistSq = FLT_MAX;
    XMFLOAT3 bestTarget = { 0, 0, 0 };
    bool hasTarget = false;

    // �˒����̍Ŋ��G
    for (int i = 0; i < enemyCount; ++i) {
        auto e = em.GetEnemy(i);
        if (!e) continue;
        XMFLOAT3 q = e->GetPosition(); q.y += e->GetHeight() * 0.5f;
        float dx = q.x - muzzle.x, dy = q.y - muzzle.y, dz = q.z - muzzle.z;
        float d2 = dx * dx + dy * dy + dz * dz;
        if (d2 <= rangeSq && d2 < bestDistSq) {
            bestDistSq = d2;
            bestTarget = q;
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
            float dx = gp.x - muzzle.x;
            float dy = (gp.y + targetHeight * 0.5f) - muzzle.y;
            float dz = gp.z - muzzle.z;
            const float distSq = dx * dx + dy * dy + dz * dz;

            if (distSq <= rangeSq && distSq < bestDistSq) {
                bestDistSq = distSq;
                bestTarget = gp;
                bestTarget.y += targetHeight * 0.5f;
                hasTarget = true;
            }
        }
    }

    if (!hasTarget) return;

    // ���˕���
    XMFLOAT3 dir = { bestTarget.x - muzzle.x, bestTarget.y - muzzle.y, bestTarget.z - muzzle.z };
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (len < 0.001f) return;
    dir.x /= len; dir.y /= len; dir.z /= len;

    // ���ǔ��e�𐶐��i�Ⴂ�͂��������j
    auto* proj = new ProjectileHoming(&projectileManager);
    proj->Launch(dir, muzzle, bestTarget);

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
                // �q�b�g���o�Ȃǁi�K�v�Ȃ�ǉ��j
            }
            // �j�������� Projectile/Manager �����d�l�ɏ]��
        }
    }
}

void AllySlimeHoming::Update(float elapsedTime)
{
    // 1) ����A���J�[�X�V
    UpdateAnchor();

    // 2) �A���J�[�֒Ǐ]�iAllySlime �Ɠ��������j
    float vx = anchor.x - position.x;
    float vz = anchor.z - position.z;
    float d = std::sqrt(vx * vx + vz * vz);
    if (d > 0.0001f) { vx /= d; vz /= d; }
    else { vx = vz = 0.0f; }

    Move(elapsedTime, vx, vz, moveSpeed);
    Turn(elapsedTime, vx, vz, turnSpeed);

    // 3) �����U��
    AutoAttackUpdate(elapsedTime);

    // 4) Character �̕W���X�V
    UpdateVelocity(elapsedTime);
    UpdateInvincibleTimer(elapsedTime);
    UpdateTransform();

    // 5) �e��������
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

    // �˒������O
    if (autoAttackEnabled) {
        const XMFLOAT3 center = { position.x, position.y, position.z };
        renderer->RenderCylinder(rc, center, autoAttackRange, height, XMFLOAT4(0, 1, 0, 0.2f));
    }
    // �A���J�[�}�[�J�[
    renderer->RenderSphere(rc, anchor, 0.15f, XMFLOAT4(1, 1, 0, 1));
}
