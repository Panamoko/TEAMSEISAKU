#pragma once
#include "GimmicBase.h"
#include "Collider.h"
#include "Player.h"
#include "ProjectileManager.h"


class Cannon :public GimmicBase
{
public:
	Cannon();

	void Update(float elapsedTime)override;//ƒMƒ~ƒbƒNXVˆ—

	// •`‰æˆ—
	void Render(const RenderContext& rc, ModelRenderer* renderer);

	void Turn(float elapsedTime, DirectX::XMFLOAT3 player_position);//‰ñ“]ˆ—

	//ƒfƒoƒbƒO•`‰æ
	void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) override;
	void OnCollision(GameObject* object)override;//Õ“Ëˆ—
	bool OnImGui()override;

	void CopyUniqueMembers(const GameObject* source) override;

	//HPæ“¾—pŠÖ”
	float GetHP() const { return hp; }

private:
	float hp;					//‘Ï‹v“x
	float power;				//UŒ‚—Í
	float attac_interval;		//UŒ‚•p“x
	float attac_timer;			//ÄUŒ‚ŠÔ
	float attac_territory;		//UŒ‚”ÍˆÍ
	float speed;				//‰ñ“]‘¬“x
	Player* player;
	ProjectileManager projectileManager;
	CylinderCollider* cylinder;
};

