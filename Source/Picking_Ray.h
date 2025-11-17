#pragma once

#include "Camera.h"
#include "CameraController.h"

#include <DirectXMath.h>

class Picking_Ray
{
public:
	Picking_Ray();
	void Update();

private:

	DirectX::XMFLOAT3   camera_position;	//カメラのワールド座標
	DirectX::XMFLOAT3   up;					//上方向ベクトル
	DirectX::XMFLOAT3   right;				//横方向ベクトル
	DirectX::XMFLOAT3   forward;			//前方向ベクトル
	DirectX::XMFLOAT4X4 view_matrix;			//ビュー行列
	DirectX::XMFLOAT4X4 projection_matrix;	//プロジェクション行列
	DirectX::XMFLOAT2   mouse_position;		//マウス座標
	float screen_width;						//スクリーン座標の縦幅
	float screen_height;					//スクリーン座標の横幅
	DirectX::XMMATRIX inverse_view_matrix;	//View行列の逆行列
};

