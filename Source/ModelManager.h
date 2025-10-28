#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include "System/Model.h"
#include "GameObject.h"

class ModelManager :public GameObject
{
public:
    static ModelManager& Instance()
    {
        static ModelManager instance;
        return instance;
    }

    // モデルロード（同名ファイルは再利用）
    Model* Load(const std::string& path);

    // 登録済みモデルを全取得（Editor用）
    const std::vector<std::unique_ptr<Model>>& GetModels() const { return models_; }

    // 全モデルの更新
    void UpdateAllTransforms()
    {
        for (auto& m : models_)
            m->UpdateTransform();
    }

private:
    ModelManager() = default;
    std::vector<std::unique_ptr<Model>> models_;
    std::unordered_map<std::string, Model*> model_map_;
};