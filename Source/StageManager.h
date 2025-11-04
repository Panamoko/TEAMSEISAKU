#pragma once

#include <memory>
#include <string>
#include "Stage.h"
#include "System/ModelRenderer.h"
#include "System/ShapeRenderer.h"

class StageManager
{
public:
	static StageManager& Instance()
	{
		static StageManager instance;
		return instance;
	}

	//コピー禁止
	StageManager(const StageManager&) = delete;
	StageManager& operator = (const StageManager&) = delete;

	//ステージを登録
	void Add(std::shared_ptr<Stage> stage);

	//更新・描画
	void Update(float elapsedTime);
	void Render(const RenderContext& rc, ModelRenderer* renderer);
	void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer);

	//削除
	void Remove(Stage* stage);


private:
	StageManager() = default;
	~StageManager() = default;

	//登録されたすべてのGameObjectへのポインタを保持
	std::vector < std::shared_ptr<Stage>> stages;

};
