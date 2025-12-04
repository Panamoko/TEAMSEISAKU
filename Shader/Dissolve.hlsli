// 頂点シェーダーからピクセルシェーダーへの受け渡しデータ
struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

// C++側から送られてくる定数バッファ (b0)
cbuffer CbDissolve : register(b0)
{
    float4 color; // フェード色 (黒など)
    float threshold; // 進行度 (0.0:透明 -> 1.0:真っ暗)
    float edgeWidth; // エッジの幅
    float2 padding; // パディング
    float4 edgeColor; // エッジの色
};