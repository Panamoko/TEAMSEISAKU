#pragma once

#include <vector>
#include <string>
#include <memory>
#include <DirectXMath.h>

#include "System/Model.h"
#include <System/ModelRenderer.h>
#include <System/ShapeRenderer.h>

#include "Collider.h"

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
public:
	Model* model = nullptr;//実際のモデルデータ
	Collider* collider = nullptr;

	std::string name;//オブジェクトの名前
	std::string class_name;//実クラス名
	DirectX::XMFLOAT3 position;//位置
	DirectX::XMFLOAT3 angle;//回転角度
	DirectX::XMFLOAT3 scale;//拡大・縮小
	DirectX::XMFLOAT4 color;
	DirectX::XMFLOAT4X4 transform;//ワールド変換行列

	enum class Type { Object, Gimmic, Player, Enemy } type;
	int mesh_index;//framework側の配列インデックスbool
	std::string model_path; // モデルのファイルパス

	bool dirty = false;

	GameObject();//デフォルトコンストラクタ

};

