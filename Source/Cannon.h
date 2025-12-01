#pragma once
#include "GimmicBase.h"
#include "Collider.h"

class Cannon :public GimmicBase
{
public:
	Cannon();

	void Update(float elapsedTime)override;//ƒMƒ~ƒbƒNXVˆ—
	//ƒfƒoƒbƒO•`‰æ
	void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) override;
	void OnCollision(GameObject* object)override;//Õ“Ëˆ—
	bool OnImGui()override;

	void CopyUniqueMembers(const GameObject* source) override;

	//HPæ“¾—pŠÖ”
	float GetHP() const { return hp; }

private:
	float hp;				//‘Ï‹v“x
	float power;			//UŒ‚—Í
	float attac_interval;	//UŒ‚•p“x
	float attac_timer;		//ÄUŒ‚ŠÔ
	float attac_territory;	//UŒ‚”ÍˆÍ
};

