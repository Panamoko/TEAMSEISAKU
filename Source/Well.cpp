#include "Well.h"
#include "Factory.h"

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
    isBroken = false;
    isRespawning = false;
    respawnTimer = 0.0f;
    fadeInTimer = 0.0f;
    scale = { 0.3f,0.3f,0.3f };
}

void Well::Update(float elapsedTime)
{
    cylinder->center = position;
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

REGISTER_GAMEOBJECT(Well);
