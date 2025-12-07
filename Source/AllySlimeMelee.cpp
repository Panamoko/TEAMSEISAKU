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
#include "AllySlime.h"
#include "SpriteManager.h"
#include <cstdlib> // rand()用
#include "Cannon.h"
#include "Collider.h"

using namespace DirectX;

static inline XMFLOAT3 operator+(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
static inline XMFLOAT3 operator-(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
static inline XMFLOAT3 operator*(const XMFLOAT3& a, float s) { return { a.x * s, a.y * s, a.z * s }; }

AllySlimeMelee::AllySlimeMelee(int formationIndex, Player* initLeader)
    : index(formationIndex), leader(initLeader)
{
    // ★修正1: Load ではなく CreateUniqueInstance を使い、個別のインスタンスを作成
    auto modelPtr = ModelManager::Instance().CreateUniqueInstance("Data/Model/Slime/Slime_R.mdl");
    slimeModel = modelPtr.release(); // 所有権を受け取る

    // ★修正2: アニメーター初期化 (スペースありの正しい名前)
    animator.SetModel(slimeModel);
    animator.Play("kyara_taiki (1)", true);

    // ★修正3: 開始時間をランダムにずらして、同期しないようにする
    float randomOffset = static_cast<float>(rand() % 100) / 100.0f; // 0.0~1.0秒
    animator.Update(randomOffset);

    scale = { 0.002f, 0.002f, 0.002f };
    radius = 0.6f;
    height = 1.0f;
    maxHealth = 30;
    health = maxHealth;

    collider = std::make_unique<CylinderCollider>();
    collider->type = ColliderType::Cylinder;
    collider->owner = this;
    auto* cyl = static_cast<CylinderCollider*>(collider.get());
    cyl->radius = radius;
    cyl->height = height;
    cyl->center = position;

    icon = SpriteManager::Instance().Load("Data/Sprite/SLime_R.png");
    hpBarSprite = new Sprite(nullptr);
    type = Type::PlayerAttack;

    const Player& ref = (leader ? *leader : Player::Instance());
    position = ref.GetPosition();

    UpdateTransform();
    attackDamage = 5;
    AllySlime::RegisterAlly(this);
}

AllySlimeMelee::~AllySlimeMelee()
{
    if (hpBarSprite) {
        delete hpBarSprite;
        hpBarSprite = nullptr;
    }
    // ★修正4: 独自インスタンスなので自分で削除する
    if (slimeModel) {
        delete slimeModel;
        slimeModel = nullptr;
    }
    AllySlime::UnregisterAlly(this);
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
    if (!targetEnemy.expired() || !targetGimmic.expired()) return;

    const Player& ref = (leader ? *leader : Player::Instance());
    XMFLOAT3 center = ref.GetPosition();
    float minStartDistSq = searchRange * searchRange;

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
            targetEnemy = e;
            targetGimmic.reset();
        }
    }

    GimmicManager& gm = GimmicManager::Instance();
    for (auto& g : gm.GetAll())
    {
        if (!g || !g->IsActive()) continue;

        bool isTarget = false;
        if (g->class_name == "Gimmic_BreakWall")
        {
            auto wall = std::dynamic_pointer_cast<Gimmic_BreakWall>(g);
            if (wall && !wall->IsBroken()) isTarget = true;
        }
        else if (g->class_name == "Core")
        {
            auto core = std::dynamic_pointer_cast<Core>(g);
            if (core && core->GetHP() > 0.0f) isTarget = true;
        }
        else if (g->class_name == "Yagura")
        {
            auto yagura = std::dynamic_pointer_cast<Yagura>(g);
            if (yagura && yagura->GetHp() > 0.0f) isTarget = true;
        }
        else if (g->class_name == "Barracks")
        {
            auto barracks = std::dynamic_pointer_cast<Barracks>(g);
            if (barracks && barracks->GetHP() > 0.0f) isTarget = true;
        }
        // ★追加: Cannonをターゲットにする
        else if (g->class_name == "Cannon")
        {
            auto cannon = std::dynamic_pointer_cast<Cannon>(g);
            if (cannon && cannon->GetHP() > 0.0f) isTarget = true;
        }

        if (isTarget)
        {
            // ... (距離判定はそのまま)
            float dx = g->position.x - center.x;
            float dz = g->position.z - center.z;
            float d2 = dx * dx + dz * dz;

            if (d2 < minStartDistSq)
            {
                minStartDistSq = d2;
                targetGimmic = g;
                targetEnemy.reset();
            }
        }
    }
}

void AllySlimeMelee::UpdateState(float elapsedTime)
{
    // ---------------------------------------------------------
    // 1. 現在のターゲットの生存確認 & 情報更新
    // ---------------------------------------------------------
    bool hasTarget = false;
    float targetRadius = 0.5f;

    // 敵ターゲットの確認
    if (auto e = targetEnemy.lock())
    {
        // 削除リクエストが出ていない、かつ HPが残っているなら有効
        if (!e->IsDestroyRequested() && e->GetHealth() > 0)
        {
            targetPos = e->GetPosition();
            targetRadius = e->GetRadius();
            hasTarget = true;
        }
        else
        {
            targetEnemy.reset();
        }
    }
    // ギミックターゲットの確認（敵がいない場合）
    else if (auto g = targetGimmic.lock())
    {
        if (g->IsActive())
        {
            targetPos = g->position;
            targetRadius = 1.0f; // デフォルト

            // コライダーから正確な半径を取得
            if (g->collider && g->collider->type == ColliderType::Cylinder)
            {
                auto* cyl = static_cast<CylinderCollider*>(g->collider.get());
                targetRadius = cyl->radius;
            }

            // 各ギミックごとの生存判定
            bool isAlive = true;
            if (g->class_name == "Gimmic_BreakWall")
            {
                if (dynamic_cast<Gimmic_BreakWall*>(g.get())->IsBroken()) isAlive = false;
            }
            else if (g->class_name == "Core")
            {
                if (dynamic_cast<Core*>(g.get())->GetHP() <= 0) isAlive = false;
            }
            else if (g->class_name == "Yagura")
            {
                if (dynamic_cast<Yagura*>(g.get())->GetHp() <= 0) isAlive = false;
            }
            else if (g->class_name == "Barracks")
            {
                if (dynamic_cast<Barracks*>(g.get())->GetHP() <= 0) isAlive = false;
            }
            else if (g->class_name == "Cannon")
            {
                if (dynamic_cast<Cannon*>(g.get())->GetHP() <= 0) isAlive = false;
            }

            if (isAlive) hasTarget = true;
            else targetGimmic.reset();
        }
        else
        {
            targetGimmic.reset();
        }
    }

    // ---------------------------------------------------------
    // 2. 共通パラメータ計算
    // ---------------------------------------------------------

    // 索敵タイマーを減らす
    searchTimer -= elapsedTime;

    // 親（アンカー）との距離を計算
    float distToAnchorX = anchor.x - position.x;
    float distToAnchorZ = anchor.z - position.z;
    float distToAnchorSq = distToAnchorX * distToAnchorX + distToAnchorZ * distToAnchorZ;

    // 強制帰還する距離（例: 15m）
    const float kMaxSeparation = 15.0f;
    const float kMaxSeparationSq = kMaxSeparation * kMaxSeparation;

    // 帰還中、再索敵を許可する距離（例: 5mまで戻ったらOK）
    const float kReEngageDistance = 5.0f;


    // ---------------------------------------------------------
    // 3. ステートマシン
    // ---------------------------------------------------------
    switch (state)
    {
    case State::Follow:
        // ターゲットがいれば追跡へ
        if (hasTarget) {
            state = State::Chase;
        }
        else {
            // タイマー経過で索敵実行
            if (searchTimer <= 0.0f) {
                SearchTarget();
                searchTimer = 0.5f; // 0.5秒間隔
            }

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
        // ★重要: 親から離れすぎていたら、ターゲットを捨てて強制帰還
        if (distToAnchorSq > kMaxSeparationSq)
        {
            targetEnemy.reset();
            targetGimmic.reset();
            state = State::Return;
            break;
        }

        if (!hasTarget) {
            state = State::Return;
        }
        else {
            float dx = targetPos.x - position.x;
            float dz = targetPos.z - position.z;
            float d = sqrtf(dx * dx + dz * dz);

            // 攻撃射程内（相手の半径＋自分の半径＋射程）に入ったか？
            float range = attackRange + targetRadius + radius;

            if (d <= range) {
                state = State::Attack;
                attackTimer = 0.0f;
            }
            else {
                // まだ遠いので追いかける
                if (d > 0.1f) {
                    Move(elapsedTime, dx / d, dz / d, moveSpeed * 1.2f);
                    Turn(elapsedTime, dx / d, dz / d, turnSpeed);
                }
            }
        }
        break;

    case State::Attack:
        // 攻撃中でも離れすぎたら中断して戻る
        if (distToAnchorSq > kMaxSeparationSq)
        {
            state = State::Return;
            break;
        }

        if (!hasTarget) {
            state = State::Return;
        }
        else {
            // 敵の方を向く
            float dx = targetPos.x - position.x;
            float dz = targetPos.z - position.z;
            Turn(elapsedTime, dx, dz, turnSpeed);

            attackTimer += elapsedTime;

            // 攻撃判定発生タイミング (0.2秒〜0.3秒の間)
            if (attackTimer > 0.2f && attackTimer < 0.3f)
            {
                CheckAttackCollision();
            }

            // クールダウン完了で再び追跡/攻撃判定へ
            if (attackTimer > damageCooldown)
            {
                state = State::Chase;
            }
        }
        break;

    case State::Return:
        float dx = anchor.x - position.x;
        float dz = anchor.z - position.z;
        float d = sqrtf(dx * dx + dz * dz);

        // ある程度近くまで戻ったら、再索敵を許可する
        if (d < kReEngageDistance)
        {
            if (searchTimer <= 0.0f) {
                SearchTarget();
                searchTimer = 0.5f;
            }

            // ターゲットが見つかったら（SearchTarget内でセットされる）追跡へ
            if (!targetEnemy.expired() || !targetGimmic.expired()) {
                state = State::Chase;
                break;
            }
        }

        // アンカーへ戻る移動
        if (d < 0.5f) {
            state = State::Follow;
        }
        else {
            if (d > 0.1f) {
                Move(elapsedTime, dx / d, dz / d, moveSpeed);
                Turn(elapsedTime, dx / d, dz / d, turnSpeed);
            }
        }
        break;
    }
}

void AllySlimeMelee::CheckAttackCollision()
{
    if (auto e = targetEnemy.lock())
    {
        float dx = e->GetPosition().x - position.x;
        float dz = e->GetPosition().z - position.z;
        float distSq = dx * dx + dz * dz;
        float hitRange = attackRange + e->GetRadius() + radius;

        if (distSq <= hitRange * hitRange)
        {
            e->ApplyDamage(attackDamage, 0.5f);
        }
    }
    else if (auto g = targetGimmic.lock())
    {
        if (g->collider)
        {
            g->OnCollision(this);
        }
    }
}

void AllySlimeMelee::Update(float elapsedTime)
{
    animator.Update(elapsedTime);

    // アニメーション制御
    // isAction (攻撃中フラグ) の管理
    if (isAction)
    {
        if (!animator.IsPlaying()) isAction = false;
    }

    if (state == State::Attack)
    {
        // 攻撃モーション (まだ再生してなければ)
        if (!isAction)
        {
            isAction = true;
            animator.Play("kyara_kugeki (1)", false);
        }
    }
    else
    {
        // 移動・待機
        if (!isAction)
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
    }

    if (leader && (!leader->IsActive() || leader->GetHealth() <= 0))
    {
        OnDead();
        return;
    }
    UpdateAnchor();
    UpdateState(elapsedTime);

    UpdateVelocity(elapsedTime);
    UpdateInvincibleTimer(elapsedTime);
    UpdateTransform();
}

void AllySlimeMelee::Render(const RenderContext& rc, ModelRenderer* renderer)
{
    DirectX::XMFLOAT4 baseColor = { 1.0f, 0.6f, 0.6f, 1.0f };
    renderer->Render(rc, transform, slimeModel, ShaderId::Lambert, GetDamageColor(baseColor));
}

void AllySlimeMelee::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
    Character::RenderDebugPrimitive(rc, renderer);
    renderer->RenderSphere(rc, anchor, 0.15f, { 1, 0, 0, 1 });

    if (state == State::Attack) {
        renderer->RenderSphere(rc, position, attackRange, { 1, 0, 0, 0.5f });
    }
}

void AllySlimeMelee::RenderUI(const RenderContext& rc, float x, float y, float size)
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

void AllySlimeMelee::OnDead()
{
    AllySlime::UnregisterAlly(this);
    GameObject::SetActive(false);
}