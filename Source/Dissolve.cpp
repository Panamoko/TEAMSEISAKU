#include "Dissolve.h"
#include "System/GpuResourceUtils.h" 
#include "System/Misc.h"

using namespace DirectX;

Dissolve::Dissolve() {}

void Dissolve::Initialize(ID3D11Device* device, const char* noiseTexturePath)
{
    // 1. ノイズテクスチャ読み込み
    HRESULT hr = GpuResourceUtils::LoadTexture(device, noiseTexturePath, noiseTextureSRV.GetAddressOf());
    if (FAILED(hr))
    {
        // 読み込み失敗時は白テクスチャで代替
        GpuResourceUtils::CreateDummyTexture(device, 0xFFFFFFFF, noiseTextureSRV.GetAddressOf());
    }

    // 2. 定数バッファ作成
    GpuResourceUtils::CreateConstantBuffer(device, sizeof(CbDissolve), constantBuffer.GetAddressOf());

    // 3. シェーダー読み込み
    // InputLayoutはnullptrでOK（VS内で頂点生成するため）
    GpuResourceUtils::LoadVertexShader(
        device, "Data/Shader/DissolveVS.cso", nullptr, 0, nullptr, vertexShader.GetAddressOf());

    GpuResourceUtils::LoadPixelShader(
        device, "Data/Shader/DissolvePS.cso", pixelShader.GetAddressOf());

    // 4. サンプラー作成
    D3D11_SAMPLER_DESC smpDesc = {};
    smpDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    smpDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    smpDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    smpDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    device->CreateSamplerState(&smpDesc, samplerState.GetAddressOf());
}

void Dissolve::Render(ID3D11DeviceContext* dc, float threshold)
{
    if (threshold <= 0.0f) return; // 透明なら描画しない

    // 定数バッファ更新
    CbDissolve cb;
    cb.color = { 0.0f, 0.0f, 0.0f, 1.0f };     // 黒色
    cb.threshold = threshold;
    cb.edgeWidth = 0.04f;                      // エッジの太さ
    cb.edgeColor = { 0.0f, 0.5f, 1.0f, 1.0f }; // エッジ色（青系）

    dc->UpdateSubresource(constantBuffer.Get(), 0, nullptr, &cb, 0, 0);

    // パイプライン設定
    ID3D11Buffer* nullVB = nullptr;
    UINT stride = 0, offset = 0;
    dc->IASetVertexBuffers(0, 1, &nullVB, &stride, &offset);
    dc->IASetInputLayout(nullptr);
    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    dc->VSSetShader(vertexShader.Get(), nullptr, 0);
    dc->PSSetShader(pixelShader.Get(), nullptr, 0);

    dc->PSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());
    dc->PSSetShaderResources(0, 1, noiseTextureSRV.GetAddressOf());
    dc->PSSetSamplers(0, 1, samplerState.GetAddressOf());

    // 描画（3頂点で画面全体を覆う）
    dc->Draw(3, 0);

    // 後始末
    ID3D11ShaderResourceView* nullSRV = nullptr;
    dc->PSSetShaderResources(0, 1, &nullSRV);
}