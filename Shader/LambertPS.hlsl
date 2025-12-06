#include "Lambert.hlsli"

cbuffer CbMesh : register(b1)
{
    float4 materialColor;
};

Texture2D DiffuseMap : register(t0);
SamplerState LinearSampler : register(s0);

float4 main(VS_OUT pin) : SV_TARGET
{
    float4 color = DiffuseMap.Sample(LinearSampler, pin.texcoord) * materialColor;

    float3 N = normalize(pin.normal);
    float3 L = normalize(-lightDirection.xyz);
    float power = max(0, dot(L, N));

    power = power * 0.5 + 0.5f;
    color.rgb *= power;

	// --- ★ここから修正: 足元発光処理 ---
    if (materialColor.a > 1.0f)
    {
		// 1. リムライト（前回の修正分。不要なら削除してください）
        float3 viewDir = normalize(cameraPosition.xyz - pin.position);
        float rim = 1.0 - saturate(dot(N, viewDir));
        rim = pow(rim, 5.0f);
        float3 rimColor = float3(1.0, 1.0, 1.0) * rim * 2.0;
        color.rgb += rimColor;

		// 2. 足元発光（グラデーション）
		// モデルのY座標（高さ）を取得
        float height = pin.localPos.y;

		// 発光させる高さの上限（モデルのサイズに合わせて調整してください）
		// 例: 高さ 0.0(足元) ～ 1.5(膝くらい) まで光らせる
        float glowHeight = 0.05f;

		// 足元ほど強く(1.0)、上に行くほど弱く(0.0)なる計算
		// saturateで 0.0～1.0 に範囲制限
        float glow = 1.0 - saturate(height / glowHeight);

		// 発光を鋭くする（2乗や3乗すると、地面スレスレだけ強く光るようになります）
        glow = pow(glow, 2.0f);

		// 発光色（シアン色）を作成
        float3 footGlowColor = float3(1.0f, 1.0f, 1.0f) * glow * 2.0f; // * 2.0f は強さ

		// 色を加算
        color.rgb += footGlowColor;

		// アルファ値を戻す
        color.a = 1.0f;
    }
	// --- 修正ここまで ---
	
    return color;
}