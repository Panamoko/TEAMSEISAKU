#include "AllySlimeMelee.h"
#include "Player.h"
#include "EnemyManager.h"
#include "GimmicManager.h"
#include "Gimmic_BreakWall.h"
#include "Core.h"
#include "Collision.h"
#include "ModelManager.h"
#include <cmath>
#include <algorithm>
#include "Yagura.h"
#include "Barracks.h"

using namespace DirectX;

// ベクトル演算用ヘルパー
static inline XMFLOAT3 operator+(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
static inline XMFLOAT3 operator-(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
static inline XMFLOAT3 operator*(const XMFLOAT3& a, float s) { return { a.x * s, a.y * s, a.z * s }; }

AllySlimeMelee::AllySlimeMelee(int formationIndex) : index(formationIndex)
{
    // 近接型は少し色を変えたいので、リソース共有しつつ後で色設定(もしあれば)
    // ここでは通常のAllySlimeと同じモデルを使用
    slimeModel = ModelManager::Instance().Load("Data/Model/Slime/Slime_R.mdl");

    // 少し大きくして近接タイプっぽくする
    scale = { 0.002f, 0.002f, 0.002f };
    radius = 0.6f;
    height = 1.0f;

    // 攻撃用として設定
    type = Type::PlayerAttack;

    const Player& ref = (leader ? *leader : Player::Instance());
    position = ref.GetPosition();
    UpdateTransform();
}

void AllySlimeMelee::UpdateAnchor()
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

void AllySlimeMelee::SearchTarget()
{
    // 既にターゲットがいれば何もしない
    if (!targetEnemy.expired() || !targetGimmic.expired()) return;

    // 基準位置（リーダーの位置）
    const Player& ref = (leader ? *leader : Player::Instance());
    XMFLOAT3 center = ref.GetPosition();
    float minStartDistSq = searchRange * searchRange;

    // 1. 敵を探す
    EnemyManager& em = EnemyManager::Instance();
    for (int i = 0; i < em.GetEnemyCount(); ++i)
    {
        auto e = em.GetEnemy(i);
        if (!e || e->IsDestroyRequested()) continue;

        float dx = e->GetPosition().x - center.x;
        float dz = e->GetPosition().z - center.z;
        float d2 = dx * dx + dz * dz;

        if (d2 < minStartDistSq)
        {
            minStartDistSq = d2;
            targetEnemy = e; // weak_ptrとして保持
            targetGimmic.reset();
        }
    }

    // 2. ギミック（壁・コア・櫓・兵舎）を探す
    GimmicManager& gm = GimmicManager::Instance();
    for (auto& g : gm.GetAll())
    {
        if (!g || !g->IsActive()) continue;

        bool isTarget = false;

        // --- クラス名でターゲット判定 ---

        // 壊せる壁
        if (g->class_name == "Gimmic_BreakWall")
        {
            auto wall = std::dynamic_pointer_cast<Gimmic_BreakWall>(g);
            // 壊れていない壁だけ狙う
            if (wall && !wall->IsBroken()) isTarget = true;
        }
        // コア
        else if (g->class_name == "Core")
        {
            auto core = std::dynamic_pointer_cast<Core>(g);
            if (core && core->GetHP() > 0.0f) isTarget = true;
        }
        // ★追加: Yagura (櫓)
        else if (g->class_name == "Yagura")
        {
            auto yagura = std::dynamic_pointer_cast<Yagura>(g);
            // Yagura.h では GetHp() (pが小文字) なので注意
            if (yagura && yagura->GetHp() > 0.0f) isTarget = true;
        }
        // ★追加: Barracks (兵舎)
        else if (g->class_name == "Barracks")
        {
            auto barracks = std::dynamic_pointer_cast<Barracks>(g);
            // 先ほど追加した GetHP() を使用
            if (barracks && barracks->GetHP() > 0.0f) isTarget = true;
        }

        // ターゲット対象なら距離チェックして一番近いものを保存
        if (isTarget)
        {
            float dx = g->position.x - center.x;
            float dz = g->position.z - center.z;
            float d2 = dx * dx + dz * dz;

            if (d2 < minStartDistSq)
            {
                minStartDistSq = d2;
                targetGimmic = g;
                targetEnemy.reset(); // 敵ターゲットは解除
            }
        }
    }
}

void AllySlimeMelee::UpdateState(float elapsedTime)
{
    // ターゲットの有効性確認と座標更新
    bool hasTarget = false;
    float targetRadius = 0.5f;

    // Enemyターゲット
    if (auto e = targetEnemy.lock())
    {
        if (!e->IsDestroyRequested())
        {
            targetPos = e->GetPosition();
            targetRadius = e->GetRadius();
            hasTarget = true;
        }
        else targetEnemy.reset();
    }
    // Gimmicターゲット
    else if (auto g = targetGimmic.lock())
    {
        if (g->IsActive())
        {
            if (g->IsActive())
            {
                targetPos = g->position;
                targetRadius = 1.0f; // 仮の半径

                // --- 状態ごとの生存チェック ---
                if (g->class_name == "Gimmic_BreakWall")
                {
                    if (dynamic_cast<Gimmic_BreakWall*>(g.get())->IsBroken()) hasTarget = false;
                    else hasTarget = true;
                }
                else if (g->class_name == "Core")
                {
                    if (dynamic_cast<Core*>(g.get())->GetHP() <= 0) hasTarget = false;
                    else hasTarget = true;
                }
                else if (g->class_name == "Yagura")
                {
                    if (dynamic_cast<Yagura*>(g.get())->GetHp() <= 0) hasTarget = false;
                    else hasTarget = true;
                }
                else if (g->class_name == "Barracks")
                {
                    if (dynamic_cast<Barracks*>(g.get())->GetHP() <= 0) hasTarget = false;
                    else hasTarget = true;
                }
                else
                {
                    // その他のギミックならActiveな限りターゲットとする
                    hasTarget = true;
                }

                if (!hasTarget) targetGimmic.reset();
            }
            else
            {
                targetGimmic.reset();
            }
        }
    }

    // ステートマシン
    switch (state)
    {
    case State::Follow:
        // ... (変更なし)
        if (hasTarget) {
            state = State::Chase;
        }
        else {
            SearchTarget();
            // アンカーへ移動
            float dx = anchor.x - position.x;
            float dz = anchor.z - position.z;
            float d = sqrtf(dx * dx + dz * dz);
            if (d > 0.1f)
            {
                Move(elapsedTime, dx / d, dz / d, moveSpeed);
                Turn(elapsedTime, dx / d, dz / d, turnSpeed);
            }
        }
        break;

    case State::Chase:
        // ... (変更なし)
        if (!hasTarget) {
            state = State::Return;
        }
        else {
            float dx = targetPos.x - position.x;
            float dz = targetPos.z - position.z;
            float d = sqrtf(dx * dx + dz * dz);

            float range = attackRange + targetRadius + radius;
            if (d <= range) {
                state = State::Attack;
                attackTimer = 0.0f;
            }
            else {
                Move(elapsedTime, dx / d, dz / d, moveSpeed * 1.2f);
                Turn(elapsedTime, dx / d, dz / d, turnSpeed);
            }
        }
        break;

    case State::Attack:
        // ... (変更なし)
        if (!hasTarget) {
            state = State::Return;
        }
        else {
            float dx = targetPos.x - position.x;
            float dz = targetPos.z - position.z;
            Turn(elapsedTime, dx, dz, turnSpeed);

            attackTimer += elapsedTime;
            if (attackTimer > 0.2f && attackTimer < 0.3f)
            {
                CheckAttackCollision();
            }
            if (attackTimer > damageCooldown)
            {
                state = State::Chase;
            }
        }
        break;

    case State::Return:
    {
        // アンカーに戻る
        float dx = anchor.x - position.x;
        float dz = anchor.z - position.z;
        float d = sqrtf(dx * dx + dz * dz);

        // 帰り道でも敵がいれば反応する
        SearchTarget();
        // ここで targetGimmic.reset() しておかないと、壊れた壁を保持したままになり
        // expired() が false を返してしまい、Chaseに戻ってしまう問題が解決します
        if (!targetEnemy.expired() || !targetGimmic.expired()) {
            state = State::Chase;
            break;
        }

        if (d < 0.5f) {
            state = State::Follow;
        }
        else {
            Move(elapsedTime, dx / d, dz / d, moveSpeed);
            Turn(elapsedTime, dx / d, dz / d, turnSpeed);
        }
    }
    break;
    }
}

void AllySlimeMelee::CheckAttackCollision()
{
    // 攻撃対象にダメージを与える
    if (auto e = targetEnemy.lock())
    {
        // 簡易距離チェック
        float dx = e->GetPosition().x - position.x;
        float dz = e->GetPosition().z - position.z;
        float distSq = dx * dx + dz * dz;
        float hitRange = attackRange + e->GetRadius() + radius;

        if (distSq <= hitRange * hitRange)
        {
            // エネミーにダメージ
            e->ApplyDamage(attackDamage, 0.5f);
            // ノックバック
            //DirectX::XMFLOAT3 impulse = { dx * 5.0f, 2.0f, dz * 5.0f };
            //e->AddImpulse(impulse);
        }
    }
    else if (auto g = targetGimmic.lock())
    {
        // ギミックへの攻撃（壁やコア）
        // ギミックは OnCollision で Type::PlayerAttack を判定しているものが多いので
        // ここで直接 OnCollision を呼んでやる
        if (g->collider)
        {
            // 衝突しているとみなして呼び出す
            g->OnCollision(this);
        }
    }
}

void AllySlimeMelee::Update(float elapsedTime)
{
    UpdateAnchor();
    UpdateState(elapsedTime);

    UpdateVelocity(elapsedTime);
    UpdateInvincibleTimer(elapsedTime);
    UpdateTransform();
}

void AllySlimeMelee::Render(const RenderContext& rc, ModelRenderer* renderer)
{
    // 通常色より少し赤っぽくして区別する
    renderer->Render(rc, transform, slimeModel, ShaderId::Lambert, { 1.0f, 0.6f, 0.6f, 1.0f });
}

void AllySlimeMelee::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
    Character::RenderDebugPrimitive(rc, renderer);
    // アンカー位置
    renderer->RenderSphere(rc, anchor, 0.15f, { 1, 0, 0, 1 });

    // 状態可視化
    if (state == State::Attack) {
        renderer->RenderSphere(rc, position, attackRange, { 1, 0, 0, 0.5f });
    }
}