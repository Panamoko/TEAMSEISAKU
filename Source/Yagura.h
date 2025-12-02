#pragma once
#include "GimmicBase.h"
#include "Collider.h"

class Yagura : public GimmicBase
{
public:
	Yagura();
	~Yagura();
	void Update(float elapsedTime)override; //ギミック更新処理
	void OnCollision(GameObject* object)override;//衝突処理
	void Render(const RenderContext& rc, ModelRenderer* renderer)override;
	void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) override;//デバッグ表示
	float GetHp() { return hp; }//HP取得

private:
	float hp;			//耐久度
	OBB* obb;
};

