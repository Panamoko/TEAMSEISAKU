#include "StageManager.h"

void StageManager::Add(std::shared_ptr<Stage> stage)
{
	stages.push_back(std::move(stage));
}

void StageManager::Update(float elapsedTime)
{
	for (auto& stage : stages)
	{
		stage->Update(elapsedTime);
	}
}

void StageManager::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	for (auto& stage : stages)
	{
		stage->Render(rc, renderer);
	}
}

void StageManager::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
}
