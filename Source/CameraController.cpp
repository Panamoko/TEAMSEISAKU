#include "CameraController.h"

#include "System/Input.h"
#include "Camera.h"
#include "System/Mouse.h"
#include <DirectXMath.h>
#include <algorithm>
#include <cmath>


using namespace DirectX;

void CameraController::Update(float elapsedTime)
{
	Input& input = Input::Instance();
	Mouse& mouse = input.GetMouse();
	Camera& camera = Camera::Instance();

	// マウスホイール入力取得
	int wheel = mouse.GetWheel();

	// 1ノッチあたりの移動量（ワールド単位）
	const float zoomStepPerNotch = 1.0f; // 好みに応じて 0.2f ～ 1.0f 程度で調整

	if (wheel != 0)
	{
		// 現在の視点と注視点
		XMFLOAT3 eyeF = camera.GetEye();
		XMFLOAT3 focusF = camera.GetFocus();

		XMVECTOR Eye = XMLoadFloat3(&eyeF);
		XMVECTOR Focus = XMLoadFloat3(&focusF);

		// eye→focus 方向
		XMVECTOR dir = XMVectorSubtract(Focus, Eye);
		float currentDist = XMVectorGetX(XMVector3Length(dir));
		XMVECTOR dirN = XMVector3Normalize(dir);

		// 近すぎ/遠すぎを制限
		const float minDist = 10.0f;
		const float maxDist = 30.0f;
		float delta = -(float)wheel * zoomStepPerNotch;
		if (currentDist <= minDist + 0.01f && delta > 0) delta *= 0.2f;
		if (currentDist >= maxDist - 0.01f && delta < 0) delta *= 0.2f;
		float desiredDist = currentDist + delta;
		desiredDist = std::clamp(desiredDist, minDist, maxDist);

		// 新しい視点 = focus から dirN 方向に desiredDist 離れた位置
		XMVECTOR NewEye = XMVectorSubtract(Focus, XMVectorScale(dirN, desiredDist));

		XMStoreFloat3(&eyeF, NewEye);

		// 反映（上方向はY+固定）
		camera.SetLookAt(eyeF, focusF, XMFLOAT3(0, 1, 0));
	}

	// マウス感度。小さいほどゆっくり動く
	const float panSpeed = 0.01f;

	float dx = 0.0f;
	float dy = 0.0f;

	// マウス右ボタンが押されている場合のみ、カメラパン用のマウス移動量を取得
	if (mouse.GetButton() & Mouse::BTN_RIGHT)
	{
		// マウス移動量を取得 (intをfloatにキャスト)
		dx = (float)mouse.GetDX(); // 左右移動量
		dy = (float)mouse.GetDY(); // 上下移動量
	}

	// マウス移動があった場合のみ処理を実行
	if (fabsf(dx) > 0.0f || fabsf(dy) > 0.0f)
	{
		// カメラの右方向ベクトルを取得（ワールドのX軸に近い方向）
		const XMFLOAT3& cameraRight = camera.GetRight();
		// カメラの前方向ベクトルを取得（ワールドのZ軸に近い方向）
		const XMFLOAT3& cameraFront = camera.GetFront();

		// 1. カメラの右方向をXZ平面に射影し、正規化
		XMVECTOR Right = XMVectorSet(cameraRight.x, 0.0f, cameraRight.z, 0.0f);
		Right = XMVector3Normalize(Right);

		// 2. カメラの前方向をXZ平面に射影し、正規化
		XMVECTOR Front = XMVectorSet(cameraFront.x, 0.0f, cameraFront.z, 0.0f);
		Front = XMVector3Normalize(Front);

		// 3. マウスの移動量とパン速度を使って移動ベクトルを計算
		// マウスを右に動かす(dx > 0)と、カメラ視点も右に動くようにdxにマイナスをかける (CoC的な操作感)
		XMVECTOR move = XMVectorAdd(
			XMVectorScale(Right, -dx * panSpeed),
			XMVectorScale(Front, dy * panSpeed)
		);

		// 注視点と視点を更新するためのベクトル
		XMFLOAT3 moveVec = { 0, 0, 0 };
		XMStoreFloat3(&moveVec, move);


		// 現在の視点と注視点に移動ベクトルを加算
		XMFLOAT3 newEye;
		XMFLOAT3 newFocus;

		newEye.x = camera.GetEye().x + moveVec.x;
		newEye.y = camera.GetEye().y + moveVec.y;
		newEye.z = camera.GetEye().z + moveVec.z;

		newFocus.x = camera.GetFocus().x + moveVec.x;
		newFocus.y = camera.GetFocus().y + moveVec.y;
		newFocus.z = camera.GetFocus().z + moveVec.z;

		// カメラ位置を更新
		camera.SetLookAt(newEye, newFocus, XMFLOAT3(0, 1, 0));
	}
}