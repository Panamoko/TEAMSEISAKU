#pragma once
#include "GimmicBase.h"
#include "Collider.h"
#include "CollisionManager.h"
#include "GimmicManager.h"

#include <Animator.h>

class Core : public GimmicBase
{
public:
	Core();
	~Core();
	void init();
	void Update(float elapsedTime)override;
	void Render(const RenderContext& rc, ModelRenderer* renderer)override;
	//デバッグ描画
	void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) override;
	void OnCollision(GameObject* object)override;
	void OnImGui()override;
	float GetHP() { return hp; };

	static Core* Instance();
private:
	static Core* sInstance;

	Animator animator;               // 追加
	CylinderCollider* cylinder;
	float hp;
};

