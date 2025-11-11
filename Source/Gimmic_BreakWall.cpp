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
        CollisionManager::Instance().Remove(this);
    }
}

//XVˆ—
void Gimmic_BreakWall::Update(float elapsedTime)
{
    if (!collider || !model) return;

    DirectX::XMFLOAT3 center;
    DirectX::XMFLOAT3 half;
    DirectX::XMFLOAT3 axis[3];

    DirectX::XMFLOAT3 rotation = {
        DirectX::XMConvertToRadians(angle.x),
        DirectX::XMConvertToRadians(angle.y),
        DirectX::XMConvertToRadians(angle.z)
    };


    // ƒ‚ƒfƒ‹‚Ì OBB ‚ðŽæ“¾
    if (model->GetModelOBB(
        model,
        position,
        rotation,
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

        if (model) {
            printf("OBB center = (%f,%f,%f)\n", center.x, center.y, center.z);
            printf("OBB half   = (%f,%f,%f)\n", half.x, half.y, half.z);
            printf("axis0 = (%f,%f,%f)\n", axis[0].x, axis[0].y, axis[0].z);
            printf("axis1 = (%f,%f,%f)\n", axis[1].x, axis[1].y, axis[1].z);
            printf("axis2 = (%f,%f,%f)\n", axis[2].x, axis[2].y, axis[2].z);
        }
    }
}

void Gimmic_BreakWall::RenderDebugPrimitive(
    const RenderContext& rc, ShapeRenderer* renderer)
{
    if (!renderer || !collider) return;

    DirectX::XMFLOAT3 pos;

    // OBB ¨ AABB
    Collision::OBBtoAABB(
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


REGISTER_GAMEOBJECT(Gimmic_BreakWall);

