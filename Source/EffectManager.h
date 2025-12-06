#pragma once
#include <Effekseer.h>
#include <EffekseerRendererDX11.h>
#include <string>
#include <map>
#include <vector>
#include <d3d11.h>
#include <DirectXMath.h>

class EffectManager
{
public:
    static EffectManager& Instance() {
        static EffectManager instance;
        return instance;
    }

    // 初期化・終了
    void Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    void Finalize();

    // 毎フレーム呼ぶ
    void Update(float elapsedTime); // 時間経過
    void Render();                  // 描画

    // エフェクト読み込み
    void Load(const std::string& name, const std::wstring& path);

    // エフェクト再生 (戻り値はハンドル。停止などに使う)
    Effekseer::Handle Play(const std::string& name, const DirectX::XMFLOAT3& position, float frameDuration = 0.0f);

    // 任意のエフェクトを停止させる関数
    void Stop(Effekseer::Handle handle);

    // スケール設定
    void SetScale(Effekseer::Handle handle, float x, float y, float z);

    // エフェクトの行列（位置・回転・スケール）を更新する
    void SetMatrix(Effekseer::Handle handle, const DirectX::XMFLOAT4X4& matrix);

    // 全停止
    void StopAll();

private:
    EffectManager() = default;
    ~EffectManager() = default;

    ::Effekseer::ManagerRef manager = nullptr;
    ::EffekseerRendererDX11::RendererRef renderer = nullptr;

    // 読み込んだエフェクトのキャッシュ
    std::map<std::string, ::Effekseer::EffectRef> effects;

    // 再生中のエフェクトを管理する構造体
    struct ActiveEffect {
        Effekseer::Handle handle;
        float remainingFrames; // 残りフレーム数
    };
    std::vector<ActiveEffect> activeEffects; // 管理リスト
};