#include "Gimmic_BreakWall.h"
#include "Factory.h"
#include <imgui.h>
#include <imstb_truetype.h>
#include <DirectXTex.h>
#include <DDS.h>

//a

Gimmic_BreakWall::Gimmic_BreakWall()
{
	class_name = "Gimmic_BreakWall";
	model = ModelManager::Instance().Load("Data/Model/bilud/saku.mdl");

    collider = std::make_unique<OBB>();
    collider->type = ColliderType::OBB;
    box = static_cast<OBB*>(collider.get());

    scale.x = 0.1f;
    scale.y = 0.05f;
    scale.z = 0.1f;

    CollisionManager::Instance().AddObject(this);
}

//Õ“ËŒ‹‰Ê
void Gimmic_BreakWall::OnCollision(GameObject* objects)
{
    if (objects->type == Type::PlayerAttack)
        hp--;
    if (hp <= 0.0f)
    {
        isActive = false;
        GimmicManager::Instance().RemoveInactive();
    }
}

//XVˆ—
void Gimmic_BreakWall::Update(float elapsedTime)
{
    if (!collider || !model) return;

    DirectX::XMFLOAT3 center;
    DirectX::XMFLOAT3 half;
    DirectX::XMFLOAT3 axis[3];

    // ƒ‚ƒfƒ‹‚Ì OBB ‚ðŽæ“¾
    if (model->GetModelOBB(
        model,
        position,
        angle,
        scale,
        center,
        half,
        axis))
    {
        box->center = center;
        box->half = half;

        box->axis[0] = axis[0];
        box->axis[1] = axis[1];
        box->axis[2] = axis[2];
    }
}

void Gimmic_BreakWall::RenderDebugPrimitive(
    const RenderContext& rc, ShapeRenderer* renderer)
{
    if (!renderer || !collider) return;

    DirectX::XMFLOAT3 pos;

    // OBB ¨ AABB
    OBBtoAABB(
        box->center,
        box->half,
        box->axis,
        pos,
        size);

    DirectX::XMFLOAT3 angle = { 0,0,0 }; // AABB ‚Í‰ñ“]‚µ‚È‚¢
    DirectX::XMFLOAT4 color = { 0.2f, 0.8f, 0.2f, 1.0f };

    // AABB ˜g‚ð•`‰æ
    renderer->RenderBox(rc, pos, angle, size, color);
}

void Gimmic_BreakWall::OnImGui()
{
    if (ImGui::CollapsingHeader("BreakWall Settings"))
    {
        ImGui::DragFloat("HP", &hp, 0.1f, 0.0f, 1500.0f, "%.1f");
        ImGui::DragFloat3("size", &size.x, 0.1f, 0.0f, 1500.0f, "%.1f");
    }
}

void Gimmic_BreakWall::OBBtoAABB(
    const DirectX::XMFLOAT3& center,
    const DirectX::XMFLOAT3& half,
    const DirectX::XMFLOAT3 axis[3],
    DirectX::XMFLOAT3& outPos,
    DirectX::XMFLOAT3& outSize)
{
    using namespace DirectX;

    XMVECTOR C = XMLoadFloat3(&center);
    XMVECTOR A0 = XMLoadFloat3(&axis[0]);
    XMVECTOR A1 = XMLoadFloat3(&axis[1]);
    XMVECTOR A2 = XMLoadFloat3(&axis[2]);

    float hx = half.x;
    float hy = half.y;
    float hz = half.z;

    XMFLOAT3 c[8];
    const int s[8][3] =
    {
        {-1,-1,-1},{-1,+1,-1},{+1,+1,-1},{+1,-1,-1},
        {-1,-1,+1},{-1,+1,+1},{+1,+1,+1},{+1,-1,+1},
    };

    for (int i = 0; i < 8; i++)
    {
        XMVECTOR p =
            C +
            A0 * (hx * s[i][0]) +
            A1 * (hy * s[i][1]) +
            A2 * (hz * s[i][2]);
        XMStoreFloat3(&c[i], p);
    }

    XMFLOAT3 mn = c[0];
    XMFLOAT3 mx = c[0];

    for (int i = 1; i < 8; i++)
    {
        mn.x = (std::min)(mn.x, c[i].x);
        mn.y = (std::min)(mn.y, c[i].y);
        mn.z = (std::min)(mn.z, c[i].z);

        mx.x = (std::max)(mx.x, c[i].x);
        mx.y = (std::max)(mx.y, c[i].y);
        mx.z = (std::max)(mx.z, c[i].z);
    }

    outPos = {
        (mn.x + mx.x) * 0.5f,
        (mn.y + mx.y) * 0.5f,
        (mn.z + mx.z) * 0.5f
    };

    outSize = {
        mx.x - mn.x,
        mx.y - mn.y,
        mx.z - mn.z
    };
}

REGISTER_GAMEOBJECT(Gimmic_BreakWall);

