#include "Core.h"
#include "Factory.h"
#include "ModelManager.h"
#include "SceneManager.h"
#include "SceneTitle.h"
#include "SceneLoading.h"
#include <imgui.h>
#include <cmath>
#include "Camera.h"          
#include "CameraController.h"
#include "SceneGame.h"
#include "SpriteManager.h"     
#include "System/Graphics.h"   

Core* Core::sInstance = nullptr;

Core* Core::Instance() { return sInstance; }

Core::Core()
{
	//生成時に自身を登録
	sInstance = this;

	model = ModelManager::Instance().Load("Data/Model/bilud/Core.mdl");

	animator.SetModel(model /* or model.get() */);
	animator.SetBlendSeconds(0.2f);
	animator.Play("Take 001", true);

	collider = std::make_unique<CylinderCollider>();
	collider->type = ColliderType::Cylinder;
	collider->owner = this;
	cylinder = static_cast<CylinderCollider*>(collider.get());
	cylinder->height = 7.0f;
	cylinder->radius = 3.5f;
	CollisionManager::Instance().AddObject(this);

	class_name = "Core";
	scale = { 0.3f, 0.3f, 0.3f };
	hp = 1500.0f;
    hitSE[1] = Audio::Instance().LoadAudioSource("Data/Sound/SE_CoreBreak.wav");

    // 演出用画像の読み込み
    overlaySprite = SpriteManager::Instance().Load("Data/Sprite/Baxkground.png");
    clearLogoSprite = SpriteManager::Instance().Load("Data/Sprite/Game Clear.png");
}

Core::~Core()
{
	if (sInstance == this)
	{
		sInstance = nullptr;
	}
}

void Core::init()
{
	CollisionManager::Instance().AddObject(this);
}

void Core::Update(float elapsedTime)
{
    UpdateInvicible(elapsedTime);

    // --- ★死亡演出中の処理 ---
    if (isDying)
    {
        // 1. 時間管理 (実時間ベース)
        float realDt = elapsedTime / std::max<float>(1e-6f, currentSlowScale);
        dyingTimer += realDt;
        float t = std::min<float>(1.0f, dyingTimer / dyingDuration);

        // 2. イージング (EaseOutQuart でより粘りのある動きに)
        float easeT = 1.0f - powf(1.0f - t, 4.0f);

        // ★追加: スロー倍率を徐々に落とす (10% -> 1% へ)
        // 最初は動きが見えて、最後は時が止まったように見せる
        float newScale = std::lerp(0.1f, 0.005f, t);
        SceneGame::SetSlowMotion(newScale, 0.1f); // 毎フレーム更新

        // 3. カメラ演出: オービット
        Camera& camera = Camera::Instance();
        XMVECTOR vCorePos = XMLoadFloat3(&position);
        XMVECTOR vStartFocus = XMLoadFloat3(&startFocus);

        // 注視点補間
        XMVECTOR vCurrentFocus = XMVectorLerp(vStartFocus, vCorePos, easeT);

        // 角度・距離計算
        float currentYaw = startYawDeg + (dyingTimer * orbitSpeed); // 旋回
        float currentPitch = std::lerp(startPitchDeg, endPitchDeg, easeT);
        float currentDist = std::lerp(startDistance, endDistance, easeT);

        XMFLOAT3 finalFocus;
        XMStoreFloat3(&finalFocus, vCurrentFocus);

        // シェイク処理 (演出初期に激しく揺らす)
        if (dyingTimer < 0.5f) // 最初の0.5秒だけ揺らす
        {
            shakeMagnitude = (1.0f - (dyingTimer / 0.5f)) * 1.5f; // 1.5mの幅で減衰振動

            // ランダムなオフセットを作成
            float rx = ((rand() % 100) / 50.0f - 1.0f) * shakeMagnitude;
            float ry = ((rand() % 100) / 50.0f - 1.0f) * shakeMagnitude;
            float rz = ((rand() % 100) / 50.0f - 1.0f) * shakeMagnitude;

            // 注視点をずらすことで画面揺れを表現
            finalFocus.x += rx;
            finalFocus.y += ry;
            finalFocus.z += rz;
        }

        camera.SetQuarterView(finalFocus, currentYaw, currentPitch, currentDist);
        animator.Update(elapsedTime);

        // UIアニメーション計算

        // A. 黒背景のフェードイン (最初の1秒で 透明 -> 0.7)
        float fadeTime = 1.0f;
        float alphaT = std::clamp(dyingTimer / fadeTime, 0.0f, 1.0f);
        overlayAlpha = std::lerp(0.0f, 0.7f, alphaT); // 最大0.7(半透明)まで

        // B. ロゴの落下 (0.5秒待ってから、1秒かけて落下)
        float dropDelay = 0.5f;
        float dropDuration = 1.0f;
        float screenH = Graphics::Instance().GetScreenHeight();
        float targetY = (screenH / 2.0f) - 150.0f; // 画面中央付近（画像の高さに合わせて調整）

        if (dyingTimer > dropDelay)
        {
            float dropT = std::clamp((dyingTimer - dropDelay) / dropDuration, 0.0f, 1.0f);

            // バウンスっぽいイージング (EaseOutBack風)
            float c1 = 1.70158f;
            float c3 = c1 + 1.0f;
            float easeDrop = 1.0f + c3 * powf(dropT - 1.0f, 3.0f) + c1 * powf(dropT - 1.0f, 2.0f);

            // ロゴのY座標を更新
            logoPosY = std::lerp(-300.0f, targetY, easeDrop);

            if (dropT >= 1.0f)
            {
                // 落下終了からの経過時間
                float floatTime = dyingTimer - (dropDelay + dropDuration);

                // sin波でオフセットを加算 (速度2.0, 振幅15.0px)
                logoPosY += sinf(floatTime * 2.0f) * 15.0f;
            }
        }
        else
        {
            logoPosY = -300.0f; // まだ時間になっていなければ画面外待機
        }

        // 終了判定
        if (dyingTimer >= dyingDuration)
        {
            CameraController::SetEnable(true);
            GimmicManager::Instance().Remove(this);
            SceneManager::Instance().ChangeScene(new SceneLoading(new SceneTitle));
        }
        return;
    }

    // --- 通常時の処理 ---

    // HPチェック
    if (hp <= 0.0f)
    {
        hitSE[1]->Play(false);
        // 死亡演出開始
        if (!isDying)
        {
            isDying = true;
            dyingTimer = 0.0f; // 実時間タイマーリセット

            // UI初期化
            overlayAlpha = 0.0f;
            logoPosY = -300.0f;

            // 全体スローモーションを開始
            float slowScale = 0.1f; // (10%の速度)
            SceneGame::SetSlowMotion(slowScale, dyingDuration);
            currentSlowScale = slowScale; // スロー倍率を保存

            // カメラ操作を無効化
            CameraController::SetEnable(false);

            // 現在のカメラ状態を保存 (オービット用にYaw/Pitch/Distも)
            Camera& camera = Camera::Instance();
            startFocus = camera.GetFocus();

            // 現在のEye, FocusからYaw/Pitch/Distanceを逆算
            XMVECTOR vEye = XMLoadFloat3(&camera.GetEye());
            XMVECTOR vFocus = XMLoadFloat3(&startFocus);
            XMVECTOR vDir = XMVectorSubtract(vEye, vFocus); // Focus -> Eye

            // 距離
            startDistance = XMVectorGetX(XMVector3Length(vDir));

            // Yaw (XZ平面での角度)
            float x = XMVectorGetX(vDir);
            float z = XMVectorGetZ(vDir);
            startYawDeg = XMConvertToDegrees(atan2f(x, z)); // Z軸(前方)からX軸(右)への角度

            // Pitch (Y方向の角度)
            float y = XMVectorGetY(vDir);
            float xzLen = sqrtf(x * x + z * z);
            startPitchDeg = XMConvertToDegrees(atan2f(y, xzLen)); // 水平面からの角度

            // コライダーを削除
            CollisionManager::Instance().Remove(this);
            cylinder = nullptr;
        }
        return;
    }

    // 通常のアニメーション更新 (スロー化された時間で)
    animator.Update(elapsedTime);

    if (cylinder)
    {
        cylinder->center = position;
    }
}

// 2D UI描画関数
void Core::RenderUI(const RenderContext& rc)
{
    if (!isDying) return;

    float sw = Graphics::Instance().GetScreenWidth();
    float sh = Graphics::Instance().GetScreenHeight();

    // 1. 黒背景の描画（画面いっぱいに引き伸ばす）
    if (overlaySprite)
    {
        // Sprite::Render(rc, x, y, z, w, h, angle, r, g, b, a)
        overlaySprite->Render(rc, 0, 0, 0, sw, sh, 0, 1.0f, 1.0f, 1.0f, overlayAlpha);
    }

    // 2. ゲームクリアロゴの描画
    if (clearLogoSprite)
    {
        // 画像サイズ（適宜調整してください）
        float logoW = 800.0f;
        float logoH = 300.0f;
        float logoX = (sw - logoW) * 0.5f; // 中央揃え

        // ロゴは常に不透明(1.0)で表示（背景フェードとは別）
        clearLogoSprite->Render(rc, logoX, logoPosY, 0, logoW, logoH, 0, 1.0f, 1.0f, 1.0f, 1.0f);
    }
}

void Core::Render(const RenderContext& rc, ModelRenderer* renderer)
{
    if (invincible_timer <= 0.0f)
    {
        renderer->Render(rc, transform, model, ShaderId::Lambert);
    }
    else
    {
        renderer->Render(rc, transform, model, ShaderId::Lambert, { 1.0f,0.0f,0.0f,1.0f });
    }
}



void Core::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
	if (hp <= 0.0f || !cylinder) return;

	// コアの当たり判定（円柱）を赤色で表示
	//renderer->RenderCylinder(
	//	rc,
	//	cylinder->center,    // 中心（地面に置く座標）
	//	cylinder->radius,      // 半径
	//	cylinder->height,      // 高さ
	//	XMFLOAT4(1, 0, 0, 1)
	//);
}

void Core::OnCollision(GameObject* object)
{
    if (hp <= 0.0f || isDying) return;

    if (object->type == Type::PlayerAttack && invincible_timer <= 0.0f)
    {
        hitSE[0]->Play(false);
        hp -= 10.0f;
        invincible_timer = 0.1f;
    }
}

bool Core::OnImGui()
{
	if (ImGui::CollapsingHeader("Core"))
	{
		ImGui::DragFloat("HP", &hp, 0.1f, 0.0f, 1500.0f, "%.1f");
        return true;
	}
    return false;
}

REGISTER_GAMEOBJECT(Core);

