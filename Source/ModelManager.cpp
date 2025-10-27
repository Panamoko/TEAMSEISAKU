#include "ModelManager.h"
#include <filesystem>

Model* ModelManager::Load(const std::string& path)
{
    std::filesystem::path p(path);

    //ファイルパスを正規化
    std::string name;
    try {
        name = std::filesystem::canonical(p).string();
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        //ファイルが存在しない場合のエラー処理
        return nullptr;
    }

    // すでにロード済みなら再利用
    if (auto it = model_map_.find(name); it != model_map_.end())
        return it->second;

    // 新規ロード
    auto model = std::make_unique<Model>(path.c_str());
    model->path = path;
    Model* ptr = model.get();
    model_map_[name] = ptr;
    models_.push_back(std::move(model));

    return ptr;
}