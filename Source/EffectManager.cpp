#include "EffectManager.h"
#include "System/Graphics.h" // Device取得用
#include "Camera.h"          // カメラ情報取得用

using namespace DirectX;

void EffectManager::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
    // 1. レンダラーの作成 (最大描画スプライト数: 2000)
    renderer = ::EffekseerRendererDX11::Renderer::Create(device, context, 2000);

    // 2. マネージャーの作成 (最大インスタンス数: 2000)
    manager = ::Effekseer::Manager::Create(2000);

    // 3. 描画モジュールとマネージャーの紐づけ
    manager->SetSpriteRenderer(renderer->CreateSpriteRenderer());
    manager->SetRibbonRenderer(renderer->CreateRibbonRenderer());
    manager->SetRingRenderer(renderer->CreateRingRenderer());
    manager->SetTrackRenderer(renderer->CreateTrackRenderer());
    manager->SetModelRenderer(renderer->CreateModelRenderer());

    // 4. 座標系の設定 (DirectXは左手系)
    manager->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);
}

void EffectManager::Finalize()
{
    // エフェクトの破棄
    // ES_SAFE_RELEASE は古いマクロなので、参照カウントを下げる(nullptr代入)だけでOKです
    for (auto& pair : effects) {
        pair.second = nullptr; // これで解放されます
    }
    effects.clear();

    // マネージャーとレンダラーの破棄
    // Destroy() は不要になりました。nullptrを代入すれば解放されます。
    manager = nullptr;
    renderer = nullptr;
}

void EffectManager::Load(const std::string& name, const std::wstring& path)
{
    if (effects.find(name) != effects.end()) return;

    // Create は EffectRef を返します
    auto effect = Effekseer::Effect::Create(manager, (const char16_t*)path.c_str());
    if (effect != nullptr) { // スマートポインタ的なチェック
        effects[name] = effect;
    }
}

Effekseer::Handle EffectManager::Play(const std::string& name, const DirectX::XMFLOAT3& position)
{
    if (effects.find(name) == effects.end()) return -1;

    // 再生
    Effekseer::Handle handle = manager->Play(effects[name], position.x, position.y, position.z);
    return handle;
}

void EffectManager::Update(float elapsedTime)
{
    // マネージャーの更新 (単位はフレーム。60fps想定なら elapsedTime * 60.0f)
    manager->Update(elapsedTime * 60.0f);
}

void EffectManager::Render()
{
    if (manager.Get() == nullptr || renderer.Get() == nullptr) return;

    // カメラ情報の取得
    Camera& cam = Camera::Instance();

    // ビュー行列の変換 (XMFLOAT4X4 -> Effekseer::Matrix44)
    const auto& v = cam.GetView();
    Effekseer::Matrix44 viewMatrix;
    for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) viewMatrix.Values[i][j] = v.m[i][j];

    // プロジェクション行列の変換
    const auto& p = cam.GetProjection();
    Effekseer::Matrix44 projMatrix;
    for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) projMatrix.Values[i][j] = p.m[i][j];

    // レンダラーにカメラ行列を設定
    renderer->SetCameraMatrix(viewMatrix);
    renderer->SetProjectionMatrix(projMatrix);

    // 描画開始
    renderer->BeginRendering();
    manager->Draw();
    renderer->EndRendering();
}

void EffectManager::StopAll()
{
    manager->StopAllEffects();
}