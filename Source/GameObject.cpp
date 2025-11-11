#include "GameObject.h"
#include "CollisionManager.h"
#include <unordered_set>

int GameObject::nextID = 0;

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

OBB GameObject::GetOBB() const
{
	OBB box;
	if (!model || !collider) return box;

	box.type = ColliderType::OBB;
	box.owner = const_cast<GameObject*>(this);

	bool ok = model->GetModelOBB(
		model,
		position,
		angle,
		scale,
		box.center,
		box.half,
		box.axis
	);

	return box;
}

GameObject::GameObject() :name("Empty")
{
	id = nextID;
	nextID++;
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

void FixDuplicateIDs(std::vector<std::shared_ptr<GameObject>>& objects)
{
	std::unordered_set<int> used;
	int maxID = -1;

	//最大のIDを検索
	for (auto& obj : objects)
	{
		if (obj->id > maxID)
			maxID = obj->id;
	}

	for (auto& obj : objects)
	{
		//IDが重複していたら新しいIDを割り当てる
		if (used.find(obj->id) != used.end())
		{
			obj->id = maxID++;
		}

		used.insert(obj->id);

		if (obj->id > maxID)
		{
			maxID = obj->id;
		}
	}

	//nextIDをmaxID+1に更新する
	GameObject::nextID = maxID + 1;
}
