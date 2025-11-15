#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include "GameObject.h"

class GameObjectManager
{
public:
	static GameObjectManager& Instance()
	{
		static GameObjectManager instance;
		return instance;
	}

	//登録・削除
	void AddObject(const std::shared_ptr<GameObject>& object);
	void RemoveObject(int id);

	//情報取得
	std::vector<std::shared_ptr<GameObject>> GetAllObjects() const;
	std::shared_ptr<GameObject> FindByID(int id) const;

	//管理全消去
	void Clear();

private:
	GameObjectManager() = default;

	std::unordered_map<int, std::shared_ptr<GameObject>> objectTable; // ID→オブジェクト
	std::vector<std::shared_ptr<GameObject>> objectList;              // 登録順リスト
};

