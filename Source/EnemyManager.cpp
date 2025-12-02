#include "EnemyManager.h"
#include "Collision.h"
#include <algorithm>
#include <vector>

// 更新処理
void EnemyManager::Update(float elapsedTime)
{
	// エネミーの更新
	for (auto& enemy : enemies)
	{
		// 削除リクエスト済みのものは更新しない
		if (enemy->IsDestroyRequested()) continue;
		enemy->Update(elapsedTime);
	}

	// 削除リクエストがあったエネミーを一括削除
	auto it = std::remove_if(enemies.begin(), enemies.end(),
		[](const std::shared_ptr<Enemy>& enemy) {
			return enemy->IsDestroyRequested();
		});
	enemies.erase(it, enemies.end());
}

// 描画処理
void EnemyManager::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	for (auto& enemy : enemies)
	{
		enemy->Render(rc, renderer);
	}
}

// エネミー登録
void EnemyManager::Register(const std::shared_ptr<Enemy>& enemy)
{
	enemies.push_back(enemy);
}

// エネミー全削除
void EnemyManager::Clear()
{
	enemies.clear();
}

// デバッグ表示
void EnemyManager::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
	for (auto& enemy : enemies)
	{
		enemy->RenderDebugPrimitive(rc, renderer);
	}
	for (auto& obj : objects)
	{
		obj->RenderDebugPrimitive(rc, renderer);
	}
}

// 個別削除
void EnemyManager::Remove(Enemy* enemy)
{
	auto it = std::find_if(enemies.begin(), enemies.end(),
		[enemy](const std::shared_ptr<Enemy>& e) {
			return e.get() == enemy; // 生ポインタで比較
		});

	if (it != enemies.end())
	{
		enemies.erase(it);
	}
}