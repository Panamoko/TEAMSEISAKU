#pragma once

#include "Editor.h"
#include "System/ModelRenderer.h"
#include "ModelManager.h"
#include "GameObject.h"

//ステージ
class Stage :public GameObject
{
public:
    Stage();
    ~Stage();

    //更新処理
    void Update(float elapsedTime);

    //描画処理
    void Render(const RenderContext& rc, ModelRenderer* renderer);

    void Clear();

private:
    //std::vector<std::unique_ptr<Model>> models;
    //Model* model = nullptr;
    editor game_editor;
    std::vector<std::unique_ptr<GameObject>> objects;
    std::vector<std::unique_ptr<GameSprite>> sprites2d;

};


