#include "Dissolve.hlsli"

Texture2D noiseTexture : register(t0);
SamplerState Sampler : register(s0);

float4 main(VS_OUT pin) : SV_TARGET
{
    // ノイズ画像から値をサンプリング (0.0 ~ 1.0)
    float noiseValue = noiseTexture.Sample(Sampler, pin.texcoord).r;

    // 進行度(threshold)がノイズ値を超えたら「塗りつぶす」
    if (noiseValue < threshold)
    {
        return color; // 指定色（黒）で描画
    }
    
    // エッジ（境界線）の表現（オプション）
    if (noiseValue < threshold + edgeWidth)
    {
        return edgeColor; // エッジ色で描画
    }

    // それ以外は描画しない（透明＝元のシーンが見える）
    discard;
    return float4(0, 0, 0, 0);
}