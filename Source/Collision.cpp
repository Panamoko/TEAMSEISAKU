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
	const XMFLOAT3& P, float r, const OBB& B, XMFLOAT3* outMTD)
{
	// ワールド→OBBローカル（中心原点・yaw=0）
	XMFLOAT3 v{ P.x - B.center.x, P.y - B.center.y, P.z - B.center.z };
	XMFLOAT3 p = rotate_to_local_yaw(v, B.yaw);

	// OBB内の最近接点
	XMFLOAT3 q;
	q.x = clampf(p.x, -B.half.x, B.half.x);
	q.y = clampf(p.y, -B.half.y, B.half.y);
	q.z = clampf(p.z, -B.half.z, B.half.z);

	XMFLOAT3 d{ p.x - q.x, p.y - q.y, p.z - q.z };
	float d2 = d.x * d.x + d.y * d.y + d.z * d.z;

	if (d2 > r * r) return false; // 離れている

	if (outMTD) {
		// 外側衝突：球中心が外→最近接点までの方向に押し出し
		if (d2 > 1e-12f) {
			float dlen = sqrtf(d2);
			float t = (r - dlen) / dlen;
			XMFLOAT3 mtd_local{ d.x * t, d.y * t, d.z * t };
			XMFLOAT3 mtd_world = rotate_to_world_yaw(mtd_local, B.yaw);
			outMTD->x = mtd_world.x; outMTD->y = mtd_world.y; outMTD->z = mtd_world.z;
		}
		else {
			// 内側衝突：最小オーバーラップ軸へ押し出し
			float ox = B.half.x - fabsf(p.x);
			float oy = B.half.y - fabsf(p.y);
			float oz = B.half.z - fabsf(p.z);
			XMFLOAT3 mtd_local{};
			if (ox < oy && ox < oz) mtd_local = XMFLOAT3((p.x < 0 ? -ox : ox), 0, 0);
			else if (oy < oz)       mtd_local = XMFLOAT3(0, (p.y < 0 ? -oy : oy), 0);
			else                    mtd_local = XMFLOAT3(0, 0, (p.z < 0 ? -oz : oz));
			XMFLOAT3 mtd_world = rotate_to_world_yaw(mtd_local, B.yaw);
			outMTD->x = mtd_world.x; outMTD->y = mtd_world.y; outMTD->z = mtd_world.z;
		}
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
