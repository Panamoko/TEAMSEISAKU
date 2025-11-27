#include "SpriteManager.h"
#include "System/Graphics.h" 
#include <filesystem>

//パスの正規化ヘルパー
std::string SpriteManager::GetNormaledName(const std::string& path)
{
    std::filesystem::path p(path);
    try {
        return std::filesystem::canonical(p).string();
    }
    catch (...) {
        return "";
    }
}

//リソースの取得・キャッシュ
std::shared_ptr<SpriteResource> SpriteManager::GetResource(const std::string& path)
{
    std::string name = GetNormaledName(path);
    if (name.empty()) return nullptr;

    // キャッシュにあれば返す
    if (auto it = resource_map_.find(name); it != resource_map_.end())
    {
        return it->second;
    }

    // なければロードして登録
    auto resource = std::make_shared<SpriteResource>();
    // Resource::Load を呼び出す
    if (FAILED(resource->Load(Graphics::Instance().GetDevice(), path.c_str())))
    {
        return nullptr;
    }

    resource_map_[name] = resource;

    return resource;
}

//共有リソースを使って新しい Sprite を作成
Sprite* SpriteManager::CreateNewInstance(const std::string& path)
{
    //リソースの取得
    auto resource = GetResource(path);
    if (!resource) return nullptr;



    return nullptr;
}
