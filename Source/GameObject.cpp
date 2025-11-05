#include "GameObject.h"
#include "CollisionManager.h"

void GameObject::UpdateTransform()
{
	////スケール行列を作成
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);

	//回転行列を作成
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);

	// 位置行列を作成
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y, position.z);

	// ３つの行列を組み合わせ、ワールド行列を作成
	DirectX::XMMATRIX W = S * R * T;
	// 計算したワールド行列を取り出す
	DirectX::XMStoreFloat4x4(&transform, W);
}

GameObject::GameObject() :name("Empty")
{
	position = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	angle = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	scale = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
	color = { 1.0f,1.0f,1.0f,1.0f };
	transform = DirectX::XMFLOAT4X4
	{
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1
	};
}

GameObject::~GameObject()
{
	CollisionManager::Instance().Remove(this);
	std::cout << "Destroyed" << std::endl;
}
