#pragma once

#include "GameObject.h"

class GameObject;

enum class ColliderType { Sphere, Box, Cylinder, OBB };

struct Collider
{
	ColliderType type;
	GameObject* owner;//属するオブジェクト
	DirectX::XMFLOAT3 mtd;
	virtual ~Collider() = default;
};

struct SphereCollider :public Collider
{
	DirectX::XMFLOAT3 center;
	float radius;
};

struct BoxCollider :public Collider
{
	DirectX::XMFLOAT3 box_min;
	DirectX::XMFLOAT3 box_max;
};

struct CylinderCollider :public Collider
{
	DirectX::XMFLOAT3 center;
	float radius;
	float height;
};

struct OBB :public Collider
{
	DirectX::XMFLOAT3 center; // 中心座標
	DirectX::XMFLOAT3 half;   // ハーフサイズ（X,Y,Z）
	DirectX::XMFLOAT3 axis[3];//X,Y,Z方向ベクトル
	//float yaw;                // Y軸回転（ラジアン）
};

static bool IsPointInsideOBB(const OBB* obb, DirectX::XMFLOAT3& p)
{
	using namespace DirectX;

	// 点と OBB 中心の差
	XMVECTOR d = XMLoadFloat3(&p) - XMLoadFloat3(&obb->center);

	for (int i = 0; i < 3; ++i)
	{
		XMVECTOR axisVec = XMLoadFloat3(&obb->axis[i]);
		float dist = XMVectorGetX(XMVector3Dot(d, axisVec));

		float halfSize = 0.0f;
		switch (i)
		{
		case 0: halfSize = obb->half.x; break;
		case 1: halfSize = obb->half.y; break;
		case 2: halfSize = obb->half.z; break;
		}

		if (fabs(dist) > halfSize)
			return false;
	}

	return true;
}
