#include "Collision.h"

using namespace DirectX;

// 球と球の交差判定
bool Collision::IntersectSphereVsSphere(
	const DirectX::XMFLOAT3& positionA,
	float radiusA,
	const DirectX::XMFLOAT3& positionB,
	float radiusB,
	DirectX::XMFLOAT3& outPositionB)
{
	// A→Bの単位ベクトルを算出
	DirectX::XMVECTOR PositionA = DirectX::XMLoadFloat3(&positionA);
	DirectX::XMVECTOR PositionB = DirectX::XMLoadFloat3(&positionB);
	DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(PositionB, PositionA);
	DirectX::XMVECTOR LengthSq = DirectX::XMVector3LengthSq(Vec);
	float lengthSq;
	DirectX::XMStoreFloat(&lengthSq, LengthSq);

	// 距離判定
	float range = radiusA + radiusB;
	if (lengthSq > range * range)	//2つの円が離れているなら
	{
		return false;
	}

	// AがBを押し出す
	Vec = DirectX::XMVector3Normalize(Vec);
	Vec = DirectX::XMVectorScale(Vec, range);
	PositionB = DirectX::XMVectorAdd(PositionA, Vec);
	DirectX::XMStoreFloat3(&outPositionB, PositionB);

	return true;
}

// 円柱と円柱の交差判定
bool Collision::IntersectCylinderVsCylinder2(
	const DirectX::XMFLOAT3& positionA,
	float radiusA,
	float heightA,
	const DirectX::XMFLOAT3& positionB,
	float radiusB,
	float heightB,
	DirectX::XMFLOAT3& outPositionB)
{
	// Aの足元がBの頭より上なら当たっていない
	if (positionA.y > positionB.y + heightB)
	{
		return false;
	}
	// Aの頭がBの足元より下なら当たっていない
	if (positionA.y + heightA < positionB.y)
	{
		return false;
	}
	// XZ平面での範囲チェック
	float vx = positionB.x - positionA.x;
	float vz = positionB.z - positionA.z;
	float range = radiusA + radiusB;
	float distXZ = sqrtf(vx * vx + vz * vz);
	if (distXZ > range)
	{
		return false;
	}
	// AがBを押し出す
	vx /= distXZ;
	vz /= distXZ;
	outPositionB.x = positionA.x + (vx * range);
	outPositionB.y = positionB.y;
	outPositionB.z = positionA.z + (vz * range);

	return true;
}

// 円柱と円柱の交差判定
bool Collision::IntersectCylinderVsCylinder(
	const DirectX::XMFLOAT3& positionA,
	float radiusA,
	float heightA,
	const DirectX::XMFLOAT3& positionB,
	float radiusB,
	float heightB,
	DirectX::XMFLOAT3& mtd)
{
	//Y軸方向の重なり確認
	float minA = positionA.y;
	float maxA = positionA.y; heightA;
	float minB = positionB.y;
	float maxB = positionB.y + heightB;

	if (maxA < minB || maxB < minA)
	{
		mtd = { 0.0f,0.0f,0.0f };
		return false;
	}

	// XZ平面で距離を調べる
	float dx = positionA.x - positionB.x;
	float dz = positionA.z - positionB.z;

	float distSq = (dx * dx) + (dz * dz);
	float hitDist = radiusA + radiusB;

	if (distSq >= hitDist * hitDist)
	{
		//衝突無し
		mtd = { 0.0f,0.0f,0.0f };
		return false;
	}

	//衝突している → 押し戻し量（MTD）計算
	float dist = std::sqrtf(distSq);

	//完全に同じ位置にある → 適当な方向へ押し出す
	if (dist == 0.0f)
	{
		mtd = { hitDist,0.0f,0.0f };
		return true;
	}

	//めり込み量
	float penetration = hitDist - dist;

	//壁の中心 → A の方向ベクトル
	float nx = dx / dist;
	float nz = dz / dist;

	//A を外側に押し戻すベクトル
	mtd = {
		nx * penetration,
		0.0f,
		nz * penetration
	};

	return true;
}

bool Collision::IntersectSphereVsCylinder(
	const DirectX::XMFLOAT3& spherePosition, 
	float sphereRadius, 
	const DirectX::XMFLOAT3& cylinderPosition, 
	float cylinderRadius, 
	float cylinderHeight, 
	DirectX::XMFLOAT3& outCylinderPosition)
{
	// 高さチェック
	if (spherePosition.y + sphereRadius < cylinderPosition.y) return false;
	if (spherePosition.y - sphereRadius > cylinderPosition.y + cylinderHeight) return false;

	// XZ平面での範囲チェック
	//円柱と円柱の交差判定とほぼ同じ処理。球のパラメータを円柱に見立てる。
	float vx = cylinderPosition.x - spherePosition.x;
	float vz = cylinderPosition.z - spherePosition.z;
	float range = sphereRadius + cylinderRadius;
	float distXZ = sqrtf(vx * vx + vz * vz);
	if (distXZ > range) return false;

	// 球が円柱を押し出す
	vx /= distXZ;
	vz /= distXZ;
	outCylinderPosition.x = spherePosition.x + (vx * range);
	outCylinderPosition.y = cylinderPosition.y;
	outCylinderPosition.z = spherePosition.z + (vz * range);

	return true;
}

/*
 	球の中心座標
	const DirectX::XMFLOAT3& spherePosition
	球の半径
	float sphereRadius
	四角形の最小座標
	const DirectX::XMFLOAT3& boxMin
	四角形の最大座標
	const DirectX::XMFLOAT3& boxMax
	押し出し後の球の位置
	DirectX::XMFLOAT3& outSpherPosition
*/
bool Collision::IntersectSphereVsBox(
	const DirectX::XMFLOAT3& spherePosition,
	float sphereRadius,
	const DirectX::XMFLOAT3& boxMin,
	const DirectX::XMFLOAT3& boxMax)
{
	using namespace DirectX;

	//球の中心からAABB内で一番近い点を求める
	XMFLOAT3 closestPoint;
	closestPoint.x = (std::max)(boxMin.x, (std::min)(spherePosition.x, boxMax.x));
	closestPoint.y = (std::max)(boxMin.y, (std::min)(spherePosition.y, boxMax.y));
	closestPoint.z = (std::max)(boxMin.z, (std::min)(spherePosition.z, boxMax.z));

	//球の中心と最近接点の距離ベクトルを計算
	float dx = spherePosition.x - closestPoint.x;
	float dy = spherePosition.y - closestPoint.y;
	float dz = spherePosition.z - closestPoint.z;
	float distSq = (dx * dx) + (dy * dy) + (dz * dz);

	if (distSq > sphereRadius * sphereRadius)
	{
		return true;
	}

	return false;
}

static inline XMFLOAT3 rotate_to_local_yaw(const XMFLOAT3& v, float yaw) {
	float c = cosf(yaw), s = sinf(yaw);
	// local = R^T * (world-center)
	return XMFLOAT3(c * v.x - s * v.z, v.y, s * v.x + c * v.z);
}
static inline XMFLOAT3 rotate_to_world_yaw(const XMFLOAT3& v, float yaw) {
	float c = cosf(yaw), s = sinf(yaw);
	// world = R * local
	return XMFLOAT3(c * v.x + s * v.z, v.y, -s * v.x + c * v.z);
}

static inline float clampf(float a, float lo, float hi) { return (a < lo) ? lo : (a > hi) ? hi : a; }

bool Collision::IntersectSphereVsOBB(
	const XMFLOAT3& sphereCenter,
	float sphereRadius,
	const OBB& obb,
	XMFLOAT3* outMTD)
{
	// 球の中心 → OBB 中心までの差分ベクトル (ワールド座標系)
	DirectX::XMFLOAT3 diffWorld{
		sphereCenter.x - obb.center.x,
		sphereCenter.y - obb.center.y,
		sphereCenter.z - obb.center.z,
	};

	// ワールド座標系の差分ベクトルを、OBBのローカル軸に射影（内積）する
	auto dot = [](const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
		{
			return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
		};

	float localX = dot(diffWorld, obb.axis[0]);
	float localY = dot(diffWorld, obb.axis[1]);
	float localZ = dot(diffWorld, obb.axis[2]);

	// OBBローカル空間における「球の中心から最も近い点」を探す
	auto clampf = [](float v, float minV, float maxV)
		{
			return (v < minV) ? minV : (v > maxV ? maxV : v);
		};

	float nearestX = clampf(localX, -obb.half.x, obb.half.x);
	float nearestY = clampf(localY, -obb.half.y, obb.half.y);
	float nearestZ = clampf(localZ, -obb.half.z, obb.half.z);

	// 球の中心と「OBB上の最近接点」とのローカル空間での距離を計算
	float diffNearestX = localX - nearestX;
	float diffNearestY = localY - nearestY;
	float diffNearestZ = localZ - nearestZ;

	float distSq = (diffNearestX * diffNearestX) +
		(diffNearestY * diffNearestY) +
		(diffNearestZ * diffNearestZ);

	// 距離の2乗が半径の2乗より大きければ、衝突していない
	if (distSq > (sphereRadius * sphereRadius))
	{
		return false;
	}

	// (MTD計算は省略。必要なら CylinderVsOBB を参考に実装)
	if (outMTD)
	{
		// TODO: MTD計算 (現在は未実装)
		outMTD->x = 0;
		outMTD->y = 0;
		outMTD->z = 0;
	}

	return true;
}

bool Collision::IntersectCylinderVsOBB(
	const XMFLOAT3& cylinderCenter,
	float cylinderRadius,
	float cylinderHeight,
	const OBB& obb,
	XMFLOAT3* outMTD
)
{
	//円柱の中心
	float cylinderCenterY = cylinderCenter.y + cylinderHeight * 0.5f;

	//円柱中心 → OBB 中心までの差分
	DirectX::XMFLOAT3 diffWorld{
		cylinderCenter.x - obb.center.x,
		cylinderCenter.y - obb.center.y,
		cylinderCenter.z - obb.center.z,
	};

	//OBBローカル空間へ射影（軸との内積）
	auto dot = [](const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
		{
			return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
		};

	float localX = dot(diffWorld, obb.axis[0]);
	float localY = dot(diffWorld, obb.axis[1]);
	float localZ = dot(diffWorld, obb.axis[2]);

	//Y軸（高さ方向）のオーバーラップチェック

	float boxMinY = -obb.half.y;
	float boxMaxY = obb.half.y;

	float cylMinY = localY - cylinderHeight * 0.5f;
	float cylMaxY = localY + cylinderHeight * 0.5f;

	float overlapTop = boxMaxY - cylMinY;
	float overlapBottom = cylMaxY - boxMinY;

	if (overlapTop <= 0.0f || overlapBottom <= 0.0f)return false;

	//XZ 平面における「円 vs 長方形」衝突チェック

	auto clampf = [](float v, float minV, float maxV)
		{
			return (v < minV) ? minV : (v > maxV ? maxV : v);
		};

	//円柱中心のローカル位置を長方形内部へクランプ
	float nearestX = clampf(localX, -obb.half.x, obb.half.x);
	float nearestZ = clampf(localZ, -obb.half.z, obb.half.z);

	float diffX = localX - nearestX;
	float diffZ = localZ - nearestZ;

	float distSq = (diffX * diffX) + (diffZ * diffZ);

	if (distSq > cylinderRadius * cylinderRadius)return false;

	//押し戻しベクトル（MTD）計算

	if (outMTD)
	{
		float mtdLocalX = 0.0f;
		float mtdLocalZ = 0.0f;

		if (distSq > 1e-12f)
		{
			float dist = sqrtf(distSq);
			float penetrationRatio = (cylinderRadius - dist) / dist;

			mtdLocalX = diffX * penetrationRatio;
			mtdLocalZ = diffZ * penetrationRatio;
		}
		else
		{
			//円心が完全にボックス内 → 最小距離軸に押し出す
			float overlapX = obb.half.x - fabsf(localX);
			float overlapZ = obb.half.z - fabsf(localZ);

			if (overlapX < overlapZ)
			{
				mtdLocalX = (localX < 0 ? -overlapX : overlapX);
			}
			else
			{
				mtdLocalZ = (localZ < 0 ? -overlapZ : overlapZ);
			}

			//Y方向の押し戻し
			float mtdLocalY = (overlapTop < overlapBottom) ? overlapTop : -overlapBottom;

			//どの方向の侵入量が一番小さいかで採用軸を決める
			DirectX::XMFLOAT3 mtdLocal{ 0.0f,0.0f,0.0f };
			float horizMag = sqrtf((mtdLocalX * mtdLocalX) + (mtdLocalZ * mtdLocalZ));

			if (horizMag > 0.0f)
			{
				//Y方向の方が小さければ縦に押し返す
				if (fabsf(mtdLocalY) < horizMag)mtdLocal = DirectX::XMFLOAT3(0.0f, mtdLocalY, 0.0f);
				else mtdLocal = DirectX::XMFLOAT3(mtdLocalX, 0.0f, mtdLocalZ);
			}
			else
			{
				//水平方向が0 → 縦方向一択
				mtdLocal = DirectX::XMFLOAT3(0.0f, mtdLocalY, 0.0f);
			}

			//ローカル → ワールドへ戻す

			DirectX::XMFLOAT3 mtdWorld{
				mtdLocal.x * obb.axis[0].x +
				mtdLocal.y * obb.axis[1].x +
				mtdLocal.z * obb.axis[2].x,

				mtdLocal.x * obb.axis[0].y +
				mtdLocal.y * obb.axis[1].y +
				mtdLocal.z * obb.axis[2].y,

				mtdLocal.x * obb.axis[0].z +
				mtdLocal.y * obb.axis[1].z +
				mtdLocal.z * obb.axis[2].z
			};

			*outMTD = mtdWorld;
		}
	}

	return true;
}

bool Collision::IntersectCylinderVsAABB(
	const CylinderCollider* cylinder,
	const BoxCollider* box)
{
	return false;
}

void Collision::OBBtoAABB(
	const DirectX::XMFLOAT3& center,
	const DirectX::XMFLOAT3& half,
	const DirectX::XMFLOAT3 axis[3],
	DirectX::XMFLOAT3& outPos,
	DirectX::XMFLOAT3& outSize)
{
	using namespace DirectX;

	XMVECTOR C = XMLoadFloat3(&center);
	XMVECTOR A0 = XMLoadFloat3(&axis[0]);
	XMVECTOR A1 = XMLoadFloat3(&axis[1]);
	XMVECTOR A2 = XMLoadFloat3(&axis[2]);

	float hx = half.x;
	float hy = half.y;
	float hz = half.z;

	XMFLOAT3 c[8];
	const int s[8][3] =
	{
		{-1,-1,-1},{-1,+1,-1},{+1,+1,-1},{+1,-1,-1},
		{-1,-1,+1},{-1,+1,+1},{+1,+1,+1},{+1,-1,+1},
	};

	for (int i = 0; i < 8; i++)
	{
		XMVECTOR p =
			C +
			A0 * (hx * s[i][0]) +
			A1 * (hy * s[i][1]) +
			A2 * (hz * s[i][2]);
		XMStoreFloat3(&c[i], p);
	}

	XMFLOAT3 mn = c[0];
	XMFLOAT3 mx = c[0];

	for (int i = 1; i < 8; i++)
	{
		mn.x = (std::min)(mn.x, c[i].x);
		mn.y = (std::min)(mn.y, c[i].y);
		mn.z = (std::min)(mn.z, c[i].z);

		mx.x = (std::max)(mx.x, c[i].x);
		mx.y = (std::max)(mx.y, c[i].y);
		mx.z = (std::max)(mx.z, c[i].z);
	}

	outPos = {
		(mn.x + mx.x) * 0.5f,
		(mn.y + mx.y) * 0.5f,
		(mn.z + mx.z) * 0.5f
	};

	outSize = {
		mx.x - mn.x,
		mx.y - mn.y,
		mx.z - mn.z
	};

}

///重さ考慮付きMTD計算
void Collision::ApplyPushOutWithWeight(
	GameObject* objA,
	GameObject* objB,
	const DirectX::XMFLOAT3& normal,
	float penetration
)
{
	if (penetration <= 1e-6f)return;

	//重さ比率を計算
	//weightが小さいほどよく動く
	float weightA = objA->weight;
	float weightB = objB->weight;

	//どちらも固定ならスキップ
	if (weightA <= 0.0f && weightB <= 0.0f)return;

	float total_weight = weightA + weightB;
	if (total_weight <= 0.0f)return;

	//比率(相手が重いほど自分が多く動く)
	float move_rarioA = (weightB) / total_weight;
	float move_rarioB = (weightA) / total_weight;

	//移動量ベクトルを計算（Aはマイナス方向、Bはプラス方向）
	DirectX::XMVECTOR n = XMLoadFloat3(&normal);
	DirectX::XMVECTOR moveA = DirectX::XMVectorScale(n, -penetration * move_rarioA);
	DirectX::XMVECTOR moveB = DirectX::XMVectorScale(n, penetration * move_rarioB);

	DirectX::XMFLOAT3 mtdA, mtdB;
	DirectX::XMStoreFloat3(&mtdA, moveA);
	DirectX::XMStoreFloat3(&mtdB, moveB);

	objA->SetMTD(mtdA);
	objB->SetMTD(mtdB);
}

bool Collision::IntersectCylinder_Vs_Cylinder(
	CylinderCollider* cylinderA,
	CylinderCollider* cylinderB,
	DirectX::XMFLOAT3& outNormal,
	float& outPenetrarion)
{
	//Y軸方向の重なりチェック 
	float minA = cylinderA->center.y;
	float maxA = cylinderA->center.y + cylinderA->height;
	float minB = cylinderB->center.y;
	float maxB = cylinderB->center.y + cylinderB->height;

	if (maxA < minB || maxB < minA)
	{
		//高さが重なっていない
		outNormal = { 0,0,0 };
		outPenetrarion = 0.0f;
		return false;
	}

	//水平方向距離
	float dx = cylinderB->center.x - cylinderA->center.x;
	float dz = cylinderB->center.z - cylinderA->center.z;
	float distSq = (dx * dx) + (dz * dz);
	float hitDist = cylinderA->radius + cylinderB->radius;

	//距離が十分離れていれば衝突していない
	if (distSq >= hitDist * hitDist)
	{
		outNormal = { 0,0,0 };
		outPenetrarion = 0.0f;
		return false;
	}

	float dist = std::sqrtf(distSq);
	if (dist <= 1e-6f)
	{
		outNormal = { 0,0,0 };
		outPenetrarion = 0.0f;
		return false;
	}

	outNormal = { dx / dist,0.0f,dz / dist };
	outPenetrarion = hitDist - dist;
	return true;
}

bool Collision::IntersectCylinder_Vs_OBB(
	CylinderCollider* cylinder,
	OBB* obb,
	DirectX::XMFLOAT3& outNormal,
	float& outPenetration)
{
	//最小貫通深度と対応する法線ベクトルを記録する変数
	float min_penetration = FLT_MAX;
	DirectX::XMVECTOR best_normal = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

	//円柱の中心座標
	DirectX::XMVECTOR c_world = DirectX::XMLoadFloat3(&cylinder->center);

	//円柱のハーフハイト
	float half_heightA = cylinder->height * 0.5f;

	//OBBの中心座標と軸ベクトル
	DirectX::XMVECTOR obb_center = DirectX::XMLoadFloat3(&obb->center);
	DirectX::XMVECTOR obb_axis[3] = {
		DirectX::XMLoadFloat3(&obb->axis[0]),
		DirectX::XMLoadFloat3(&obb->axis[1]),
		DirectX::XMLoadFloat3(&obb->axis[2])
	};

	//OBBの3つの軸(axis0,axis1,axis2)による分離チェック
	for (int i = 0; i < 3; i++)
	{
		DirectX::XMVECTOR axis = obb_axis[i];

		//OBBの投影
		float minB, maxB;
		ProjectOBB(obb, axis, minB, maxB);

		//円柱の投影
		float cylinder_projection = DirectX::XMVectorGetX(DirectX::XMVector3Dot(c_world, axis));

		//円柱の投影幅を計算
		DirectX::XMVECTOR axis_xz = DirectX::XMVectorSetY(axis, 0.0f);
		float radius_projection = cylinder->radius * DirectX::XMVectorGetX(DirectX::XMVector3Length(axis_xz));

		//高さによる広がり
		float height_projection = half_heightA * DirectX::XMVectorGetX(DirectX::XMVectorAbs(DirectX::XMVector3Dot(DirectX::XMVectorSet(0, 1, 0, 0), axis)));

		float radiusA = radius_projection + height_projection;
		float minA = cylinder_projection - radiusA;
		float maxA = cylinder_projection + radiusA;

		float overlap, center_diff;
		if (!Overlap(minA, maxA, minB, maxB, overlap, center_diff))
		{
			//分離軸が見つかった
			outNormal = { 0.0f,0.0f,0.0f };
			outPenetration = 0.0f;
			return false;
		}

		//最長貫通深度を更新
		if (overlap < min_penetration)
		{
			min_penetration = overlap;
			//法線ベクトルの方向を、OBBの中心から円柱の中心に向かう方向(center_diff)で決定
			if (center_diff < 0.0f)
			{
				best_normal = DirectX::XMVectorNegate(axis);
			}
			else
			{
				best_normal = axis;
			}
		}
	}

	//円柱の高さ軸(Y軸)による分離チェック
	DirectX::XMVECTOR axis_cylinder = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	//円柱の投影(ワールドY軸方向)
	float minA_cylinder = cylinder->center.y - half_heightA;
	float maxA_cylinder = cylinder->center.y + half_heightA;

	//OBBの投影(ワールドY軸方向)
	float minB_cross, maxB_cross;
	ProjectOBB(obb, axis_cylinder, minB_cross, maxB_cross);

	float overlap_cylinder, center_diff_cylinder;
	if (!Overlap(minA_cylinder, maxA_cylinder, minB_cross, maxB_cross, overlap_cylinder, center_diff_cylinder))
	{
		//分離軸が見つかった
		outNormal = { 0.0f,0.0f,0.0f };
		outPenetration = 0.0f;
		return false;
	}

	//最小貫通深度を更新
	if (overlap_cylinder < min_penetration)
	{
		min_penetration = overlap_cylinder;
		best_normal = (center_diff_cylinder > 0) ? axis_cylinder : DirectX::XMVectorNegate(axis_cylinder);
	}

	//OBBの3辺と円柱の軸の外積軸による分離チェック

	DirectX::XMVECTOR axis_cylinder_world = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	for (int i = 0; i < 3; i++)
	{
		DirectX::XMVECTOR obb_edge_axis = obb_axis[i];
		DirectX::XMVECTOR cross_axis = DirectX::XMVector3Cross(obb_edge_axis, axis_cylinder_world);

		// 軸がゼロベクトルに近い場合はスキップ (OBBのY軸とワールドY軸が平行な場合など)
		if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(cross_axis)) < 1e-6f) continue;

		// 軸を正規化
		cross_axis = DirectX::XMVector3Normalize(cross_axis);

		// OBBの投影
		float minB_cross, maxB_cross;
		ProjectOBB(obb, cross_axis, minB_cross, maxB_cross);

		// 円柱の投影 (高さと半径を考慮)
		float cylinder_projection = DirectX::XMVectorGetX(DirectX::XMVector3Dot(c_world, cross_axis));

		DirectX::XMVECTOR axis_xz = DirectX::XMVectorSetY(cross_axis, 0.0f);
		float radius_projection = cylinder->radius * DirectX::XMVectorGetX(DirectX::XMVector3Length(axis_xz));

		float height_projection = half_heightA * DirectX::XMVectorGetX(DirectX::XMVectorAbs(DirectX::XMVector3Dot(axis_cylinder_world, cross_axis)));

		float radiusA = radius_projection + height_projection;
		float minA = cylinder_projection - radiusA;
		float maxA = cylinder_projection + radiusA;

		float overlap_cylinder, center_diff_cylinder;
		if (!Overlap(minA, maxA, minB_cross, maxB_cross, overlap_cylinder, center_diff_cylinder))
		{
			//分離軸が見つかった
			return false;
		}

		//最小貫通深度を更新
		if (overlap_cylinder < min_penetration)
		{
			min_penetration = overlap_cylinder;
			best_normal = (center_diff_cylinder > 0) ? cross_axis : DirectX::XMVectorNegate(cross_axis);
		}
	}

	//OBBに最も近い円柱の中心点への軸による分離チェック

	// OBBの中心から円柱の中心へのベクトル
	DirectX::XMVECTOR obb_to_cylinder = DirectX::XMVectorSubtract(c_world, obb_center);

	// OBBのローカル空間での円柱の中心の座標を求める
	float localX = DirectX::XMVectorGetX(DirectX::XMVector3Dot(obb_to_cylinder, obb_axis[0]));
	float localY = DirectX::XMVectorGetX(DirectX::XMVector3Dot(obb_to_cylinder, obb_axis[1]));
	float localZ = DirectX::XMVectorGetX(DirectX::XMVector3Dot(obb_to_cylinder, obb_axis[2]));

	// OBBの半分のサイズ (ハーフエクステント)
	float obb_half_extent[3] = { obb->half.x, obb->half.y, obb->half.z };

	// OBBに最も近い円柱の中心点をローカル座標で見つける (クランプ)
	float clampedX = (std::max)(-obb_half_extent[0], (std::min)(localX, obb_half_extent[0]));
	float clampedY = (std::max)(-obb_half_extent[1], (std::min)(localY, obb_half_extent[1]));
	float clampedZ = (std::max)(-obb_half_extent[2], (std::min)(localZ, obb_half_extent[2]));

	// OBBのローカル空間での、クランプされた点から円柱の中心へのベクトル
	DirectX::XMVECTOR diff_local = DirectX::XMVectorSet(localX - clampedX, localY - clampedY, localZ - clampedZ, 0.0f);

	// diff_local をワールド座標に戻す（最短距離軸の候補）
	DirectX::XMVECTOR axis_candidate = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	axis_candidate = DirectX::XMVectorAdd(axis_candidate, DirectX::XMVectorScale(obb_axis[0], DirectX::XMVectorGetX(diff_local)));
	axis_candidate = DirectX::XMVectorAdd(axis_candidate, DirectX::XMVectorScale(obb_axis[1], DirectX::XMVectorGetY(diff_local)));
	axis_candidate = DirectX::XMVectorAdd(axis_candidate, DirectX::XMVectorScale(obb_axis[2], DirectX::XMVectorGetZ(diff_local)));

	// 軸を正規化
	DirectX::XMVECTOR shortest_distance_axis = DirectX::XMVector3Normalize(axis_candidate);

	// 軸がゼロベクトルに近い場合はスキップ
	if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(shortest_distance_axis)) > 1e-6f)
	{
		// OBBの投影
		float minB_shortest, maxB_shortest;
		ProjectOBB(obb, shortest_distance_axis, minB_shortest, maxB_shortest);

		// 円柱の投影 (高さと半径を考慮)
		float cylinder_projection = DirectX::XMVectorGetX(DirectX::XMVector3Dot(c_world, shortest_distance_axis));

		DirectX::XMVECTOR axis_xz = DirectX::XMVectorSetY(shortest_distance_axis, 0.0f);
		float radius_projection = cylinder->radius * DirectX::XMVectorGetX(DirectX::XMVector3Length(axis_xz));

		DirectX::XMVECTOR axis_cylinder_world = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		float height_projection = half_heightA * DirectX::XMVectorGetX(DirectX::XMVectorAbs(DirectX::XMVector3Dot(axis_cylinder_world, shortest_distance_axis)));

		float radiusA = radius_projection + height_projection;
		float minA = cylinder_projection - radiusA;
		float maxA = cylinder_projection + radiusA;

		float overlap_cylinder, center_diff_cylinder;
		if (!Overlap(minA, maxA, minB_shortest, maxB_shortest, overlap_cylinder, center_diff_cylinder))
		{
			//分離軸が見つかった
			return false;
		}

		//最小貫通深度を更新
		if (overlap_cylinder < min_penetration)
		{
			min_penetration = overlap_cylinder;
			best_normal = (center_diff_cylinder > 0) ? shortest_distance_axis : DirectX::XMVectorNegate(shortest_distance_axis);
		}
	}

	//すべての軸で分離が見つからなかった場合、衝突している
	DirectX::XMStoreFloat3(&outNormal, best_normal);
	outPenetration = min_penetration;

	return true;
}


bool Collision::IntersectCylinder_Vs_Box(
	CylinderCollider* cylinder,
	BoxCollider* box,
	DirectX::XMFLOAT3& outNormal,
	float& outPenetration)
{
	using namespace DirectX;

	const XMFLOAT3& cPos = cylinder->center;
	float radius = cylinder->radius;
	float height = cylinder->height;

	// Cylinder の底と上のY座標
	float cylMinY = cPos.y;
	float cylMaxY = cPos.y + height;

	// Y方向の重なりチェック
	if (cylMaxY < box->box_min.y || cylMinY > box->box_max.y)
	{
		outNormal = { 0,0,0 };
		outPenetration = 0.0f;
		return false;
	}

	// Cylinder 中心のXZから最近接点を求める
	float closestX = std::clamp(cPos.x, box->box_min.x, box->box_max.x);
	float closestZ = std::clamp(cPos.z, box->box_min.z, box->box_max.z);

	float dx = cPos.x - closestX;
	float dz = cPos.z - closestZ;
	float distSq = dx * dx + dz * dz;

	// 接触チェック
	if (distSq >= radius * radius)
	{
		outNormal = { 0,0,0 };
		outPenetration = 0.0f;
		return false;
	}

	float dist = std::sqrtf(distSq);
	if (dist < 1e-6f)
	{
		// 中心が完全に中にあるケース
		outNormal = { 0,1,0 }; // 適当（上方向）に出す
		outPenetration = radius;
		return true;
	}

	// 法線と貫入量計算
	outNormal = { dx / dist, 0.0f, dz / dist };
	outPenetration = radius - dist;

	return true;
}

//ベクトルを正規化する
DirectX::XMVECTOR Collision::Normalize(const DirectX::XMFLOAT3& v)
{
	return DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&v));
}

//2つの凸多面体の軸への投影が重なっているかチェックする
bool Collision::Overlap(float min1, float max1, float min2, float max2, float& overlap, float& centerDiff)
{
	//投影された範囲の最大値と最小値を決定
	float min_max = (std::min)(max1, max2);
	float max_min = (std::max)(min1, min2);

	//重なりの計算
	overlap = min_max - max_min;

	if (overlap > 0.0f)
	{
		//中心間の差分を計算
		float c1 = (min1 + max1) * 0.5f;
		float c2 = (min2 + max2) * 0.5f;
		centerDiff = c1 - c2;
		return true;
	}

	return false;
}

//OBBを任意の軸(axis)に投影したときの最小値と最大値を計算する
void Collision::ProjectOBB(const OBB* obb, DirectX::XMVECTOR axis, float& min, float& max)
{
	DirectX::XMVECTOR center = DirectX::XMLoadFloat3(&obb->center);
	DirectX::XMVECTOR hx = DirectX::XMLoadFloat3(&obb->axis[0]) * obb->half.x;
	DirectX::XMVECTOR hy = DirectX::XMLoadFloat3(&obb->axis[1]) * obb->half.y;
	DirectX::XMVECTOR hz = DirectX::XMLoadFloat3(&obb->axis[2]) * obb->half.z;

	//投影の幅(中心からの最大距離)を計算
	float radius =
		DirectX::XMVectorGetX(DirectX::XMVectorAbs(DirectX::XMVector3Dot(hx, axis))) +
		DirectX::XMVectorGetX(DirectX::XMVectorAbs(DirectX::XMVector3Dot(hy, axis))) +
		DirectX::XMVectorGetX(DirectX::XMVectorAbs(DirectX::XMVector3Dot(hz, axis)));

	//中心の投影値を計算
	float center_projection = DirectX::XMVectorGetX(DirectX::XMVector3Dot(center, axis));

	min = center_projection - radius;
	max = center_projection + radius;
}

