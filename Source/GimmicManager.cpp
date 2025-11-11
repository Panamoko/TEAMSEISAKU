#include "GimmicManager.h"

//ギミック登録
void GimmicManager::Add(std::shared_ptr<GimmicBase> gimmic)
{
	gimmicks.push_back(std::move(gimmic));
}

//ギミック更新
void GimmicManager::Update(float elapsedTime)
{
	for (auto& gimmic : gimmicks)
	{
		if (!gimmic)continue;
		gimmic->Update(elapsedTime);

	}
	RemoveInactive();
}

//ギミック描画
void GimmicManager::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	for (auto& gimmic : gimmicks)
	{
		gimmic->Render(rc, renderer);
	}
}

//デバッグ用描画
void GimmicManager::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
	for (auto& gimmic : gimmicks)
	{
		gimmic->RenderDebugPrimitive(rc, renderer);
	}
}

//ギミック削除
void GimmicManager::RemoveInactive()
{
	/*
		std::remove_if(...)
		配列の中身を「残すもの」と「消すもの」に分けて整理

		[](const std::unique_ptr<GimmicBase>& gimmic)
		{
			return !gimmic->IsActive();
		}),
		削除条件を指定する

		gimmicks.erase(if,gimmicks.end())
		削除対象をすべて削除
	*/
	gimmicks.erase(
		std::remove_if(gimmicks.begin(), gimmicks.end(),
			[](const std::shared_ptr<GimmicBase>& gimmic)
			{
				//nullまたは非アクティブなら削除対象
				return !gimmic || !gimmic->IsActive();
			}),
		gimmicks.end());
}

void GimmicManager::Remove(GimmicBase* gimmic)
{
	if (!gimmic)return;

	auto it = std::find_if(gimmicks.begin(), gimmicks.end(),
		[gimmic](const std::shared_ptr<GimmicBase>& g)
		{
			return g.get() == gimmic;
		});

	if (it != gimmicks.end())
	{
		gimmicks.erase(it);
	}
}
