#include "GameObject.h"

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

//void GameObject::UpdateTransform()
//{
//	using namespace DirectX;
//	XMMATRIX S = XMMatrixScaling(scale.x, scale.y, scale.z);
//	XMMATRIX R = XMMatrixRotationRollPitchYaw(
//		XMConvertToRadians(angle.x),
//		XMConvertToRadians(angle.y),
//		XMConvertToRadians(angle.z)
//	);
//	XMMATRIX T = XMMatrixTranslation(position.x, position.y, position.z);
//
//	XMStoreFloat4x4(&world, S * R * T);
//}
