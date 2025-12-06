#include "AllySlimeHeal.h"
#include "AllySlime.h"
#include "Player.h"
#include "ProjectileHoming.h"
#include "ModelManager.h"
#include "Collision.h"
#include <cfloat>
#include <cmath>
#include <cstdlib>
#include "EffectManager.h"

using namespace DirectX;

static inline XMFLOAT3 operator+(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x + b.x,a.y + b.y,a.z + b.z }; }
static inline XMFLOAT3 operator-(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x - b.x,a.y - b.y,a.z - b.z }; }
static inline XMFLOAT3 operator*(const XMFLOAT3& a, float s) { return { a.x * s,a.y * s,a.z * s }; }

AllySlimeHeal::AllySlimeHeal(int formationIndex, Player* initLeader)
    : index(formationIndex), leader(initLeader)
{
    // ★修正: CreateUniqueInstance を使用
    auto modelPtr = ModelManager::Instance().CreateUniqueInstance("Data/Model/Slime/Slime_G.mdl");
    slimeModel = modelPtr.release();

    animator.SetModel(slimeModel);
    animator.Play("kyara_taiki (1)", true);

    // ★修正: ランダムオフセット
    float randomOffset = static_cast<float>(rand() % 100) / 100.0f;
    animator.Update(randomOffset);

    scale = { 0.002f, 0.002f, 0.002f };
    radius = 0.5f;
    height = 1.0f;
    maxHealth = 20;
    health = maxHealth;
    icon = SpriteManager::Instance().Load("Data/Sprite/SLime_G.png");
    hpBarSprite = new Sprite(nullptr);

    collider = std::make_unique<CylinderCollider>();
    collider->type = ColliderType::Cylinder;
    collider->owner = this;

    auto* cyl = static_cast<CylinderCollider*>(collider.get());
    cyl->radius = radius;
    cyl->height = height;
    cyl->center = position;

    const Player& ref = (leader ? *leader : Player::Instance());
    position = ref.GetPosition();
    UpdateTransform();

    AllySlime::RegisterAlly(this);
}

AllySlimeHeal::~AllySlimeHeal()
{
    if (hpBarSprite) {
        delete hpBarSprite;
        hpBarSprite = nullptr;
    }
    // ★修正: 削除
    if (slimeModel) {
        delete slimeModel;
        slimeModel = nullptr;
    }
    AllySlime::UnregisterAlly(this);
}

void AllySlimeHeal::UpdateAnchor()
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

void AllySlimeHeal::UpdateHealing(float elapsedTime)
{
    if (!autoHealEnabled) return;

    if (autoHealTimer > 0.0f) {
        autoHealTimer -= elapsedTime;
        return;
    }

    const float rangeSq = autoHealRange * autoHealRange;
    bool healedAny = false;

    const auto& players = Player::GetAllPlayers();
    for (auto* p : players)
    {
        if (!p || p->GetHealth() <= 0) continue;
        if (p->GetHealth() >= p->GetMaxHealth()) continue;

        float dx = p->GetPosition().x - position.x;
        float dz = p->GetPosition().z - position.z;
        if ((dx * dx + dz * dz) <= rangeSq)
        {
            p->Heal(10);

            // プレイヤーの位置で回復エフェクト再生
            EffectManager::Instance().Play("Heal", p->GetPosition(), 30.0f);

            healedAny = true;
        }
    }

    const auto& allies = AllySlime::GetAllAllies();
    for (auto* a : allies)
    {
        if (!a || a->GetHealth() <= 0) continue;
        if (a->GetHealth() >= a->GetMaxHealth()) continue;

        float dx = a->GetPosition().x - position.x;
        float dz = a->GetPosition().z - position.z;
        if ((dx * dx + dz * dz) <= rangeSq)
        {
            a->Heal(1);

            // 味方の位置で回復エフェクト再生
            EffectManager::Instance().Play("Heal", a->GetPosition(), 30.0f);

            healedAny = true;
        }
    }

    if (healedAny)
    {
        autoHealTimer = autoHealInterval;
        // ★修正: 回復アニメーション
        isHealingAction = true;
        animator.Play("kyara_kugeki (1)", false);
    }
}

void AllySlimeHeal::Update(float elapsedTime)
{
    animator.Update(elapsedTime);

    if (leader && (!leader->IsActive() || leader->GetHealth() <= 0))
    {
        OnDead();
        return;
    }

    UpdateAnchor();

    // 揺れ防止移動
    float vx = anchor.x - position.x;
    float vz = anchor.z - position.z;
    float d = std::sqrt(vx * vx + vz * vz);
    if (d < 0.1f) { vx = 0.0f; vz = 0.0f; }
    else { vx /= d; vz /= d; }

    Move(elapsedTime, vx, vz, moveSpeed);
    Turn(elapsedTime, vx, vz, turnSpeed);

    UpdateHealing(elapsedTime);

    // アニメーション
    if (isHealingAction)
    {
        if (!animator.IsPlaying()) isHealingAction = false;
    }
    else
    {
        float speedSq = velocity.x * velocity.x + velocity.z * velocity.z;
        if (speedSq > 0.01f)
        {
            animator.Play("kyara_junp", true);
        }
        else
        {
            animator.Play("kyara_taiki (1)", true);
        }
    }

    UpdateVelocity(elapsedTime);
    UpdateInvincibleTimer(elapsedTime);
    UpdateTransform();

    if (collider)
    {
        static_cast<CylinderCollider*>(collider.get())->center = position;
    }
}

void AllySlimeHeal::Render(const RenderContext& rc, ModelRenderer* renderer)
{
    renderer->Render(rc, transform, slimeModel, ShaderId::Lambert, GetDamageColor());
    projectileManager.Render(rc, renderer);
}

void AllySlimeHeal::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
    Character::RenderDebugPrimitive(rc, renderer);
    projectileManager.RenderDebugPrimitive(rc, renderer);

    if (autoHealEnabled) {
        const XMFLOAT3 center = { position.x, position.y, position.z };
        renderer->RenderCylinder(rc, center, autoHealRange, height, XMFLOAT4(0, 1, 0, 0.2f));
    }
    renderer->RenderSphere(rc, anchor, 0.15f, XMFLOAT4(1, 1, 0, 1));
}

void AllySlimeHeal::RenderUI(const RenderContext& rc, float x, float y, float size)
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
            hpBarSprite->Render(rc, barX, barY, 0.0f, barW * hpRatio, barH, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
        }
    }
}

void AllySlimeHeal::OnDead()
{
    AllySlime::UnregisterAlly(this);
    GameObject::SetActive(false);
}