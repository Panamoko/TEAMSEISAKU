#pragma once

#include "GameObjectManager.h"

void GameObjectManager::AddObject(const std::shared_ptr<GameObject>& object)
{
	if (!object)return;

	int id = object->GetID();

	if (objectTable.find(id) == objectTable.end())
	{
		objectTable[id] = object;
		objectList.push_back(object);
	}
}

void GameObjectManager::RemoveObject(int id)
{
	if (objectTable.empty())return;

	auto it = objectTable.find(id);
	if (it != objectTable.end())
	{
		objectTable.erase(it);
		objectList.erase(
			std::remove_if(objectList.begin(), objectList.end(),
				[&](const std::shared_ptr<GameObject>& obj)
				{
					return obj->GetID() == id;
				}),
			objectList.end()
		);
	}
}

std::vector<std::shared_ptr<GameObject>> GameObjectManager::GetAllObjects() const
{
	return objectList;
}

std::shared_ptr<GameObject> GameObjectManager::FindByID(int id) const
{
	auto it = objectTable.find(id);
	if (it != objectTable.end())
	{
		return it->second;
	}
	return nullptr;
}

void GameObjectManager::Clear()
{
	objectTable.clear();
	objectList.clear();
}
