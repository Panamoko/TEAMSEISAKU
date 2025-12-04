#include "GimmicBase.h"

void GimmicBase::OnCollision(GameObject* objcts)
{
	if (objcts->type == Type::PlayerAttack)
	{
		hitSE[0]->Play(false);
	}
}

void GimmicBase::UpdateInvicible(float elapsedTime)
{
	if (invincible_timer >= 0.0f)
	{
		invincible_timer -= elapsedTime;
	}
}
