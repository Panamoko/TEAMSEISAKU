#pragma once

#include <vector>
#include <memory>

#include "GameObject.h"
#include "Collision.h"

class CollisionManager
{
public:
	//インスタンス取得
	static CollisionManager& Instance()
	{
		static CollisionManager instance;
		return instance;
	}

	//コピー禁止
	CollisionManager(const CollisionManager&) = delete;
	CollisionManager& operator = (const CollisionManager&) = delete;

	//オブジェクト登録
	void AddObject(GameObject* obj);

	//オブジェクトリストをクリア
	void Clear();

	//全オブジェクト間の衝突判定
	void CheckAllCollision();

private:
	CollisionManager() = default;
	~CollisionManager() = default;

	//登録されたすべてのGameObjectへのポインタを保持
	std::vector<GameObject*> objects;
};

