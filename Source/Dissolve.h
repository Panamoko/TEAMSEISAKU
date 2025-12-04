#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <string>

// 前方宣言
struct RenderContext;

class Dissolve
{
public:
    Dissolve();
    ~Dissolve() = default;

    // 初期化
    void Initialize(ID3D11Device* device, const char* noiseTexturePath);

    // 描画実行
    // threshold: 0.0(なし) ～ 1.0(全画面塗りつぶし)
    void Render(ID3D11DeviceContext* dc, float threshold);

private:
    struct CbDissolve
    {
        DirectX::XMFLOAT4 color;      // 塗りつぶし色
        float threshold;              // 閾値
        float edgeWidth;              // エッジ幅
        DirectX::XMFLOAT2 padding;
        DirectX::XMFLOAT4 edgeColor;  // エッジ色
    };

    Microsoft::WRL::ComPtr<ID3D11VertexShader>       vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>        pixelShader;
    Microsoft::WRL::ComPtr<ID3D11Buffer>             constantBuffer;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> noiseTextureSRV;
    Microsoft::WRL::ComPtr<ID3D11SamplerState>       samplerState;
};