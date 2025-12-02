#include "GimmicBase.h"

void GimmicBase::UpdateInvicible(float elapsedTime)
{
	if (invincible_timer >= 0.0f)
	{
		invincible_timer -= elapsedTime;
	}
}
