#pragma once
#include "GimmicBase.h"
#include "GimmicManager.h"
#include "Collider.h"
#include "System/ShapeRenderer.h"

class Gimmic_BreakWall : public GimmicBase
{
public:
	Gimmic_BreakWall();

	// 陦晉ｪ∝・逅・
	void OnCollision(GameObject* objects) override;

	// 繧ｮ繝溘ャ繧ｯ譖ｴ譁ｰ蜃ｦ逅・
	void Update(float elapsedTime)override;

	// 謠冗判蜃ｦ逅・
	void Render(const RenderContext& rc, ModelRenderer* renderer) override;

	// 繝・ヰ繝・げ謠冗判
	void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) override;

	bool OnImGui();

	bool IsBroken() const { return isBroken; } // 謾ｻ謦・ｯｾ雎｡蛻､螳夂畑

protected:
	bool isBroken = false; // 螢翫ｌ縺ｦ縺・ｋ縺九←縺・°

	float maxHp = 2.0f; 
	float respawnTime = 5.0f; // 繝ｪ繧ｹ繝昴・繝ｳ縺吶ｋ縺ｾ縺ｧ縺ｮ譎る俣 (5遘・
	float respawnTimer = 0.0f; // 繝ｪ繧ｹ繝昴・繝ｳ繧ｿ繧､繝槭・

	bool isRespawning = false;      // 繝輔ぉ繝ｼ繝峨う繝ｳ荳ｭ縺・
	float fadeInDuration = 1.5f;    // 繝輔ぉ繝ｼ繝峨う繝ｳ縺ｫ縺九°繧区凾髢難ｼ・.5遘抵ｼ・
	float fadeInTimer = 0.0f;       // 繝輔ぉ繝ｼ繝峨う繝ｳ逕ｨ繧ｿ繧､繝槭・

	DirectX::XMFLOAT3 halfSize;
	DirectX::XMFLOAT3 size;

	OBB* box;
};

