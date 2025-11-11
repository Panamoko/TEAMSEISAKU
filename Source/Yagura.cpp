#include "Yagura.h"
#include "GimmicManager.h"
#include "Player.h"

Yagura::Yagura()
{
	//当たり判定設定
	collider = std::make_unique<OBB>();
	collider->type = ColliderType::OBB;
	collider->owner = this;
	obb = static_cast<OBB*>(collider.get());
	CollisionManager::Instance().AddObject(this);

	//基礎設定
	class_name = "Yagura";
	hp = 100.0f;
}

Yagura::~Yagura()
{
}

//ギミック更新処理
void Yagura::Update(float elapsedTime)
{
	if (hp <= 0.0f)
	{
		GimmicManager::Instance().Remove(this);
		CollisionManager::Instance().Remove(this);
	}
}

//衝突処理
void Yagura::OnCollision(GameObject* object)
{
	if (object->type == Type::PlayerAttack)
	{
		hp -= 10.0f;
	}
}

void Yagura::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
}
