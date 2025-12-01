#include "Cannon.h"
#include "Player.h"
#include "Factory.h"
#include "CollisionManager.h"

Cannon::Cannon()
{
	hp = 50.0f;
	attac_interval = 3.0f;
	attac_territory = 10.0f;
	attac_timer = 0.0f;
	power = 10.0f;
	
}

void Cannon::Update(float elapsedTime)
{
	attac_timer += elapsedTime;
}

void Cannon::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
}

void Cannon::OnCollision(GameObject* object)
{
}

bool Cannon::OnImGui()
{
	return false;
}

void Cannon::CopyUniqueMembers(const GameObject* source)
{
}
