#pragma once

#include "System/ModelRenderer.h"
#include "character.h"

// エネミー
class Enemy : public Character
{
public:
	Enemy() {}
	~Enemy() override {}

	// 更新処理
	virtual void Update(float elapsedTime) = 0;

	// 描画処理
	virtual void Render(const RenderContext& rc, ModelRenderer* renderer) = 0;

	//破棄
	void Destroy();
};
