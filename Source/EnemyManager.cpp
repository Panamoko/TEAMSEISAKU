#include "EnemyManager.h"
#include "Collision.h"
#include <algorithm>
#include <vector>

// 更新処理
void EnemyManager::Update(float elapsedTime)
{
	for (auto& enemy : enemies) // ★ auto& に変更
	{
		// 削除リクエスト済みのものは更新しない
		if (enemy->IsDestroyRequested()) continue;
		enemy->Update(elapsedTime);
	}

	auto it = std::remove_if(enemies.begin(), enemies.end(),
		[](const std::shared_ptr<Enemy>& enemy) {
			return enemy->IsDestroyRequested();
		});
	enemies.erase(it, enemies.end());
}

// 描画処理
void EnemyManager::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	for (auto enemy : enemies)
	{
		enemy->Render(rc, renderer);
	}
}

//エネミー登録
void EnemyManager::Register(const std::shared_ptr<Enemy>& enemy)
{
	enemies.push_back(enemy);
}

void EnemyManager::Clear()
{
	enemies.clear();
	//removes.clear();
	//for (auto enemy : enemies)
	//{
	//	delete enemy;
	//}
	//enemies.clear();
}

void EnemyManager::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
	for (auto enemy : enemies)
	{
		enemy->RenderDebugPrimitive(rc, renderer);
	}
	for (auto& obj : objects)
	{
		obj->RenderDebugPrimitive(rc, renderer);
	}
}


void EnemyManager::CollisionEnemyVsEnemies()
{
	size_t enemyCount = enemies.size();
	for (int i = 0; i < enemyCount; ++i)
	{
		auto enemyA = enemies.at(i);
		for (int j = i + 1; j < enemyCount; ++j)
		{
			auto enemyB = enemies.at(j);

			DirectX::XMFLOAT3 outPosition;
			//if (Collision::IntersectSphereVsSphere(
			//	enemyA->GetPosition(),
			//	enemyA->GetRadius(),
			//	enemyB->GetPosition(),
			//	enemyB->GetRadius(),
			//	outPosition))
			//{
			//	enemyB->SetPosition(outPosition);
			//}

			if (Collision::IntersectCylinderVsCylinder(
				enemyA->GetPosition(),
				enemyA->GetRadius(),
				enemyA->GetHeight(),
				enemyB->GetPosition(),
				enemyB->GetRadius(),
				enemyB->GetHeight(),
				outPosition))
			{
				enemyB->SetPosition(outPosition);
			}

		}
	}
}

void EnemyManager::Remove(Enemy* enemy)
{
	auto it = std::find_if(enemies.begin(), enemies.end(),
		[enemy](const std::shared_ptr<Enemy>& e) {
			return e.get() == enemy; // 生ポインタを比較
		});

	if (it != enemies.end())
	{
		enemies.erase(it);
	}
	// 破棄リストに追加
	//removes.insert(enemy);
}
