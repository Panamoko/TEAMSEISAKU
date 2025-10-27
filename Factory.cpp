#include "Factory.h"

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
    if (it != Registry().end())
    {
        return it->second();
    }

    //登録されていない場合はデフォルト
    return std::make_unique<GameObject>();
}
