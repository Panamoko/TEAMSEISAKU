#include "Factory.h"
#include "GimmicManager.h"
#include "EnemyManager.h"

std::unordered_map<std::string, Factory::CreateFunc>& Factory::Registry()
{
    static std::unordered_map<std::string, CreateFunc> map;
    return map;
}

void Factory::Register(const std::string& className, CreateFunc func)
{
    Registry()[className] = func;
}

std::unique_ptr<GameObject> Factory::Create(const std::string& className)
{
    auto it = Registry().find(className);
    std::unique_ptr<GameObject> object;

    if (it != Registry().end())object = it->second();
    else object = std::make_unique<GameObject>();

    switch (object->type)
    {
        case GameObject::Type::Gimmic:
            if (auto gimmic = dynamic_cast<GimmicBase*>(object.get()))
            {
                GimmicManager::Instance().Add(
                    std::unique_ptr<GimmicBase>(static_cast<GimmicBase*>(object.release())));
            }
            break;

        case GameObject::Type::Enemy:
            if (auto enemy = dynamic_cast<Enemy*>(object.get()))
            {
                EnemyManager::Instance().Register(enemy);
            }
            break;

        default: break;
    }

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
