#include "Graphics.h"
#include "Model.h"
#include <filesystem>

// コンストラクタ
Model::Model(const char* filename)
{
	if (filename == nullptr)
	{
		OutputDebugStringA("[Model] filename is nullptr!\n");
		name = "InvalidModel";
		return;
	}

	OutputDebugStringA(("[Model] Loading: " + std::string(filename) + "\n").c_str());

	// ファイル名から name を初期化
	name = std::filesystem::path(filename).stem().string();

	// リソース読み込み
	resource = std::make_shared<ModelResource>();
	resource->Load(Graphics::Instance().GetDevice(), filename);

	// ノード
	const std::vector<ModelResource::Node>& resNodes = resource->GetNodes();

	nodes.resize(resNodes.size());
	for (size_t nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex)
	{
		auto&& src = resNodes.at(nodeIndex);
		auto&& dst = nodes.at(nodeIndex);

		dst.name = src.name.c_str();
		dst.parent = src.parentIndex >= 0 ? &nodes.at(src.parentIndex) : nullptr;
		dst.scale = src.scale;
		dst.rotate = src.rotate;
		dst.translate = src.translate;

		if (dst.parent != nullptr)
		{
			dst.parent->children.emplace_back(&dst);
		}
	}

	// 行列計算
	UpdateTransform();
}

// 変換行列計算
void Model::UpdateTransform()
{
	for (Node& node : nodes)
	{
		// ローカル行列算出
		DirectX::XMMATRIX S = DirectX::XMMatrixScaling(node.scale.x, node.scale.y, node.scale.z);
		DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&node.rotate));
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(node.translate.x, node.translate.y, node.translate.z);
		DirectX::XMMATRIX LocalTransform = S * R * T;
		DirectX::XMStoreFloat4x4(&node.localTransform, LocalTransform);

		// グローバル行列算出
		if (node.parent != nullptr)
		{
			DirectX::XMMATRIX ParentGlobalTransform = DirectX::XMLoadFloat4x4(&node.parent->globalTransform);
			DirectX::XMMATRIX GlobalTransform = DirectX::XMMatrixMultiply(LocalTransform, ParentGlobalTransform);
			DirectX::XMStoreFloat4x4(&node.globalTransform, GlobalTransform);
		}
		else
		{
			DirectX::XMStoreFloat4x4(&node.globalTransform, LocalTransform);
		}
	}
}

bool Model::getModelAABB(
	const Model* model,
	DirectX::XMFLOAT3& outMin,
	DirectX::XMFLOAT3& outMax)
{
	const ModelResource* res = model->GetResource();
	if (!res) return false;

	bool first = true;

	for (const auto& mesh : res->GetMeshes())
	{
		if (first)
		{
			outMin = mesh.boundsMin;
			outMax = mesh.boundsMax;
			first = false;
		}
		else
		{
			outMin.x = (std::min)(outMin.x, mesh.boundsMin.x);
			outMin.y = (std::min)(outMin.y, mesh.boundsMin.y);
			outMin.z = (std::min)(outMin.z, mesh.boundsMin.z);

			outMax.x = (std::max)(outMax.x, mesh.boundsMax.x);
			outMax.y = (std::max)(outMax.y, mesh.boundsMax.y);
			outMax.z = (std::max)(outMax.z, mesh.boundsMax.z);
		}
	}

	return true;
}

bool Model::GetModelOBB(
	const Model* model,
	const DirectX::XMFLOAT3& position,
	const DirectX::XMFLOAT3& rotation,
	const DirectX::XMFLOAT3& scale,
	DirectX::XMFLOAT3& outCenter,
	DirectX::XMFLOAT3& outHalfSize,
	DirectX::XMFLOAT3 outAxis[3])
{
	if (!model) return false;

	using namespace DirectX;

	// -------------------------------
	// 1. ローカル AABB を取る (省略)
	// -------------------------------
	XMFLOAT3 localMin, localMax;
	if (!getModelAABB(model, localMin, localMax))
		return false;

	// OBB のローカル中心とハーフサイズ
	XMFLOAT3 localCenter = {
		(localMin.x + localMax.x) * 0.5f,
		(localMin.y + localMax.y) * 0.5f,
		(localMin.z + localMax.z) * 0.5f
	};

	XMFLOAT3 localHalf = {
		(localMax.x - localMin.x) * 0.5f,
		(localMax.y - localMin.y) * 0.5f,
		(localMax.z - localMin.z) * 0.5f
	};

	// -------------------------------
	// 2. ワールド行列を構築
	// -------------------------------
	XMMATRIX mat_scale = XMMatrixScaling(scale.x, scale.y, scale.z);
	XMMATRIX mat_rot = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
	XMMATRIX mat_trans = XMMatrixTranslation(position.x, position.y, position.z);

	XMMATRIX mat_world = mat_scale * mat_rot * mat_trans;


	// -------------------------------
	// 3. 中心と軸をワールドへ
	// -------------------------------
	// (3-1) 中心座標をワールド空間へ変換
	XMVECTOR c = XMLoadFloat3(&localCenter);
	// Center はスケール、回転、移動すべて適用する
	c = XMVector3Transform(c, mat_world);
	XMStoreFloat3(&outCenter, c);

	// (3-2) OBBの軸をワールド空間へ変換 (スケールは含まない)
	// 回転行列mat_rotのみを使用して軸を変換
	XMVECTOR axisX = XMVector3TransformNormal(XMVectorSet(1, 0, 0, 0), mat_rot);
	XMVECTOR axisY = XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), mat_rot);
	XMVECTOR axisZ = XMVector3TransformNormal(XMVectorSet(0, 0, 1, 0), mat_rot);

	// 回転行列を使用しているため、軸は既に正規化されているが、念のため再度正規化して保存
	XMStoreFloat3(&outAxis[0], XMVector3Normalize(axisX));
	XMStoreFloat3(&outAxis[1], XMVector3Normalize(axisY));
	XMStoreFloat3(&outAxis[2], XMVector3Normalize(axisZ));

	// -------------------------------
	// 4. ハーフサイズ（拡大済み）
	// -------------------------------
	// ハーフサイズはローカルサイズにワールドのスケールを乗じるだけで良い
	outHalfSize = {
		localHalf.x * scale.x,
		localHalf.y * scale.y,
		localHalf.z * scale.z
	};

	return true;
}
