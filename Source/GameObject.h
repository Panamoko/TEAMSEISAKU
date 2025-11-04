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

class Collider;

class GameObject
{
public:
	virtual void Update(float elapsedTime) {};
	virtual void Render(const RenderContext& rc, ModelRenderer* renderer)
	{
		if (model)
			renderer->Render(rc, transform, model, ShaderId::Lambert);
	}

	virtual void UpdateTransform();
	virtual void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) {};
	virtual void OnImGui() {};
	virtual void OnCollision(GameObject*object){}

	bool IsActive() const { return is_active; }
	void SetActive(bool active) { is_active = active; }

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

	bool dirty = false;
	bool is_active = true;

	enum class Type { Object, Gimmic, Player, Enemy, Stage } type = Type::Object;
public:
	void SetType(Type t) { type = t; }

	GameObject();//デフォルトコンストラクタ
	~GameObject()
	{
		std::cout << "Destroyed" << std::endl;
	}

};

