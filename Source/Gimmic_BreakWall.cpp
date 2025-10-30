#include "Gimmic_BreakWall.h"
#include "Factory.h"

Gimmic_BreakWall::Gimmic_BreakWall()
{
    auto wall = std::make_unique<Gimmic_BreakWall>();
	class_name = "Gimmic_BreakWall";
	wall->model = ModelManager::Instance().Load("Data/Model/bilud/saku.mdl");

    collider->type = ColliderType::Box;
    box = static_cast<BoxCollider*>(collider);
}

void Gimmic_BreakWall::OnTrigger(GameObject* objects)
{
    hp--;
    if (hp <= 0.0f) delete model;
}

void Gimmic_BreakWall::Update(float elapsedTime)
{
    if (!collider)return;

    // ① 壁のAABB（軸平行境界ボックス）を求める
    halfSize = { 
        scale.x * 0.5f,
        scale.y * 0.5f,
        scale.z * 0.5f };

    box->box_min = {
        position.x - halfSize.x,
        position.y - halfSize.y,
        position.z - halfSize.z
    };
    box->box_max = {
        position.x + halfSize.x,
        position.y + halfSize.y,
        position.z + halfSize.z
    };

}

void Gimmic_BreakWall::RenderDebugPrimitive(
    const RenderContext& rc, ShapeRenderer* renderer)
{

}

REGISTER_GAMEOBJECT(Gimmic_BreakWall);

