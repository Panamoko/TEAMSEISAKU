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

AllySlimeHeal::AllySlimeHeal(int formationIndex)
    : index(formationIndex)
{
    // ヒーラーっぽい色（例えば赤やピンク、緑など）のモデルがあれば差し替えると分かりやすいです
    slimeModel = ModelManager::Instance().Load("Data/Model/Slime/Slime_G.mdl");

    scale = { 0.002f, 0.002f, 0.002f };
    radius = 0.5f;
    height = 1.0f;
    icon = SpriteManager::Instance().Load("Data/Sprite/SLime_G.png");
    const Player& ref = (leader ? *leader : Player::Instance());
    position = ref.GetPosition();
    UpdateTransform();

    AllySlime::RegisterAlly(this); // 登録
}

AllySlimeHeal::~AllySlimeHeal()
{
    AllySlime::UnregisterAlly(this); // 解除
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

// ----------------------------------------------------------------------------
// ターゲット検索：HPが減っているプレイヤーを探す
// ----------------------------------------------------------------------------
void AllySlimeHeal::UpdateHealing(float elapsedTime)
{
    if (!autoHealEnabled) return;

    if (autoHealTimer > 0.0f) {
        autoHealTimer -= elapsedTime;
        return;
    }

    // 発射位置（スライムの中心あたり）
    const XMFLOAT3 muzzle = { position.x, position.y + height * 0.5f, position.z };
    const float rangeSq = autoHealRange * autoHealRange;

    float bestDistSq = FLT_MAX;
    XMFLOAT3 bestTarget = { 0, 0, 0 };
    bool hasTarget = false;

    // 全プレイヤーを取得してループ
    const auto& players = Player::GetAllPlayers();
    for (auto* player : players)
    {
        if (!player) continue;

        // HPが満タンなら回復対象にしない
        // ※GetHealth(), GetMaxHealth() は Characterクラスに追加済みと仮定
        if (player->GetHealth() >= player->GetMaxHealth()) continue;

        // 距離チェック
        XMFLOAT3 q = player->GetPosition();
        q.y += player->GetHeight() * 0.5f; // プレイヤーの中心を狙う

        float dx = q.x - muzzle.x;
        float dy = q.y - muzzle.y;
        float dz = q.z - muzzle.z;
        float d2 = dx * dx + dy * dy + dz * dz;

        // 射程内かつ、今までで一番近ければターゲットにする
        if (d2 <= rangeSq && d2 < bestDistSq) {
            bestDistSq = d2;
            bestTarget = q;
            hasTarget = true;
        }
    }

    // ターゲット（傷ついた味方）がいなければ何もしない
    if (!hasTarget) return;

    // 発射処理
    XMFLOAT3 dir = { bestTarget.x - muzzle.x, bestTarget.y - muzzle.y, bestTarget.z - muzzle.z };
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (len < 0.001f) return;
    dir.x /= len; dir.y /= len; dir.z /= len;

    auto* proj = new ProjectileHoming(&projectileManager);
    // 必要ならここで弾の色を変える等の処理を入れる
    proj->Launch(dir, muzzle, bestTarget);

    autoHealTimer = autoHealInterval;
}

// ----------------------------------------------------------------------------
// 衝突判定：弾丸 vs プレイヤー（回復）
// ----------------------------------------------------------------------------
void AllySlimeHeal::CollisionProjectilesVsPlayers()
{
    const auto& players = Player::GetAllPlayers();
    const int projectileCount = projectileManager.GetProjectileCount();

    for (int i = 0; i < projectileCount; ++i) {
        Projectile* projectile = projectileManager.GetProjectile(i);
        if (!projectile) continue;

        bool isHit = false;

        // 全プレイヤーと判定
        for (auto* player : players) {
            if (!player) continue;

            XMFLOAT3 outPos;
            // Sphere vs Cylinder 判定
            const bool hit = Collision::IntersectSphereVsCylinder(
                projectile->GetPosition(), projectile->GetRadius(),
                player->GetPosition(), player->GetRadius(), player->GetHeight(),
                outPos
            );

            if (hit) {
                // 回復処理を実行 (Character::Heal)
                player->Heal(1); // 1回復

                // エフェクトや音を出すならここで

                // 弾を消去
                projectile->Destroy();
                isHit = true;
                break;
            }
        }
    }
}

void AllySlimeHeal::Update(float elapsedTime)
{
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

    // 5) 弾の更新と衝突判定
    projectileManager.Update(elapsedTime);
    CollisionProjectilesVsPlayers();
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
    }
}