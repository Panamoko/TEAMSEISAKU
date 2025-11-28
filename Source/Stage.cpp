#include "Stage.h"
#include "Factory.h"

//コンストラクタ
Stage::Stage()
{
    type = Type::Stage;
    class_name = "Stage";

    //ステージモデルを読み込み
    model = ModelManager::Instance().Load("Data/Model/Stage/Zimensi.mdl");
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
   // DirectX::XMStoreFloat4x4(&transform, DirectX::XMMatrixIdentity());

    renderer->Render(rc, transform, model, ShaderId::Lambert, { 0,1,0,1 });
}

REGISTER_GAMEOBJECT(Stage);


