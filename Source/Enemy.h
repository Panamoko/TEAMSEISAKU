#pragma once

#include "System/ModelRenderer.h"
#include "character.h"

// エネミー
class Enemy : public Character
{
public:
	Enemy() { type = GameObject::Type::Enemy; }
	~Enemy() override {}

	// 更新処理
	virtual void Update(float elapsedTime) = 0;

	// 描画処理
	virtual void Render(const RenderContext& rc, ModelRenderer* renderer) = 0;

	//破棄
	void Destroy();

	// 破棄予定か確認
	bool IsDestroyRequested() const { return destroyRequested; }

private:
	bool destroyRequested = false; // 削除フラグ
};
