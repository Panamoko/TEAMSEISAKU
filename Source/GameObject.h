#pragma once

#include <vector>
#include <string>
#include <memory>
#include <DirectXMath.h>

#include "System/Model.h"
#include <System/ModelRenderer.h>
#include <System/ShapeRenderer.h>
#include <iostream>

#include "Collider.h"
#include "CollisionManager.h"
#include "System/AudioResource.h"
#include "System/Audio.h"


struct Collider;
struct OBB;
class CollisionManager;

class GameObject
{
public:
	virtual void Update(float elapsedTime) {};
	virtual void Render(const RenderContext& rc, ModelRenderer* renderer)
	{
		if (model)
			renderer->Render(rc, transform, model, ShaderId::Lambert, color);
	}

	virtual void UpdateTransform();
	virtual void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) {};
	virtual bool OnImGui() { return false; };
	virtual void OnCollision(GameObject* object);
	virtual void CopyUniqueMembers(const GameObject* source) {};

	bool IsActive() const { return is_active; }
	void SetActive(bool active) { is_active = active; }

	int GetID() { return id; }

	OBB GetOBB() const;

	DirectX::XMFLOAT3 SetMTD(DirectX::XMFLOAT3 out_mtd) { return mtd = out_mtd; }

public:
	Model* model = nullptr;//実際のモデルデータ
	std::unique_ptr<Collider> collider;
	std::string name;//オブジェクトの名前
	std::string class_name;//実クラス名
	DirectX::XMFLOAT3 position;//位置
	DirectX::XMFLOAT3 angle;//回転角度
	DirectX::XMFLOAT3 scale;//拡大・縮小
	DirectX::XMFLOAT4 color;//色
	DirectX::XMFLOAT4X4 transform;//ワールド変換行列
	int mesh_index;//framework側の配列インデックスbool
	std::string model_path; // モデルのファイルパス
	int id = 0;//個別のID
	static int nextID;//次のIDを採番するための静的変数
	DirectX::XMFLOAT3 mtd;
	std::shared_ptr<GameObject> objects;

	AudioSource* hitSE[2] = { nullptr,nullptr };


	float weight;


	bool dirty = false;
	bool is_active = true;

	enum class Type {
		Object,
		Gimmic,
		Player,
		PlayerAttack,
		Enemy,
		EnemyAttack,
		Stage } type = Type::Object;

public:
	void SetType(Type t) { type = t; }

	GameObject();//デフォルトコンストラクタ
	~GameObject();
};

void FixDuplicateIDs(std::vector<std::shared_ptr<GameObject>>& objects);

