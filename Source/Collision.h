#pragma once

#include <DirectXMath.h>
#include <algorithm>

// コリジョン
class Collision
{
public:
	// 球と球の交差判定
	static bool IntersectSphereVsSphere(
		const DirectX::XMFLOAT3& positionA,
		float radiusA,
		const DirectX::XMFLOAT3& positionB,
		float radiusB,
		DirectX::XMFLOAT3& outPositionB
	);

	// 円柱と円柱の交差判定
	static bool IntersectCylinderVsCylinder(
		const DirectX::XMFLOAT3& positionA,
		float radiusA,
		float heightA,
		const DirectX::XMFLOAT3& positionB,
		float radiusB,
		float heightB,
		DirectX::XMFLOAT3& outPositionB
	);

	// 球と円柱の交差判定
	static bool IntersectSphereVsCylinder(
		const DirectX::XMFLOAT3& spherePosition,
		float sphereRadius,
		const DirectX::XMFLOAT3& cylinderPosition,
		float cylinderRadius,
		float cylinderHeight,
		DirectX::XMFLOAT3& outCylinderPosition
	);

	//四角形と球の交差判定
	static bool IntersectSphereVsBox(
		const DirectX::XMFLOAT3& spherePosition,//球の中心座標
		float sphereRadius,//球の半径
		const DirectX::XMFLOAT3& boxMin,//四角形の最小座標
		const DirectX::XMFLOAT3& boxMax//四角形の最大座標
	);

};
