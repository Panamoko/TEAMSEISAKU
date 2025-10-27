#pragma once

#include <vector>
#include <string>
#include <memory>
#include "Editor.h"

class GameObjectManager
{
public:
	static GameObjectManager& Instance()
	{
		static GameObjectManager instance;
		return instance;
	}

	//新しいGameObjectを管理リストに追加
	void AddObject(std::shared_ptr<GameObject> obj)
	{
		objects.push_back(obj);
	}

	//名前でGameObjectを検索する
	std::shared_ptr<GameObject> FindObject(const std::string& name)
	{
		for (auto& obj : objects)
		{
			if (obj->name == name)return obj;
		}
		return nullptr;
	}

	std::shared_ptr<GameObject> CreateObjects(const std::string& name)
	{
		auto obj = std::make_shared<GameObject>();
		obj->id = next_id++;
	}

	//登録されている全てのGameObjectを取得
	const std::vector<std::shared_ptr<GameObject>>& GetAllObjects() const { return objects; }

private:
	int next_id = 0;
	std::unordered_map<int, std::shared_ptr<GameObject>>id_map;
	std::unordered_map<std::string, std::shared_ptr<GameObject>> name_mep;

	//GameObjectを保存しているリスト
	std::vector<std::shared_ptr<GameObject>> objects;
};