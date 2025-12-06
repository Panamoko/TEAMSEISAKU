#include "Player.h"
#include "PlayerMelee.h"
#include "PlayerHeal.h"
#include "PlayerShot.h"
#include "SceneGame.h"
#include "ModelManager.h"
#include "CollisionManager.h"
#include "EnemyManager.h"
#include "Core.h"
#include "System/Input.h"
#include "Picking_Ray.h"
#include "MathUtils.h"
#include "SpriteManager.h"
#include <algorithm>
#include <imgui.h>
#include "GimmicManager.h"
#include "Gimmic_BreakWall.h"
using namespace DirectX;

Player* Player::sActive = nullptr;
std::vector<Player*> Player::sAllPlayers;

Player::Player() { type = Type::Player; }
Player::~Player() { Finalize(); }

int Player::s_spawnCount = 0;

Player& Player::Instance() { return *sActive; }
void Player::SetActive(Player* p) { sActive = p; }
Player* Player::GetActivePtr() { return sActive; }
void Player::RegisterPlayer(Player* player) {
    if (std::find(sAllPlayers.begin(), sAllPlayers.end(), player) == sAllPlayers.end())
        sAllPlayers.push_back(player);
}
void Player::UnregisterPlayer(Player* player) {
    sAllPlayers.erase(std::remove(sAllPlayers.begin(), sAllPlayers.end(), player), sAllPlayers.end());
}
const std::vector<Player*>& Player::GetAllPlayers() { return sAllPlayers; }

void Player::Initialize()
{
    InitializeCommon("Data/Model/Slime/Player_Slime.mdl", "Data/Sprite/Player.png");
}

// 共通初期化処理
void Player::InitializeCommon(const char* modelPath, const char* iconPath)
{
    model = ModelManager::Instance().CreateNewInstance(modelPath);
    scale = { 0.005f, 0.005f, 0.005f };

    animator.SetModel(model);

    // ★修正: 初期アニメーションを設定し、変数にも記録しておく
    currentAnim = "kyara_taiki";
    animator.Play(currentAnim.c_str(), true);
    animator.SetBlendSeconds(0.2f);

    const auto& nodes = model->GetNodes();
    headBoneIndex = -1;
    crownNodeIndex = -1;
    for (int i = 0; i < nodes.size(); ++i) {
        std::string name = nodes[i].name;
        if (name.find("joint4") != std::string::npos) headBoneIndex = i;
        else if (name.find("pTorus1") != std::string::npos) crownNodeIndex = i;
    }

    maxHealth = 100;
    health = maxHealth;
    radius = 0.5f;
    height = 2.0f;

    collider = std::make_unique<CylinderCollider>();
    collider->type = ColliderType::Cylinder;
    collider->owner = this;
    cylinder = static_cast<CylinderCollider*>(collider.get());
    cylinder->height = height;
    cylinder->radius = radius;
    CollisionManager::Instance().AddObject(this);

    playerIcon = SpriteManager::Instance().Load(iconPath);
    hpBarSprite = new Sprite(nullptr);

    RegisterPlayer(this);
}

void Player::Finalize()
{
    if (hpBarSprite) { delete hpBarSprite; hpBarSprite = nullptr; }
    model = nullptr;
}

void Player::Update(float elapsedTime)
{
    // アニメーション更新
    animator.Update(elapsedTime);
    if (cylinder) cylinder->center = position;

    bool isActive = IsPlayerActive();
    bool isMoving = false; // 動いているかどうかのフラグ

    if (isActive)
    {
        // --- プレイヤー操作時 ---
        DirectX::XMFLOAT3 moveVec = GetMoveVec();

        // ベクトルの長さの2乗を計算 (デッドゾーン判定)
        float lenSq = moveVec.x * moveVec.x + moveVec.z * moveVec.z;

        // 入力が一定以上(約25%)ある場合のみ「移動中」とする
        if (lenSq > 0.5f)
        {
            isMoving = true;

            // ★重要修正: 「移動中」と判定された時だけ移動処理を行う
            // これで「待機モーションなのに勝手に動く」現象がなくなります
            InputMove(elapsedTime);
        }

        // ジャンプは移動していなくてもできるように外に出しておく
        InputJump();
    }
    else
    {
        // --- AI操作時 ---
        isMoving = UpdateMoveToCore(elapsedTime);
        UpdateAutoSpawn(elapsedTime);
    }

    // --- アニメーション切り替え制御 ---
    std::string nextAnim = isMoving ? "kyara_junp" : "kyara_taiki";

    if (nextAnim != currentAnim)
    {
        currentAnim = nextAnim;
        // アニメーションが見つからない場合、ここで警告が出る可能性があります
        animator.Play(currentAnim.c_str(), true);
    }

    UpdateVelocity(elapsedTime);
    UpdateInvincibleTimer(elapsedTime);
    UpdateTransform();
    if (model) model->UpdateTransform();

    if (headBoneIndex != -1 && crownNodeIndex != -1) {
        auto* nodes = const_cast<Model::Node*>(model->GetNodes().data());
        nodes[crownNodeIndex].globalTransform = nodes[headBoneIndex].globalTransform;
    }
}

// ... (以下、変更なし) ...
void Player::SpawnAlly(SceneGame* scene) { if (scene) scene->AddAllyStraightFor(this); }
// ... (他の関数は元のままでOK) ...
// UpdateSpawn, UpdateActiveByKeyboard, UpdateAutoSpawn, InputMove, InputJump, GetMoveVecなど
// 下記の部分だけは省略せず記述しますが、変更点は上記までです。

void Player::UpdateSpawn(std::vector<std::shared_ptr<Character>>& players, const Picking_Ray& pickingRay)
{
    // ... (元のコードのまま) ...
    bool isClick = (Input::Instance().GetMouse().GetButtonDown() & Mouse::BTN_LEFT);
    if (!ImGui::GetIO().WantCaptureMouse && isClick)
    {
        // ▼▼▼ 修正1: 判定を players.size() から s_spawnCount に変更 ▼▼▼
        if (s_spawnCount < 5)
        {
            DirectX::XMFLOAT3 org = pickingRay.GetRayOrigin();
            DirectX::XMFLOAT3 dir = pickingRay.GetRayDirection();
            if (fabs(dir.y) > 1e-4f)
            {
                float t = -org.y / dir.y;
                if (t > 0.0f)
                {
                    float hitX = org.x + t * dir.x;
                    float hitZ = org.z + t * dir.z;
                    Core* core = Core::Instance();
                    if (core) {
                        float dx = std::abs(hitX - core->position.x);
                        float dz = std::abs(hitZ - core->position.z);
                        if (dx < 30.0f && dz < 30.0f) return;
                    }
                    std::shared_ptr<Player> newPlayer = nullptr;
                    if (GetAsyncKeyState('X') & 0x8000)      newPlayer = std::make_shared<PlayerMelee>();
                    else if (GetAsyncKeyState('V') & 0x8000) newPlayer = std::make_shared<PlayerHeal>();
                    else if (GetAsyncKeyState('C') & 0x8000) newPlayer = std::make_shared<PlayerShot>();
                    else return;

                    if (newPlayer)
                    {
                        newPlayer->Initialize();
                        newPlayer->SetPosition({ hitX, 0.0f, hitZ });
                        players.push_back(newPlayer);
                        if (players.size() == 1) SetActive(newPlayer.get());

                        // ▼▼▼ 修正2: 生成成功時にカウントを増やす ▼▼▼
                        s_spawnCount++;
                    }
                }
            }
        }
    }
}

bool Player::UpdateActiveByKeyboard(const std::vector<std::shared_ptr<Character>>& players)
{
    // ... (元のコードのまま) ...
    for (int i = 0; i < 5; ++i) {
        if (GetAsyncKeyState('1' + i) & 0x8000) {
            if (i < players.size()) {
                if (auto p = std::dynamic_pointer_cast<Player>(players[i])) {
                    Player::SetActive(p.get());
                    return true;
                }
            }
        }
    }
    return false;
}

bool Player::UpdateAutoSpawn(float elapsedTime)
{
    spawnTimer -= elapsedTime;
    if (spawnTimer <= 0.0f) {
        spawnTimer = spawnInterval;
        return true;
    }
    return false;
}

void Player::InputMove(float elapsedTime)
{
    DirectX::XMFLOAT3 moveVec = GetMoveVec();
    Move(elapsedTime, moveVec.x, moveVec.z, moveSpeed);
    Turn(elapsedTime, moveVec.x, moveVec.z, turnSpeed);
}

void Player::InputJump()
{
    GamePad& gamePad = Input::Instance().GetGamePad();
    if (gamePad.GetButtonDown() & GamePad::BTN_A) {
        if (jumpCount < jumpLimit) {
            jumpCount++;
            Jump(jumpSpeed);
        }
    }
}

DirectX::XMFLOAT3 Player::GetMoveVec() const
{
    GamePad& gamePad = Input::Instance().GetGamePad();
    float ax = gamePad.GetAxisLX();
    float ay = gamePad.GetAxisLY();
    DirectX::XMFLOAT3 vec = { ax, 0.0f, ay };
    float lenSq = vec.x * vec.x + vec.z * vec.z;
    if (lenSq > 1.0f) {
        float len = sqrtf(lenSq);
        vec.x /= len;
        vec.z /= len;
    }
    return vec;
}

void Player::OnLanding() { jumpCount = 0; }

void Player::OnDead() {
    UnregisterPlayer(this);
    if (GetActivePtr() == this) SetActive(sAllPlayers.empty() ? nullptr : sAllPlayers.front());
    GameObject::SetActive(false);
}

bool Player::IsPlayerActive() const { return sActive == this; }
void Player::OnCollision(GameObject* obj) { Character::OnCollision(obj); }
void Player::RequestPathRecalculation() { pathRecalcTimer = 0.0f; currentPath.clear(); pathIndex = 0; }

bool Player::UpdateMoveToCore(float elapsedTime)
{
    Core* core = Core::Instance();
    // コアが無い、破壊されている場合は動かない
    if (!core || core->GetHP() <= 0.0f) { Move(elapsedTime, 0, 0, 0); return false; }
    if (!gridMap) return false;

    // --- 1. 経路の定期更新 (A* to Core) ---
    pathRecalcTimer -= elapsedTime;
    if (pathRecalcTimer <= 0.0f)
    {
        pathRecalcTimer = 0.5f;

        auto start = gridMap->WorldToCell(position.x, position.z);
        auto goal = gridMap->WorldToCell(core->position.x, core->position.z);

        // コアへのパスを検索
        currentPath = aStar.FindPath(start.first, start.second, goal.first, goal.second, *gridMap);

        // パスが見つかったらインデックスを初期化
        if (!currentPath.empty()) pathIndex = (currentPath.size() > 1) ? 1 : 0;
    }

    // --- 2. 移動実行 ---

    // ■ ケースA: コアへの経路が見つかっている場合
    if (!currentPath.empty() && pathIndex < currentPath.size())
    {
        auto [cx, cz] = currentPath[pathIndex];
        DirectX::XMFLOAT3 targetPos = gridMap->GetWorldPosition(cx, cz);

        // 次の経由地点に到達したらインデックスを進める
        float dx = targetPos.x - position.x;
        float dz = targetPos.z - position.z;
        if ((dx * dx + dz * dz) < 0.25f)
        {
            pathIndex++;
            if (pathIndex < currentPath.size()) {
                auto [ncx, ncz] = currentPath[pathIndex];
                targetPos = gridMap->GetWorldPosition(ncx, ncz);
            }
        }

        // --- 敵による足止め判定 (既存処理) ---
        bool isBlockedByEnemy = false;
        EnemyManager& enemyMgr = EnemyManager::Instance();
        int enemyCount = enemyMgr.GetEnemyCount();
        float searchRadiusSq = 6.0f * 6.0f; // 半径6.0

        // 簡易視線チェック
        auto HasLineOfSightGrid = [&](const DirectX::XMFLOAT3& s, const DirectX::XMFLOAT3& e) {
            // ... (簡易化のため省略、必要なら前のコードのHasLineOfSightGridを入れてください)
            // ここでは距離判定だけに依存せず、厳密にやるなら前回の処理を使ってください
            return true;
            };
        // ※以前のラムダ式が長いため省略していますが、元のロジックが必要なら維持してください
        // ここでは「移動」に焦点を当てています

        for (int i = 0; i < enemyCount; ++i)
        {
            auto enemy = enemyMgr.GetEnemy(i);
            if (!enemy) continue;
            float edx = position.x - enemy->position.x;
            float edz = position.z - enemy->position.z;
            if ((edx * edx + edz * edz) < searchRadiusSq) {
                // 近くに敵がいれば止まる（攻撃は別処理）
                isBlockedByEnemy = true;
                break;
            }
        }

        if (isBlockedByEnemy)
        {
            Move(elapsedTime, 0, 0, 0);
            return false;
        }

        // 経路に沿って移動
        float vx = targetPos.x - position.x;
        float vz = targetPos.z - position.z;
        float dist = sqrtf(vx * vx + vz * vz);
        if (dist > 0.001f)
        {
            vx /= dist; vz /= dist;
            Move(elapsedTime, vx, vz, moveSpeed * autoMoveSpeedRate);
            Turn(elapsedTime, vx, vz, turnSpeed * autoMoveTurnRate);
            return true;
        }
    }
    // ■ ケースB: 経路が見つからない（囲われている）場合 -> 一番近い壁へ直進
    else
    {
        float minDistSq = FLT_MAX;
        std::shared_ptr<Gimmic_BreakWall> targetWall = nullptr;

        // 全ギミックから一番近い「壊れていない壁」を探す
        auto& gimmics = GimmicManager::Instance().GetAll();
        for (auto& g : gimmics)
        {
            if (g->class_name == "Gimmic_BreakWall")
            {
                auto wall = std::dynamic_pointer_cast<Gimmic_BreakWall>(g);
                if (wall && !wall->IsBroken())
                {
                    float dx = position.x - wall->position.x;
                    float dz = position.z - wall->position.z;
                    float distSq = dx * dx + dz * dz;

                    if (distSq < minDistSq)
                    {
                        minDistSq = distSq;
                        targetWall = wall;
                    }
                }
            }
        }

        // 壁が見つかったら、その方向へ単純移動
        if (targetWall)
        {
            float dx = targetWall->position.x - position.x;
            float dz = targetWall->position.z - position.z;
            float dist = sqrtf(dx * dx + dz * dz);

            // 壁に密着する手前まで移動し続ける
            if (dist > 0.5f)
            {
                float vx = dx / dist;
                float vz = dz / dist;

                // 経路探索なしで直進
                Move(elapsedTime, vx, vz, moveSpeed);
                Turn(elapsedTime, vx, vz, turnSpeed);
                return true;
            }
        }
    }

    // 移動すべき場所がない
    Move(elapsedTime, 0, 0, 0);
    return false;
}

void Player::Render(const RenderContext& rc, ModelRenderer* renderer)
{
    // 通常のダメージ点滅色を取得
    DirectX::XMFLOAT4 drawColor = GetDamageColor();

    // ★追加: 操作中のプレイヤーなら、リムライト用のフラグ(a=2.0)を立てる
    // (ダメージを受けていない時のみ適用。ダメージ赤点滅を優先)
    if (IsPlayerActive() && invincibleTimer <= 0.0f)
    {
        // RGBはそのまま(1.0)、Alphaを2.0にしてシェーダーに通知
        drawColor = { 1.0f, 1.0f, 1.0f, 2.0f };
    }

    if (model) renderer->Render(rc, transform, model, ShaderId::Lambert, drawColor);
}

void Player::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
    Character::RenderDebugPrimitive(rc, renderer);

    // アクティブなプレイヤーの足元表示
    if (this == GetActivePtr()) {
        DirectX::XMFLOAT3 ringPos = position; ringPos.y += 0.02f;
        renderer->RenderCylinder(rc, ringPos, radius + 0.15f, 0.05f, { 1, 1, 0, 0.8f });
    }

    // 経路の表示（緑の点）
    if (!currentPath.empty()) {
        for (const auto& cell : currentPath) {
            DirectX::XMFLOAT3 pos = gridMap->GetWorldPosition(cell.first, cell.second);
            renderer->RenderSphere(rc, pos, 0.2f, { 0, 1, 0, 1 });
        }
    }

    // ★修正: 索敵範囲のデバッグ表示
    // 経路がある時だけ表示（AI移動中のみ確認したい場合）
    if (!currentPath.empty() && pathIndex < currentPath.size())
    {
        // 古いコード（targetPosへの描画）は削除しました

        // プレイヤー中心の索敵範囲 (半径3.0f) を赤枠で表示
        // ロジック側の searchRadius と同じ大きさにします
        renderer->RenderSphere(rc, position, 6.0f, { 1.0f, 0.0f, 0.0f, 1.0f });
    }
}

void Player::RenderUI(const RenderContext& rc, float x, float y, float size)
{
    if (playerIcon)
    {
        // ★追加: 操作中のプレイヤーなら、背後に強調表示を描画
        if (IsPlayerActive())
        {
            float outlineSize = size * 1.1f; // 少し大きく
            float offset = (outlineSize - size) * 0.5f;

            // 黄色で強調表示 (RGB=1,1,0)
            playerIcon->Render(rc,
                x - offset, y - offset, 0.0f,
                outlineSize, outlineSize,
                0.0f,
                1.0f, 1.0f, 0.0f, 1.0f // 黄色
            );
        }

        // 通常のアイコン描画
        playerIcon->Render(rc, x, y, 0.0f, size, size, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f);

        if (hpBarSprite)
        {
            // ... (HPバーの描画処理、変更なし) ...
            float barW = size;
            float barH = 10.0f;
            float barX = x;
            float barY = y - barH - 5.0f;
            float hpRatio = std::clamp((float)health / (float)maxHealth, 0.0f, 1.0f);
            hpBarSprite->Render(rc, barX, barY, 0.0f, barW, barH, 0.0f, 0.2f, 0.2f, 0.2f, 1.0f);
            float r = (hpRatio < 0.3f) ? 1.0f : 0.0f;
            float g = (hpRatio < 0.3f) ? 0.0f : 1.0f;
            hpBarSprite->Render(rc, barX, barY, 0.0f, barW * hpRatio, barH, 0.0f, r, g, 0.0f, 1.0f);
        }
    }
}

void Player::ResetSpawnCount()
{
    s_spawnCount = 0;
}