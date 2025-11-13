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
