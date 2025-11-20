#include "SceneFactory.h"
#include <map>
#include <iostream>

Scene* SceneFactory::CreateScene(const std::string& className)
{
    //マップからクラス名に対応するエントリを検索
    auto it = s_creators.find(className);

    if (it == s_creators.end())
    {
        //登録されていないクラス名の場合
        std::cerr << "Error: Scene class '" << className << "' not registered." << std::endl;
        return nullptr; // 生成に失敗
    }

    //見つかった生成関数 (it->second) を呼び出し、Scene* を返す
    SceneCreatorFunc creator = it->second;
    return creator();
}

void SceneFactory::RegisterScene(const std::string& className, SceneCreatorFunc func)
{
    //マップにクラス名と生成関数ポインタを関連付けて登録
    s_creators[className] = func;

    //デバッグ用: 登録されたシーンを表示
    std::cout << "Registered Scene: " << className << std::endl;
}
