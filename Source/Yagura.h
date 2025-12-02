#pragma once
#include "GimmicBase.h"
#include "Collider.h"
#include "EnemySlimeTurret.h"

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
	
	// --- タレット管理用 ---
	std::shared_ptr<EnemySlimeTurret> turret = nullptr; // 生成したタレットのポインタ
	bool isTurretSpawned = false; // 生成済みフラグ
};

