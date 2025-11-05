#pragma once

#include <vector>
#include <memory>

#include "Collision.h"

class GameObject;

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

	////個別削除
	void Remove(GameObject* obj);

	//全オブジェクト間の衝突判定
	void CheckAllCollision();

private:
	CollisionManager() = default;
	~CollisionManager() = default;

	void ProcessPendingremovals();//衝突後に削除実行

	//登録されたすべてのGameObjectへのポインタを保持
	std::vector<GameObject*> objects;
	std::vector<GameObject*> pendingRemovals;
	bool inUpdate = false;
};

