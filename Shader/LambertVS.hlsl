#include "Lambert.hlsli"
#include "Skinning.hlsli"

VS_OUT main(
	float4 position : POSITION,
	float4 boneWeights : BONE_WEIGHTS,
	uint4 boneIndices : BONE_INDICES,
	float2 texcoord : TEXCOORD,
	float3 normal : NORMAL)
{
    VS_OUT vout = (VS_OUT) 0;

	// ★追加: 変形前の座標(足元がY=0付近)を保存しておく
    vout.localPos = position.xyz;

	// 以下、通常のアニメーション計算
    position = SkinningPosition(position, boneWeights, boneIndices);
    vout.vertex = mul(position, viewProjection);
    vout.texcoord = texcoord;
    vout.normal = SkinningVector(normal, boneWeights, boneIndices);
	
	// ワールド座標も渡す（リムライト等で使用）
    vout.position = position.xyz;

    return vout;
}