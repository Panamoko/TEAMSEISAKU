#include "Well.h"
#include "Factory.h"
#include "GimmicManager.h"

Well::Well()
{
    collider = std::make_unique<CylinderCollider>();
    collider->type = ColliderType::Cylinder;
    cylinder = static_cast<CylinderCollider*>(collider.get());

    cylinder->height = 2.0f;
    cylinder->owner = this;
    cylinder->radius = 2.0f;
    CollisionManager::Instance().AddObject(this);

    hp = 2.0f;
    isRespawning = false;
    respawnTimer = 0.0f;
    fadeInTimer = 0.0f;
    scale = { 0.3f,0.3f,0.3f };

    color = { 1.0f,1.0f,1.0f,1.0f };
}

void Well::Update(float elapsedTime)
{
    cylinder->center = position;
    if (invincible_timer > 0.0f)invincible_timer -= elapsedTime;
    else color = { 1.0f,1.0f,1.0f,1.0f };
    if (hp <= 0.0f)
    {
        GimmicManager::Instance().Remove(this);
        CollisionManager::Instance().Remove(this);
    }
}

void Well::OnCollision(GameObject* objects)
{
    if (objects->type == Type::PlayerAttack && invincible_timer <= 0.0f)
    {
        hp--;
        invincible_timer = 0.1f;
        color = { 1.0f,0.0f,0.0f,1.0f };
    }
}

void Well::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
    renderer->RenderCylinder(
	rc,
	cylinder->center,    // 中心（地面に置く座標）
	cylinder->radius,      // 半径
	cylinder->height,      // 高さ
	DirectX::XMFLOAT4(1, 0, 0, 1)
);

}

void Well::Render(const RenderContext& rc, ModelRenderer* renderer)
{
    renderer->Render(rc, transform, model, ShaderId::Lambert, color);
}

REGISTER_GAMEOBJECT(Well);
