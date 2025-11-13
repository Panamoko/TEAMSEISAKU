#pragma once

#include <DirectXMath.h>
#include <algorithm>
#include "Collider.h"

//struct OBB {
//	DirectX::XMFLOAT3 center; // 中心座標
//	DirectX::XMFLOAT3 half;   // ハーフサイズ（X,Y,Z）
//	float yaw;                // Y軸回転（ラジアン）
//};

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
	static bool IntersectCylinderVsCylinder2(
		const DirectX::XMFLOAT3& positionA,
		float radiusA,
		float heightA,
		const DirectX::XMFLOAT3& positionB,
		float radiusB,
		float heightB,
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
		DirectX::XMFLOAT3& mtd
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

	// 球 vs OBB（MTD=最小押し出しベクトルを任意で返す）
	static bool IntersectSphereVsOBB(
		const DirectX::XMFLOAT3& sphereCenter,
		float sphereRadius,
		const OBB& box,
		DirectX::XMFLOAT3* outMTD = nullptr
	);

	// 円柱 vs OBB（円柱はXZ円＋高さ。MTD任意）
	static bool IntersectCylinderVsOBB(
		const DirectX::XMFLOAT3& cylCenter,
		float cylRadius,
		float cylHeight,
		const OBB& box,
		DirectX::XMFLOAT3* outMTD = nullptr
	);

	//円柱 VS 箱
	static bool IntersectCylinderVsAABB(
		const CylinderCollider* cylinder,
		const BoxCollider* box
	);

	static void OBBtoAABB(
		const DirectX::XMFLOAT3& center,
		const DirectX::XMFLOAT3& half,
		const DirectX::XMFLOAT3 axis[3],
		DirectX::XMFLOAT3& outPos,
		DirectX::XMFLOAT3& outSize
	);

	//重さ考慮付き押し出し処理
	static void ApplyPushOutWithWeight(
		GameObject* objA,
		GameObject* objB,
		const DirectX::XMFLOAT3& normal,
		float penetration
	);

	//円柱と円柱の交差判定（MTDを計算せず、衝突情報のみ返す）
	static bool IntersectCylinder_Vs_Cylinder(
		CylinderCollider* cylinderA,
		CylinderCollider* cylinderB,
		DirectX::XMFLOAT3& outNormal,
		float& outPenetrarion
	);
};
