#include "Gimmic_BreakWall.h"
#include "Factory.h"

//a

Gimmic_BreakWall::Gimmic_BreakWall()
{
	class_name = "Gimmic_BreakWall";
	model = ModelManager::Instance().Load("Data/Model/bilud/saku.mdl");

    collider = new BoxCollider();
    collider->type = ColliderType::Box;
    box = static_cast<BoxCollider*>(collider);

    scale.x = 0.1f;
    scale.y = 0.05f;
    scale.z = 0.1f;
}

//衝突結果
void Gimmic_BreakWall::OnCollision(GameObject* objects)
{
    if (objects->type == Type::Player)hp--;
    else if (hp <= 0.0f && objects->type == Type::Player)
    {
        isActive = false;
        GimmicManager::Instance().RemoveInactive();
    }
}

//更新処理
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

