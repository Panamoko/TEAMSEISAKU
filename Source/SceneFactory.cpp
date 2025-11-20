#include "SceneFactory.h"
#include <map>
#include <iostream>

Scene* SceneFactory::CreateScene(const std::string& className)
{
    auto creators = GetCreatorMap();//マップ取得

    auto it = creators.find(className);

    if (it == creators.end())
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
    GetCreatorMap()[className] = func;

    //デバッグ用: 登録されたシーンを表示
    std::cout << "Registered Scene: " << className << std::endl;
}

//シーン名を取得
std::vector<std::string> SceneFactory::GetRegisteredNames()
{
    std::vector<std::string> names;

    //s_creatorsマップの要素を一つずつ巡回
    for (const auto& pair : GetCreatorMap())
    {
        //マップのキー (pair.first) がシーンクラス名（文字列）なので、これをリストに追加
        names.push_back(pair.first);
    }

    return names;
}

std::map<std::string, SceneCreatorFunc>& SceneFactory::GetCreatorMap()
{
    static std::map<std::string, SceneCreatorFunc> s_creators_local;
    return s_creators_local;
}
