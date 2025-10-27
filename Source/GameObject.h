#pragma once
#include <vector>
#include <string>
#include <memory>
#include <DirectXMath.h>

#include "System/Model.h"

class GameObject
{
public:
	Model* model = nullptr;//実際のモデルデータ

	std::string name;//オブジェクトの名前
	DirectX::XMFLOAT3 position;//位置
	DirectX::XMFLOAT3 angle;//回転角度
	DirectX::XMFLOAT3 scale;//拡大・縮小
	DirectX::XMFLOAT4 color;
	DirectX::XMFLOAT4X4 transform;//ワールド変換行列

	enum class Type { Object, Player, Enemy } type;
	int mesh_index;//framework側の配列インデックスbool
	std::string model_path; // モデルのファイルパス

	bool dirty = false;

	GameObject();//デフォルトコンストラクタ

	//void UpdateTransform();

};

