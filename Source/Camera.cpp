#include "Camera.h"

// 指定方向を向く
void Camera::SetLookAt(const DirectX::XMFLOAT3& eye, const DirectX::XMFLOAT3& focus, const DirectX::XMFLOAT3& up)
{
	// 視点、注視点、上方向からビュー行列を作成
	DirectX::XMVECTOR Eye = DirectX::XMLoadFloat3(&eye);
	DirectX::XMVECTOR Focus = DirectX::XMLoadFloat3(&focus);
	DirectX::XMVECTOR Up = DirectX::XMLoadFloat3(&up);
	DirectX::XMMATRIX View = DirectX::XMMatrixLookAtLH(Eye, Focus, Up);
	DirectX::XMStoreFloat4x4(&view, View);

	// カメラの方向を取り出す
	this->right = { view._11, view._21, view._31 };
	this->up = { view._12, view._22, view._32 };
	this->front = { view._13, view._23, view._33 };

	// 視点、注視点を保存
	this->eye = eye;
	this->focus = focus;
}

// パースペクティブ設定
void Camera::SetPerspectiveFov(float fovY, float aspect, float nearZ, float farZ)
{
	// 画角、画面比率、クリップ距離からプロジェクション行列を作成
	DirectX::XMMATRIX Projection = DirectX::XMMatrixPerspectiveFovLH(fovY, aspect, nearZ, farZ);
	DirectX::XMStoreFloat4x4(&projection, Projection);
}

//クォータービュー用のヘルパー
void Camera::SetQuarterView(const DirectX::XMFLOAT3& focus,
	float yawDeg, float pitchDeg, float distance)
{
	using namespace DirectX;
	const float yaw = XMConvertToRadians(yawDeg);
	const float pitch = XMConvertToRadians(pitchDeg);

	// 前方（eye→focus）の単位ベクトル
	XMFLOAT3 fwd{
		cosf(pitch) * sinf(yaw),
		-sinf(pitch),
		cosf(pitch) * cosf(yaw)
	};

	// eye = focus - fwd * distance
	XMFLOAT3 eye{
		focus.x - fwd.x * distance,
		focus.y - fwd.y * distance,
		focus.z - fwd.z * distance
	};

	SetLookAt(eye, focus, XMFLOAT3{ 0, 1, 0 }); // ロール固定（Up は常に +Y）
}
