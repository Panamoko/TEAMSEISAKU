#include "Picking_Ray.h"
#include <Windows.h>


Picking_Ray::Picking_Ray()
{
	screen_width = 1280.0f;
	screen_height = 720.0f;
	mouse_position = { 0.0f,0.0f };
}

void Picking_Ray::Update()
{
	//Cameraクラスから値を取得
	camera_position = Camera::Instance().GetEye();
	up = Camera::Instance().GetUp();
	right = Camera::Instance().GetRight();
	forward = Camera::Instance().GetFront();

	view_matrix = Camera::Instance().GetView();
	projection_matrix = Camera::Instance().GetProjection();

	//マウス座標の取得
	POINT mouse;

	GetCursorPos(&mouse);
	ScreenToClient(GetActiveWindow(), &mouse);

	mouse_position.x = static_cast<float>(mouse.x);
	mouse_position.y = static_cast<float>(mouse.y);

	//NDCへ変換
	float ndc_x = (mouse_position.x / screen_width) * 2.0f - 1.0f;
	float ndc_y = ((screen_height - mouse_position.y) / screen_height) * 2.0f - 1.0f;

	//射影行列の逆行列でクリップ空間 → カメラ空間へ
	DirectX::XMMATRIX proj = DirectX::XMLoadFloat4x4(&projection_matrix);
	DirectX::XMMATRIX inv_proj = DirectX::XMMatrixInverse(nullptr, proj);

	DirectX::XMVECTOR ray_clip = DirectX::XMVectorSet(ndc_x, ndc_y, 1.0f, 1.0f);
	DirectX::XMVECTOR ray_camera = DirectX::XMVector3TransformCoord(ray_clip, inv_proj);

	//カメラ空間のレイは前方向 z=1
	ray_camera = DirectX::XMVectorSetW(ray_camera, 0.0f);

	//ビュー行列の逆行列でレイをワールド空間へ
	inverse_view_matrix = DirectX::XMMatrixInverse(nullptr, DirectX::XMLoadFloat4x4(&view_matrix));

	DirectX::XMVECTOR ray_world = DirectX::XMVector3TransformNormal(ray_camera, inverse_view_matrix);
	ray_world = DirectX::XMVector3Normalize(ray_world);

	//計算結果をメンバに反映
	forward = {};
	DirectX::XMStoreFloat3(&forward, ray_world);
}
