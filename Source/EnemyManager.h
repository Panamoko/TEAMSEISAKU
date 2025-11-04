#pragma once

#include <vector>
//#include <set>
#include "Enemy.h"
#include "GameObject.h"
#include <memory>

// エネミーマネージャー
class EnemyManager
{
private:
	EnemyManager() {}
	~EnemyManager() {}

public:
	// 唯一のインスタンス取得
	static EnemyManager& Instance()
	{
		static EnemyManager instance;
		return instance;
	}

	// 更新処理
	void Update(float elapsedTime);

	// 描画処理
	void Render(const RenderContext& rc, ModelRenderer* renderer);

	//エネミー登録
	void Register(const std::shared_ptr<Enemy>& enemy);

	//エネミー全削除
	void Clear();

	//デバッグプリミティブ描画
	virtual void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer);

	//エネミー数取得
	int GetEnemyCount() const { return static_cast<int>(enemies.size()); }

	//エネミー取得
	std::shared_ptr<Enemy> GetEnemy(int index)
	{ 
		if (index < 0 || index >= static_cast<int>(enemies.size()))
		{
			return nullptr;
		}
		return enemies.at(index); 
	}

	// エネミー削除
	//void Remove(Enemy* enemy);

private:
	// エネミー同士の衝突処理
	void CollisionEnemyVsEnemies();

private:
	std::vector<std::shared_ptr<Enemy>>	enemies;
	//std::set<Enemy*>		removes;

	std::vector<std::shared_ptr<GameObject>> objects;
};
