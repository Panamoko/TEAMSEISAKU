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

