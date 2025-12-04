// =============================================
// AllySlimeHoming.cpp
// =============================================
#include "AllySlimeHeal.h"
#include "AllySlime.h"
#include "Player.h"
#include "ProjectileHoming.h"
#include "ModelManager.h"
#include "Collision.h"
#include <cfloat>
#include <cmath>

// 不要なインクルードを削除
// #include "EnemyManager.h"
// #include "GimmicManager.h"
// #include "Gimmic_BreakWall.h"
// #include "Core.h"

using namespace DirectX;

// 演算子オーバーロード（ベクトル計算用）
static inline XMFLOAT3 operator+(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x + b.x,a.y + b.y,a.z + b.z }; }
static inline XMFLOAT3 operator-(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x - b.x,a.y - b.y,a.z - b.z }; }
static inline XMFLOAT3 operator*(const XMFLOAT3& a, float s) { return { a.x * s,a.y * s,a.z * s }; }

AllySlimeHeal::AllySlimeHeal(int formationIndex, Player* initLeader)
    : index(formationIndex), leader(initLeader)
{
    // ヒーラーっぽい色（例えば赤やピンク、緑など）のモデルがあれば差し替えると分かりやすいです
    slimeModel = ModelManager::Instance().Load("Data/Model/Slime/Slime_G.mdl");

    scale = { 0.002f, 0.002f, 0.002f };
    radius = 0.5f;
    height = 1.0f;
    icon = SpriteManager::Instance().Load("Data/Sprite/SLime_G.png");
    hpBarSprite = new Sprite(nullptr);
    // ★追加: コライダーの設定
    collider = std::make_unique<CylinderCollider>();
    collider->type = ColliderType::Cylinder;
    collider->owner = this;

    // コライダーのサイズ設定 (モデルに合わせて調整)
    auto* cyl = static_cast<CylinderCollider*>(collider.get());
    cyl->radius = radius;
    cyl->height = height;
    cyl->center = position; // 初期位置

    const Player& ref = (leader ? *leader : Player::Instance());
    position = ref.GetPosition();
    UpdateTransform();

    AllySlime::RegisterAlly(this); // 登録
}

AllySlimeHeal::~AllySlimeHeal()
{
    // ★追加
    if (hpBarSprite) {
        delete hpBarSprite;
        hpBarSprite = nullptr;
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

    // --- 全プレイヤーをチェック ---
    const auto& players = Player::GetAllPlayers();
    for (auto* p : players)
    {
        if (!p || p->GetHealth() <= 0) continue;
        if (p->GetHealth() >= p->GetMaxHealth()) continue; // HP満タンならスキップ

        // 距離チェック
        float dx = p->GetPosition().x - position.x;
        float dz = p->GetPosition().z - position.z;
        if ((dx * dx + dz * dz) <= rangeSq)
        {
            p->Heal(1); // 1回復
            healedAny = true;
        }
    }

    // --- 全味方スライムをチェック ---
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
            healedAny = true;
        }
    }

    // 誰かを回復したらクールダウンに入る
    if (healedAny)
    {
        autoHealTimer = autoHealInterval;
        // エフェクトがあればここで再生
    }
}

void AllySlimeHeal::Update(float elapsedTime)
{
    if (leader && (!leader->IsActive() || leader->GetHealth() <= 0))
    {
        OnDead(); // 死亡処理を実行
        return;   // 更新をここで打ち切る
    }
    // 1) 隊列アンカー更新
    UpdateAnchor();

    // 2) 移動処理
    float vx = anchor.x - position.x;
    float vz = anchor.z - position.z;
    float d = std::sqrt(vx * vx + vz * vz);
    if (d > 0.0001f) { vx /= d; vz /= d; }
    else { vx = vz = 0.0f; }

    Move(elapsedTime, vx, vz, moveSpeed);
    Turn(elapsedTime, vx, vz, turnSpeed);

    // 3) 自動回復ロジック
    UpdateHealing(elapsedTime);

    // 4) 基底クラス更新
    UpdateVelocity(elapsedTime);
    UpdateInvincibleTimer(elapsedTime);
    UpdateTransform();

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
        // 回復射程を緑色で表示
        renderer->RenderCylinder(rc, center, autoHealRange, height, XMFLOAT4(0, 1, 0, 0.2f));
    }
    renderer->RenderSphere(rc, anchor, 0.15f, XMFLOAT4(1, 1, 0, 1));
}

void AllySlimeHeal::RenderUI(const RenderContext& rc, float x, float y, float size)
{
    if (icon)
    {
        icon->Render(rc, x, y, 0.0f, size, size, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f);

        // ★追加: HPバー描画
        if (hpBarSprite)
        {
            float barW = size;
            float barH = 8.0f;
            float barX = x;
            float barY = y - barH - 3.0f;

            float hpRatio = (float)health / (float)maxHealth;
            hpRatio = std::clamp(hpRatio, 0.0f, 1.0f);

            // 背景
            hpBarSprite->Render(rc, barX, barY, 0.0f, barW, barH, 0.0f, 0.2f, 0.2f, 0.2f, 1.0f);
            // HPバー
            hpBarSprite->Render(rc, barX, barY, 0.0f, barW * hpRatio, barH, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
        }
    }
}

// ---------------------------------------------------------
// 死亡時の処理
// ---------------------------------------------------------
void AllySlimeHeal::OnDead()
{
    // 1. 味方リストから削除
    AllySlime::UnregisterAlly(this);

    // 2. 非アクティブ化
    GameObject::SetActive(false);
}