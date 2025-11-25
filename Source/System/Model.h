#pragma once

#include <memory>
#include <vector>
#include <DirectXMath.h>
#include "ModelResource.h"

// モデル
class Model
{
public:
	Model(const char* filename);
	//既存のリソースから作成するコンストラクタ
	Model(const std::shared_ptr<ModelResource>& resource, const char* filename);
	~Model() {}

	std::string name;
	std::string path;

	struct Node
	{
		const char* name;
		Node* parent;
		DirectX::XMFLOAT3	scale;
		DirectX::XMFLOAT4	rotate;
		DirectX::XMFLOAT3	translate;
		DirectX::XMFLOAT4X4	localTransform;
		DirectX::XMFLOAT4X4	globalTransform;

		std::vector<Node*>	children;
	};

	// 行列計算
	void UpdateTransform();

	// ノードリスト取得
	const std::vector<Node>& GetNodes() const { return nodes; }
	std::vector<Node>& GetNodes() { return nodes; }

	// リソース取得
	const ModelResource* GetResource() const { return resource.get(); }

	//バウンディングボックス
	bool getModelAABB(const Model* model, DirectX::XMFLOAT3& outMin, DirectX::XMFLOAT3& outMax);

	//OBBバウンディングボックス
	bool GetModelOBB(
		const Model* model,
		const DirectX::XMFLOAT3& position,
		const DirectX::XMFLOAT3& rotation,   // RPY（XYZ 回転）
		const DirectX::XMFLOAT3& scale,
		DirectX::XMFLOAT3& outCenter,
		DirectX::XMFLOAT3& outHalfSize,
		DirectX::XMFLOAT3 outAxis[3]);         // 3本の軸
private:
	//ノード初期化処理を共通化
	void InitializeNodes();

	std::shared_ptr<ModelResource>	resource;
	std::vector<Node>				nodes;
};