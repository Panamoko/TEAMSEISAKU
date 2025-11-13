#include "Factory.h"
#include "GimmicManager.h"
#include "EnemyManager.h"
#include "StageManager.h"
#include "GameObjectManager.h"

std::unordered_map<std::string, Factory::CreateFunc>& Factory::Registry()
{
    static std::unordered_map<std::string, CreateFunc> map;
    return map;
}

void Factory::Register(const std::string& className, CreateFunc func)
{
    Registry()[className] = func;
}

std::shared_ptr<GameObject> Factory::Create(const std::string& className)
{
    auto it = Registry().find(className);
   std::shared_ptr<GameObject> object;

    if (it != Registry().end())object = it->second();
    else object = std::make_shared<GameObject>();

    switch (object->type)
    {
        case GameObject::Type::Gimmic:
            if (auto gimmic = std::dynamic_pointer_cast<GimmicBase>(object))
            {
                GimmicManager::Instance().Add(gimmic);
            }
            break;

        case GameObject::Type::Enemy:
            if (auto enemy = std::dynamic_pointer_cast<Enemy>(object))
            {
                EnemyManager::Instance().Register(enemy);
            }
            break;

        case GameObject::Type::Stage:
           if (auto stage = std::dynamic_pointer_cast<Stage>(object))
           {
               StageManager::Instance().Add(stage);
           }
           break;

        default: break;
    }

    GameObjectManager::Instance().AddObject(object);

    return object;
}

std::vector<std::string> Factory::GetRegisteredClassNames()
{
    std::vector<std::string> names;
    for (const auto& pair : Registry())
    {
        names.push_back(pair.first);
    }
    return names;
}
