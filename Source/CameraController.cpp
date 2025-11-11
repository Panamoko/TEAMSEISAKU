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

void CameraController::Update(float elapsedTime)
{
    Input& input = Input::Instance();
    Mouse& mouse = input.GetMouse();
    Camera& camera = Camera::Instance();

    // ---- 追加状態（ヘッダは触らず、ここで永続化） ----
    static bool  s_inited = false;
    static float s_panVelX = 0.0f;   // パン慣性（X）
    static float s_panVelZ = 0.0f;   // パン慣性（Z）
    static float s_zoomVel = 0.0f;   // ズーム慣性（距離変化  [units/frame]）

    // チューニング係数（お好みで）
    const float panTau = 0.08f;     // パン減衰の時定数（秒）  0.06～0.15 推奨
    const float zoomTau = 0.10f;     // ズーム減衰の時定数（秒）
    const float wheelStepPerNotch = 2.0f; // ホイール1ノッチあたりの距離変化目安
    const float rotSpeed = 0.25f;     // オービット回転 [deg/px]
    const float minD = 10.0f;         // 最小距離
    const float maxD = 65.0f;         // 最大距離

    // 現在のEye/Focus/距離
    XMFLOAT3 eyeF = camera.GetEye();
    XMFLOAT3 focusF = camera.GetFocus();
    XMVECTOR Eye = XMLoadFloat3(&eyeF);
    XMVECTOR Focus = XMLoadFloat3(&focusF);
    float currentDist = XMVectorGetX(XMVector3Length(XMVectorSubtract(Focus, Eye)));

    if (!s_inited) {
        // 初回のみ：distanceフィールドを現在距離で初期化
        if (distance <= 0.0f) distance = currentDist;
        s_inited = true;
    }

    // ====== 入力デルタ ======
    float dx = (float)mouse.GetDX();
    float dy = (float)mouse.GetDY();

    // ====== ホイール：ズーム（慣性に積む） ======
    if (int wheel = mouse.GetWheel(); wheel != 0) {
        // 既存挙動に合わせて「-wheel」で近づく/遠ざかる
        s_zoomVel += -(float)wheel * wheelStepPerNotch;
    }

    // ====== 右ドラッグ：パン（距離/FOV連動＋慣性） ======
    if (mouse.GetButton() & Mouse::BTN_RIGHT)
    {
        // 微小ノイズ抑制
        auto dead = [](float v) { return (std::fabs(v) < 0.5f) ? 0.0f : v; };
        dx = dead(dx);
        dy = dead(dy);

        // 微操作の解像感を上げる軽い非線形
        auto curve = [](float v) { float a = std::fabs(v); return (v >= 0 ? 1.f : -1.f) * std::pow(a, 1.2f); };
        dx = curve(dx);
        dy = curve(dy);

        // プロジェクション行列からFOVを復元
        const auto& P = camera.GetProjection();
        float fovY = 2.0f * std::atan(1.0f / P._22); // P._22 = 1 / tan(fovY/2)
        float screenH = (float)Graphics::Instance().GetScreenHeight();

        // 距離とFOVから「1pxが何ワールド単位か」
        // ※ currentDist を使うことでズーム後も手触り一定
        float panSensitivity = 0.7f;
        float unitsPerPixel = (2.0f * currentDist * std::tan(fovY * 0.5f)) / std::max<float>(1.0f, screenH);
        unitsPerPixel *= panSensitivity;

        // カメラRight/FrontのXZ投影
        XMFLOAT3 r = camera.GetRight();
        XMFLOAT3 f = camera.GetFront();
        XMVECTOR Right = XMVector3Normalize(XMVectorSet(r.x, 0.0f, r.z, 0.0f));
        XMVECTOR Front = XMVector3Normalize(XMVectorSet(f.x, 0.0f, f.z, 0.0f));

        // 既存の符号を踏襲（右へドラッグで画面も右へ動く→Rightに -dx）
        XMVECTOR move = XMVectorAdd(
            XMVectorScale(Right, -dx * unitsPerPixel),
            XMVectorScale(Front, dy * unitsPerPixel)
        );

        XMFLOAT3 m; XMStoreFloat3(&m, move);
        s_panVelX += m.x;   // 慣性（速度）に積む
        s_panVelZ += m.z;
    }

    // ====== 中ボタンドラッグ：オービット（距離は常に“今”を使う） ======
    if (mouse.GetButton() & Mouse::BTN_MIDDLE)
    {
        yawDeg += dx * rotSpeed;
        pitchDeg += dy * rotSpeed;                       // 逆に感じたら -= dy
        pitchDeg = std::clamp(pitchDeg, minPitchDeg, maxPitchDeg);

        // Eye/Focus から毎フレーム距離を再算出して使うことで
        // 「中ボタン開始で昔のdistanceに戻る」問題を回避
        XMVECTOR EyeNow = XMLoadFloat3(&camera.GetEye());
        XMVECTOR FocusNow = XMLoadFloat3(&camera.GetFocus());
        float curDistForOrbit = XMVectorGetX(
            XMVector3Length(XMVectorSubtract(FocusNow, EyeNow))
        );

        // オービットを優先して即反映（パン/ズーム慣性は同時進行でもOK）
        camera.SetQuarterView(camera.GetFocus(), yawDeg, pitchDeg,
            std::clamp(curDistForOrbit, minD, maxD));
        // ※returnしない：慣性によるパン/ズームもこのフレームで進める
    }

    // ====== 速度の指数減衰（dt依存） ======
    auto alpha = [](float dt, float tau) {
        return 1.0f - std::exp(-dt / std::max<float>(1e-4f, tau));
        };
    float ap = alpha(elapsedTime, panTau);
    float az = alpha(elapsedTime, zoomTau);

    // ---- ズーム（距離）を慣性で更新
    {
        // 速度→距離変化（減衰適用）
        float deltaDist = s_zoomVel * az;
        s_zoomVel -= s_zoomVel * az;

        float desired = std::clamp(currentDist + deltaDist, minD, maxD);

        // dirN（focus→eye 方向）を保持して eye を更新（focusはまだこの後パンで動く）
        XMVECTOR dir = XMVectorSubtract(Focus, Eye);
        XMVECTOR dirN = XMVector3Normalize(dir);
        XMVECTOR NewEye = XMVectorSubtract(Focus, XMVectorScale(dirN, desired));
        XMStoreFloat3(&eyeF, NewEye);

        // SetLookAt で一旦反映（Upは+Y固定）
        camera.SetLookAt(eyeF, focusF, XMFLOAT3(0, 1, 0));

        // Controllerのdistanceを最新に寄せておく（他所で参照されてもズレないように）
        distance = desired;

        // 最新値を再取得して以降の計算に使う
        eyeF = camera.GetEye();
        focusF = camera.GetFocus();
        Eye = XMLoadFloat3(&eyeF);
        Focus = XMLoadFloat3(&focusF);
        currentDist = XMVectorGetX(XMVector3Length(XMVectorSubtract(Focus, Eye)));
    }

    // ---- パン（XZ）を慣性で更新
    {
        float moveX = s_panVelX * ap;
        float moveZ = s_panVelZ * ap;
        s_panVelX -= s_panVelX * ap;
        s_panVelZ -= s_panVelZ * ap;

        if (std::fabs(moveX) + std::fabs(moveZ) > 0.0f) {
            XMFLOAT3 newEye = { camera.GetEye().x + moveX, camera.GetEye().y,   camera.GetEye().z + moveZ };
            XMFLOAT3 newFocus = { camera.GetFocus().x + moveX, camera.GetFocus().y, camera.GetFocus().z + moveZ };
            camera.SetLookAt(newEye, newFocus, XMFLOAT3(0, 1, 0));

            // 最新値を再取得（この後のオービット・次フレの基準に）
            eyeF = camera.GetEye();
            focusF = camera.GetFocus();
            Eye = XMLoadFloat3(&eyeF);
            Focus = XMLoadFloat3(&focusF);
            currentDist = XMVectorGetX(XMVector3Length(XMVectorSubtract(Focus, Eye)));
        }
    }

    // ---- 最後に yaw/pitch も反映（距離は currentDist を使って統一）
    //      ※パン＆ズームで動いた後の focus を基準に作り直すことで一貫性を保つ
    camera.SetQuarterView(focusF, yawDeg, pitchDeg, std::clamp(currentDist, minD, maxD));
}
