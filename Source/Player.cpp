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
#include <imgui.h> // ★追加

using namespace DirectX;

Player* Player::sActive = nullptr;
std::vector<Player*> Player::sAllPlayers;

Player::Player() { type = Type::Player; }
Player::~Player() { Finalize(); }

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
    InitializeCommon("Data/Model/Slime/Player_Slime.mdl");
}

// ★共通初期化処理
void Player::InitializeCommon(const char* modelPath)
{
    model = ModelManager::Instance().CreateNewInstance(modelPath);
    scale = { 0.005f, 0.005f, 0.005f };

    animator.SetModel(model);
    animator.Play("Take 001", true);
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

    playerIcon = SpriteManager::Instance().Load("Data/Sprite/Player.png");
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
    animator.Update(elapsedTime);
    if (cylinder) cylinder->center = position;

    bool isActive = IsPlayerActive();
    if (isActive)
    {
        InputMove(elapsedTime);
        InputJump();
    }
    else
    {
        UpdateMoveToCore(elapsedTime);
        // SceneGame側で判定しSpawnAllyを呼ぶため、タイマーのみ更新
        UpdateAutoSpawn(elapsedTime);
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

void Player::SpawnAlly(SceneGame* scene)
{
    if (scene) scene->AddAllyStraightFor(this);
}

// ★スポーン処理: shared_ptr<Character> を受け取るように変更済み
void Player::UpdateSpawn(std::vector<std::shared_ptr<Character>>& players, const Picking_Ray& pickingRay)
{
    bool isClick = (Input::Instance().GetMouse().GetButtonDown() & Mouse::BTN_LEFT);
    if (!ImGui::GetIO().WantCaptureMouse && isClick)
    {
        if (players.size() < 5)
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
                    }
                }
            }
        }
    }
}

// キーボード切り替え
bool Player::UpdateActiveByKeyboard(const std::vector<std::shared_ptr<Character>>& players)
{
    for (int i = 0; i < 5; ++i)
    {
        if (GetAsyncKeyState('1' + i) & 0x8000)
        {
            if (i < players.size())
            {
                if (auto p = std::dynamic_pointer_cast<Player>(players[i]))
                {
                    Player::SetActive(p.get());
                    return true;
                }
            }
        }
    }
    return false;
}

// 共通処理の実装
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
    if (gamePad.GetButtonDown() & GamePad::BTN_A)
    {
        if (jumpCount < jumpLimit)
        {
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

void Player::UpdateMoveToCore(float elapsedTime)
{
    Core* core = Core::Instance();
    if (!core || core->GetHP() <= 0.0f) { Move(elapsedTime, 0, 0, 0); return; }
    if (!gridMap) return;

    pathRecalcTimer -= elapsedTime;
    if (currentPath.empty() || pathRecalcTimer <= 0.0f)
    {
        pathRecalcTimer = 0.5f;
        auto start = gridMap->WorldToCell(position.x, position.z);
        auto goal = gridMap->WorldToCell(core->position.x, core->position.z);
        currentPath = aStar.FindPath(start.first, start.second, goal.first, goal.second, *gridMap);
        if (!currentPath.empty()) pathIndex = (currentPath.size() > 1) ? 1 : 0;
    }

    if (!currentPath.empty() && pathIndex < currentPath.size())
    {
        auto [cx, cz] = currentPath[pathIndex];
        DirectX::XMFLOAT3 targetPos = gridMap->GetWorldPosition(cx, cz);
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
        float vx = targetPos.x - position.x;
        float vz = targetPos.z - position.z;
        float dist = sqrtf(vx * vx + vz * vz);
        if (dist > 0.001f)
        {
            vx /= dist; vz /= dist;
            Move(elapsedTime, vx, vz, moveSpeed * autoMoveSpeedRate);
            Turn(elapsedTime, vx, vz, turnSpeed * autoMoveTurnRate);
        }
    }
    else
    {
        Move(elapsedTime, 0, 0, 0);
    }
}

void Player::Render(const RenderContext& rc, ModelRenderer* renderer)
{
    if (model) renderer->Render(rc, transform, model, ShaderId::Lambert, GetDamageColor());
}

void Player::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
    Character::RenderDebugPrimitive(rc, renderer);
    if (this == GetActivePtr()) {
        DirectX::XMFLOAT3 ringPos = position; ringPos.y += 0.02f;
        renderer->RenderCylinder(rc, ringPos, radius + 0.15f, 0.05f, { 1, 1, 0, 0.8f });
    }
    if (!currentPath.empty()) {
        for (const auto& cell : currentPath) {
            DirectX::XMFLOAT3 pos = gridMap->GetWorldPosition(cell.first, cell.second);
            renderer->RenderSphere(rc, pos, 0.2f, { 0, 1, 0, 1 });
        }
    }
}

void Player::RenderUI(const RenderContext& rc, float x, float y, float size)
{
    if (playerIcon)
        playerIcon->Render(rc, x, y, 0.0f, size, size, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f);

    if (hpBarSprite)
    {
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