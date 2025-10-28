#include "Stage.h"

//コンストラクタ
Stage::Stage()
{
    class_name = "Stage";

    //ステージモデルを読み込み
    model = ModelManager::Instance().Load("Data/Model/Stage/ExampleStage.mdl");
}

Stage::~Stage()
{
    //ステージモデルを破棄
    //delete model;
}

void Stage::Update(float elapsedTime)
{
    //今は特にやることはない
}

void Stage::Render(const RenderContext& rc, ModelRenderer* renderer)
{
    DirectX::XMFLOAT4X4 transform;
    DirectX::XMStoreFloat4x4(&transform, DirectX::XMMatrixIdentity());

    // Editor に ModelManager の全モデルを渡す
    game_editor.render(objects, sprites2d, ModelManager::Instance().GetModels(), renderer);

    for (auto& obj : objects)
    {
        if (!obj || !obj->model) continue;
        renderer->Render(rc, obj->transform, obj->model, ShaderId::Lambert);
    }
    //renderer->Render(rc, transform, model, ShaderId::Lambert);
}

REGISTER_GAMEOBJECT(Stage);


