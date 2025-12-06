#include "StrawBag.h"
#include "GimmicManager.h"
#include "Factory.h"

StrawBag::StrawBag()
{
    collider = std::make_unique<OBB>();
    collider->type = ColliderType::OBB;
    obb = static_cast<OBB*>(collider.get());
    obb->owner = this;
    CollisionManager::Instance().AddObject(this);

    hp = 2.0f;
    color = { 1.0f,1.0f,1.0f,1.0f };
    scale = { 1.0f,1.0f,1.0f };
}

void StrawBag::Update(float elapsedTime)
{
    // 各軸の向きを更新（Y軸回転のみと仮定）
    DirectX::XMFLOAT3 rotation = {
    DirectX::XMConvertToRadians(angle.x),
    DirectX::XMConvertToRadians(angle.y),
    DirectX::XMConvertToRadians(angle.z)
    };
    float c = cosf(rotation.y);
    float s = sinf(rotation.y);
    obb->axis[0] = DirectX::XMFLOAT3(c, 0.0f, -s); // X軸（右）
    obb->axis[1] = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f); // Y軸（上）
    obb->axis[2] = DirectX::XMFLOAT3(s, 0.0f, c);  // Z軸（前）

    float halfHeight = 5.0f * scale.y;
    obb->half = DirectX::XMFLOAT3(scale.x * 1.5f, halfHeight, scale.z * 5.0f);

    //OBB設定
    obb->center = position;
    obb->center.x -= 0.1f;

    if (invincible_timer > 0.0f)invincible_timer -= elapsedTime;
    else color = { 1.0f,1.0f,1.0f,1.0f };
    if (hp <= 0.0f)
    {
        GimmicManager::Instance().Remove(this);
        CollisionManager::Instance().Remove(this);
    }

}

void StrawBag::OnCollision(GameObject* objects)
{
    if (objects->type == Type::PlayerAttack && invincible_timer <= 0.0f)
    {
        hp--;
        invincible_timer = 0.1f;
        color = { 1.0f,0.0f,0.0f,1.0f };
    }
}

void StrawBag::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
    DirectX::XMFLOAT3 obb_center = obb->center;
    DirectX::XMFLOAT3 obb_half = obb->half;
    DirectX::XMFLOAT3 obb_axis[3] = { obb->axis[0], obb->axis[1], obb->axis[2] };
    DirectX::XMFLOAT4 debug_color = { 0.2f, 0.8f, 0.2f, 1.0f };

    renderer->RenderOBB(
        rc,
        obb_center,
        obb_half,
        obb_axis, // 3つの軸配列を渡す
        debug_color
    );
}

void StrawBag::Render(const RenderContext& rc, ModelRenderer* renderer)
{
    renderer->Render(rc, transform, model, ShaderId::Lambert, color);
}

REGISTER_GAMEOBJECT(StrawBag);