#pragma once

#include <vector>
#include <memory>
#include <string>
#include "GimmicBase.h"
#include "System/ModelRenderer.h"
#include "System/ShapeRenderer.h"

class GimmicManager
{
public:
    // シングルトン化
    static GimmicManager& Instance()
    {
        static GimmicManager instance;
        return instance;
    }

    //ギミックを登録
    void Add(std::unique_ptr<GimmicBase> gimmic);

    //更新・描画
    void Update(float elapsedTime);
    void Render(const RenderContext& rc, ModelRenderer* renderer);
    void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer);

    //削除処理
    void RemoveInactive();

    //全ギミックへのアクセス
    std::vector<std::unique_ptr<GimmicBase>>& GetAll(){ return gimmicks; }

private:
    GimmicManager() = default;
    std::vector < std::unique_ptr<GimmicBase>> gimmicks;
};

