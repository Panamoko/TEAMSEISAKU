#include "Dissolve.hlsli"

// 全画面を覆う四角形を頂点シェーダー内で生成する
// (頂点バッファを使わずに描画するテクニック)
VS_OUT main(uint vertexID : SV_VertexID)
{
    VS_OUT output;

    // 0: (-1, 1), 1: (3, 1), 2: (-1, -3) という巨大な三角形を描くことで画面を覆う
    float2 texcoord = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(texcoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    output.texcoord = texcoord;

    return output;
}