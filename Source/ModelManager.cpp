#include "ModelManager.h"
#include "System/Graphics.h" // Device取得用
#include <filesystem>

//パスの正規化ヘルパー
static std::string GetNormalizedName(const std::string& path)
{
    std::filesystem::path p(path);
    try {
        return std::filesystem::canonical(p).string();
    }
    catch (...) {
        return "";
    }
}

Model* ModelManager::Load(const std::string& path)
{
    std::string name = GetNormalizedName(path);
    if (name.empty()) return nullptr;

    // すでにロード済みなら再利用
    if (auto it = model_map_.find(name); it != model_map_.end())
        return it->second;

    // 新規ロード（ModelResourceも内部で新規作成される）
    auto model = std::make_unique<Model>(path.c_str());
    model->path = path;
    Model* ptr = model.get();
    model_map_[name] = ptr;
    models_.push_back(std::move(model));

    return ptr;
}

//共有リソースを使って新しいModelを作成
Model* ModelManager::CreateNewInstance(const std::string& path)
{
    // リソースを取得（キャッシュにあればそれを使う）
    auto resource = GetResource(path);
    if (!resource) return nullptr;

    // リソースを渡してModelを作成（ディスクロードは発生しない）
    auto model = std::make_unique<Model>(resource, path.c_str());
    model->path = path;
    Model* ptr = model.get();

    // models_ に追加して管理対象にする（更新や解放のため）
    models_.push_back(std::move(model));

    return ptr;
}

//リソースの取得・キャッシュ
std::shared_ptr<ModelResource> ModelManager::GetResource(const std::string& path)
{
    std::string name = GetNormalizedName(path);
    if (name.empty()) return nullptr;

    // キャッシュにあれば返す
    if (auto it = resource_map_.find(name); it != resource_map_.end())
    {
        return it->second;
    }

    // なければロードして登録
    auto resource = std::make_shared<ModelResource>();
    resource->Load(Graphics::Instance().GetDevice(), path.c_str());
    resource_map_[name] = resource;

    return resource;
}

std::unique_ptr<Model> ModelManager::CreateUniqueInstance(const std::string& path)
{
    // リソースを取得（キャッシュにあればそれを使う）
    auto resource = GetResource(path);
    if (!resource) return nullptr;

    // リソースを渡して新しいModelを作成
    // models_.push_back(...) をしないのがポイントです
    return std::make_unique<Model>(resource, path.c_str());
}