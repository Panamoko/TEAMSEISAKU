// CameraController.cpp（差し替え用フル版）
#include "CameraController.h"

#include "System/Input.h"
#include "System/Mouse.h"
#include "System/Graphics.h"
#include "Camera.h"

#include <DirectXMath.h>
#include <algorithm>
#include <cmath>

using namespace DirectX;

bool CameraController::s_enable = true;

CameraController::CameraController()
    : panVelX(0.0f), panVelZ(0.0f), zoomVel(0.0f), isInitialized(false)
{
}

void CameraController::SyncAngleFromCamera()
{
    Camera& camera = Camera::Instance();
    XMVECTOR Eye = XMLoadFloat3(&camera.GetEye());
    XMVECTOR Focus = XMLoadFloat3(&camera.GetFocus());
    XMVECTOR Dir = XMVectorSubtract(Eye, Focus);

    // 距離更新
    distance = XMVectorGetX(XMVector3Length(Dir));

    // 角度逆算
    XMFLOAT3 dirF;
    XMStoreFloat3(&dirF, Dir);
    yawDeg = XMConvertToDegrees(atan2f(dirF.x, dirF.z)) + 180.0f;
    float xzLen = sqrtf(dirF.x * dirF.x + dirF.z * dirF.z);
    pitchDeg = XMConvertToDegrees(atan2f(dirF.y, xzLen));
}

void CameraController::Update(float elapsedTime)
{
    if (!s_enable) return;

    Input& input = Input::Instance();
    Mouse& mouse = input.GetMouse();
    Camera& camera = Camera::Instance();

    // 初回、またはカメラが外部要因（プレイヤー追従など）で強制移動された場合に備えて
    // フレームの最初に「今のカメラ状態」と「内部変数」を同期する
    // これをやると、パンとオービットが喧嘩しなくなる
    if (!isInitialized) {
        SyncAngleFromCamera();
        isInitialized = true;
    }

    // --- 入力処理 ---
    float dx = (float)mouse.GetDX();
    float dy = (float)mouse.GetDY();
    int wheel = mouse.GetWheel();

    // 1. ズーム入力 (慣性に加算)
    if (wheel != 0) {
        zoomVel += -(float)wheel * 1.0f;
    }

    // 2. パン入力 (慣性に加算)
    if (mouse.GetButton() & Mouse::BTN_RIGHT)
    {
        // 感度調整曲線
        auto curve = [](float v) { return (v >= 0 ? 1.f : -1.f) * std::pow(std::fabs(v), 1.2f); };
        dx = curve(dx);
        dy = curve(dy);

        // 距離に応じた移動量補正
        float panSens = distance * 0.0001f;

        // カメラの向きに合わせて移動ベクトルを作成
        XMFLOAT3 r = camera.GetRight();
        XMFLOAT3 f = camera.GetFront();
        // Y成分を消してXZ平面移動にする（RTSっぽい挙動なら）
        XMVECTOR Right = XMVector3Normalize(XMVectorSet(r.x, 0.0f, r.z, 0.0f));
        XMVECTOR Front = XMVector3Normalize(XMVectorSet(f.x, 0.0f, f.z, 0.0f));

        XMVECTOR Move = XMVectorAdd(
            XMVectorScale(Right, -dx * panSens),
            XMVectorScale(Front, dy * panSens)
        );

        XMFLOAT3 m; XMStoreFloat3(&m, Move);
        panVelX += m.x;
        panVelZ += m.z;
    }

    // 3. オービット入力 (角度を直接操作)
    bool isOrbiting = (mouse.GetButton() & Mouse::BTN_MIDDLE);
    if (isOrbiting)
    {
        yawDeg += dx * 0.25f;
        pitchDeg += dy * 0.25f;
        pitchDeg = std::clamp(pitchDeg, minPitchDeg, maxPitchDeg);
    }

    // --- 慣性の適用と減衰 ---
    // 減衰係数 (フレームレート依存を軽減する計算式)
    auto decay = [&](float vel, float tau) {
        return vel * (1.0f - std::min<float>(1.0f, elapsedTime / tau)); // 簡易的な減衰
        // または: vel * exp(-elapsedTime / tau);
        };

    panVelX = decay(panVelX, 0.1f);
    panVelZ = decay(panVelZ, 0.1f);
    zoomVel = decay(zoomVel, 0.15f);

    // --- 最終的なカメラ更新 ---

    // A. パン（注視点の移動）
    XMFLOAT3 focus = camera.GetFocus();
    focus.x += panVelX;
    focus.z += panVelZ;

    // B. ズーム（距離の変更）
    distance += zoomVel;
    distance = std::clamp(distance, minDistance, maxDistance);

    // C. 姿勢の反映
    // オービットしていない時も、SetQuarterViewを通して位置を決定することで
    // 「パン移動」と「回転」が常に整合性を保つ
    camera.SetQuarterView(focus, yawDeg, pitchDeg, distance);
}