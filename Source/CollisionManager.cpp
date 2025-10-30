#include "CollisionManager.h"

//オブジェクト登録
void CollisionManager::AddObject(GameObject* obj)
{
	if (obj)objects.push_back(obj);
}

//登録リストをクリア
void CollisionManager::Clear()
{
	objects.clear();
}

//全オブジェクト間の衝突判定
void CollisionManager::CheckAllCollision()
{
}
