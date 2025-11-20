#include "SceneFactory.h"

Scene* SceneFactory::CreateScene(const std::string& className)
{
    return nullptr;
}

void SceneFactory::RegisterScene(const std::string& className, SceneCreatorFunc func)
{
}
