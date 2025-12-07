#include "AllySlime.h"
#include "Player.h"
#include "EnemyManager.h"
#include "ProjectileStraite.h"
#include "ModelManager.h"
#include "Collision.h"
#include "GimmicManager.h"
#include "Gimmic_BreakWall.h"
#include "Core.h"
#include "EffectManager.h"
#include <cmath>
#include <cfloat>
#include "SpriteManager.h"
#include "Collider.h"
#include "CollisionManager.h"
#include <cstdlib>
#include "Cannon.h"

using namespace DirectX;

static inline XMFLOAT3 operator+(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
static inline XMFLOAT3 operator-(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
static inline XMFLOAT3 operator*(const XMFLOAT3& a, float s) { return { a.x * s, a.y * s, a.z * s }; }

std::vector<Character*> AllySlime::s_allies;

void AllySlime::RegisterAlly(Character* ally)
{
    auto it = std::find(s_allies.begin(), s_allies.end(), ally);
    if (it == s_allies.end()) {
        s_allies.push_back(ally);
    }
}

void AllySlime::UnregisterAlly(Character* ally)
{
    auto it = std::remove(s_allies.begin(), s_allies.end(), ally);
    s_allies.erase(it, s_allies.end());
}

const std::vector<Character*>& AllySlime::GetAllAllies()
{
    return s_allies;
}

AllySlime::AllySlime(int formationIndex, Player* initLeader)
    : index(formationIndex), leader(initLeader)
{
    // ★修正: CreateUniqueInstance
    auto modelPtr = ModelManager::Instance().CreateUniqueInstance("Data/Model/Slime/Slime_B.mdl");
    slimeModel = modelPtr.release();

    animator.SetModel(slimeModel);
    animator.Play("kyara_taiki (1)", true);

    // ランダム開始
    animator.Update(static_cast<float>(rand() % 100) / 100.0f);

    scale = { 0.002f, 0.002f, 0.002f };
    radius = 0.5f;
    height = 1.0f;
    maxHealth = 20;
    health = maxHealth;
    icon = SpriteManager::Instance().Load("Data/Sprite/SLime_B.png");
    hpBarSprite = new Sprite(nullptr);

    if (leader)
    {
        UpdateAnchor();
        position = anchor;
    }
    else if (Player::GetActivePtr())
    {
        const Player& ref = Player::Instance();
        position = ref.GetPosition();
    }
    UpdateTransform();

    type = Type::Player;

    RegisterAlly(this);

    collider = std::make_unique<CylinderCollider>();
    collider->type = ColliderType::Cylinder;
    collider->owner = this;
    auto* cyl = static_cast<CylinderCollider*>(collider.get());
    cyl->radius = radius;
    cyl->height = height;
    cyl->center = position;
    CollisionManager::Instance().AddObject(this);
}

void AllySlime::UpdateAnchor()
{
    const Player& ref = (leader ? *leader : Player::Instance());
    const XMFLOAT3& p = ref.GetPosition();
    const XMFLOAT3& a = ref.GetAngle();

    XMFLOAT3 fwd = { std::sinf(a.y), 0.0f,  std::cosf(a.y) };
    XMFLOAT3 rgt = { std::cosf(a.y), 0.0f, -std::sinf(a.y) };

    const int row = index / rowWidth;
    const int col = index % rowWidth;
    const float rightOffset = (col - (rowWidth - 1) * 0.5f) * lateralSpacing;
    const float backOffset = (row + 1) * followDistance;

    anchor = p + (rgt * rightOffset) + (fwd * (-backOffset));
    anchor.y = p.y;
}

void AllySlime::AutoAttackUpdate(float elapsedTime)
{
    if (!autoAttackEnabled) return;

    if (autoAttackTimer > 0.0f) {
        autoAttackTimer -= elapsedTime;
        return;
    }

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
        if (!gimmic || !gimmic->IsActive()) continue;
        if (!gimmic->collider) continue;

        if (gimmic->class_name == "Gimmic_BreakWall")
        {
            auto wall = std::dynamic_pointer_cast<Gimmic_BreakWall>(gimmic);
            if (wall && wall->IsBroken()) continue;
        }

        float targetHeight = 0.0f;
        bool isTarget = false;

        // ★追加: Cannonをターゲットとして認識する
        if (gimmic->class_name == "Cannon")
        {
            auto cannon = std::dynamic_pointer_cast<Cannon>(gimmic);
            // HPが残っている場合のみターゲットにする
            if (cannon && cannon->GetHP() > 0.0f) isTarget = true;
        }

        // コライダー形状に応じた高さ設定
        if (gimmic->collider->type == ColliderType::OBB)
        {
            OBB* box = static_cast<OBB*>(gimmic->collider.get());
            targetHeight = box->half.y * 2.0f;
            // 壁などは既に上で判定済みだが、OBBを持つ他のギミック用
            if (gimmic->class_name == "Gimmic_BreakWall" && !isTarget) {
                // 壁は上で判定しているのでここではフラグを立て直さない
                // (BreakWallの判定ロジックに合わせて調整)
                auto wall = std::dynamic_pointer_cast<Gimmic_BreakWall>(gimmic);
                if (wall && !wall->IsBroken()) isTarget = true;
            }
        }
        else if (gimmic->collider->type == ColliderType::Cylinder)
        {
            CylinderCollider* cyl = static_cast<CylinderCollider*>(gimmic->collider.get());
            targetHeight = cyl->height;
            // Cannonなどは上で isTarget=true になっていればOK
        }

        if (isTarget) {
            // ... (距離判定とベストターゲット更新のロジックはそのまま)
            const XMFLOAT3& gp = gimmic->position;
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

    if (!hasTarget) return;

    XMFLOAT3 dir = { bestTarget.x - pos.x, bestTarget.y - pos.y, bestTarget.z - pos.z };
    const float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (len < 0.001f) return;
    dir.x /= len; dir.y /= len; dir.z /= len;

    auto* proj = new ProjectileStraite(&projectileManager, "Data/Model/Slime/Bullet.mdl", Type::PlayerAttack, 0.15f);
    proj->Launch(dir, pos);

    // 攻撃アニメーション
    // AllySlime.h に isAction がない場合は追加してください
    // isAction = true;
    animator.Play("kyara_kugeki (1)", false);

    autoAttackTimer = autoAttackInterval;
}

void AllySlime::OnCollision(GameObject* object)
{
    Character::OnCollision(object);
}

void AllySlime::CollisionProjectilesVsEnemies()
{
    EnemyManager& em = EnemyManager::Instance();
    const int projectileCount = projectileManager.GetProjectileCount();
    const int enemyCount = em.GetEnemyCount();

    for (int i = 0; i < projectileCount; ++i) {
        Projectile* projectile = projectileManager.GetProjectile(i);
        if (!projectile) continue;

        for (int j = 0; j < enemyCount; ++j) {
            std::shared_ptr<Enemy> enemy = em.GetEnemy(j);
            if (!enemy) continue;

            XMFLOAT3 outPos;
            const bool hit = Collision::IntersectSphereVsCylinder(
                projectile->GetPosition(), projectile->GetRadius(),
                enemy->GetPosition(), enemy->GetRadius(), enemy->GetHeight(), outPos);

            if (!hit) continue;

            // エフェクト設定
            Effekseer::Handle handle = EffectManager::Instance().Play("SlimeAttack", outPos);
            EffectManager::Instance().SetScale(handle, 0.05f, 0.05f, 0.05f);
            EffectManager::Instance().Skip(handle, 151.0f);

            if (enemy->ApplyDamage(5, 0.5f)) {}
            projectile->Destroy();
            break;
        }
        if (!projectile) continue;

        GimmicManager& gm = GimmicManager::Instance();
        auto& gimmicks = gm.GetAll();
        for (auto& gimmic : gimmicks) {
            if (!gimmic || !gimmic->IsActive()) continue;
            if (!gimmic->collider) continue;

            bool hit = false;
            if (gimmic->collider->type == ColliderType::OBB) {
                OBB* obb = static_cast<OBB*>(gimmic->collider.get());
                XMFLOAT3 outMTD;
                if (Collision::IntersectSphereVsOBB(projectile->GetPosition(), projectile->GetRadius(), *obb, &outMTD)) hit = true;
            }
            else if (gimmic->collider->type == ColliderType::Cylinder) {
                CylinderCollider* cylinder = static_cast<CylinderCollider*>(gimmic->collider.get());
                XMFLOAT3 outPos;
                if (Collision::IntersectSphereVsCylinder(projectile->GetPosition(), projectile->GetRadius(), cylinder->center, cylinder->radius, cylinder->height, outPos)) hit = true;
            }

            if (hit) {
                // エフェクト設定
                Effekseer::Handle handle = EffectManager::Instance().Play("SlimeAttack", projectile->GetPosition());
                EffectManager::Instance().SetScale(handle, 0.05f, 0.05f, 0.05f);
                EffectManager::Instance().Skip(handle, 151.0f);

                gimmic->OnCollision(projectile);
                projectile->Destroy();
                break;
            }
        }
    }
}

void AllySlime::Update(float elapsedTime)
{
    animator.Update(elapsedTime);

    if (leader && (!leader->IsActive() || leader->GetHealth() <= 0))
    {
        OnDead();
        return;
    }
    UpdateAnchor();

    float vx = anchor.x - position.x;
    float vz = anchor.z - position.z;
    float d = std::sqrt(vx * vx + vz * vz);
    if (d < 0.1f) { vx = 0.0f; vz = 0.0f; }
    else { vx /= d; vz /= d; }

    Move(elapsedTime, vx, vz, moveSpeed);
    Turn(elapsedTime, vx, vz, turnSpeed);

    AutoAttackUpdate(elapsedTime);

    // アニメーション（攻撃中は上書きしない）
    if (!animator.IsPlaying() || animator.GetCurrentSeconds() > 0.5f) // 簡易判定
    {
        float speedSq = velocity.x * velocity.x + velocity.z * velocity.z;
        if (speedSq > 0.01f) animator.Play("kyara_junp", true);
        else                 animator.Play("kyara_taiki (1)", true);
    }

    UpdateVelocity(elapsedTime);
    UpdateInvincibleTimer(elapsedTime);
    UpdateTransform();

    if (collider)
    {
        static_cast<CylinderCollider*>(collider.get())->center = position;
    }

    projectileManager.Update(elapsedTime);
    CollisionProjectilesVsEnemies();
}

void AllySlime::Render(const RenderContext& rc, ModelRenderer* renderer)
{
    renderer->Render(rc, transform, slimeModel, ShaderId::Lambert, GetDamageColor());
    projectileManager.Render(rc, renderer);
}

void AllySlime::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
    Character::RenderDebugPrimitive(rc, renderer);
    projectileManager.RenderDebugPrimitive(rc, renderer);

    if (autoAttackEnabled) {
        const XMFLOAT3 center = { position.x, position.y, position.z };
        renderer->RenderCylinder(rc, center, autoAttackRange, height, XMFLOAT4(0, 1, 0, 0.2f));
    }

    renderer->RenderSphere(rc, anchor, 0.15f, XMFLOAT4(1, 1, 0, 1));
}

void AllySlime::RenderUI(const RenderContext& rc, float x, float y, float size)
{
    if (icon)
    {
        icon->Render(rc, x, y, 0.0f, size, size, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f);

        if (hpBarSprite)
        {
            float barW = size;
            float barH = 8.0f;
            float barX = x;
            float barY = y - barH - 3.0f;

            float hpRatio = (float)health / (float)maxHealth;
            hpRatio = std::clamp(hpRatio, 0.0f, 1.0f);

            hpBarSprite->Render(rc, barX, barY, 0.0f, barW, barH, 0.0f, 0.2f, 0.2f, 0.2f, 1.0f);
            hpBarSprite->Render(rc, barX, barY, 0.0f, barW * hpRatio, barH, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
        }
    }
}

void AllySlime::OnDead()
{
    UnregisterAlly(this);
    GameObject::SetActive(false);
}