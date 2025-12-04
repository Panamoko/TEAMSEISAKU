#include "System/Graphics.h"
#include "System/Input.h"
#include "SceneLoading.h"
#include "SceneManager.h"
#include <algorithm>

// 初期化
void SceneLoading::Initialize()
{
    // スプライト初期化
    sprite = new Sprite("Data/Sprite/LoadingIcon.png");

	// スレッド開始
	thread = new std::thread(LoadingThread, this);

	// Dissolve初期化
	dissolve = std::make_unique<Dissolve>();
	dissolve->Initialize(Graphics::Instance().GetDevice(), "Data/Sprite/DissolveNoise.png");

	// 最初は「完全に黒」からスタートして、徐々に晴れていく
	transitionTimer = transitionDuration;
	// 「まずは画面を表示していくモード」にする
	isFadeInMode = true;
}

// 終了化
void SceneLoading::Finalize()
{
	// スレッド終了化
	if (thread != nullptr)
	{
		thread->join();
		delete thread;
		thread = nullptr;
	}


	// スプライト終了化
	if (sprite != nullptr)
	{
		delete sprite;
		sprite = nullptr;
	}
}

// 更新処理
void SceneLoading::Update(float elapsedTime)
{
	constexpr float speed = 180;
	angle += speed * elapsedTime;

	// --- ディゾルブ演出ロジック ---
	if (isFadeInMode)
	{
		// 【フェードイン】 (真っ黒 1.0 -> 表示 0.0)
		transitionTimer -= elapsedTime;

		if (transitionTimer <= 0.0f)
		{
			transitionTimer = 0.0f;

			// 画面が見えきった状態で、次のシーンのロードが終わっているか確認
			if (nextScene && nextScene->IsReady())
			{
				// ロード完了＆フェードイン完了なら、フェードアウトへ移行
				isFadeInMode = false;
			}
			// ロードがまだなら、transitionTimer=0(見える状態)のまま待機
		}
	}
	else
	{
		// 【フェードアウト】 (表示 0.0 -> 真っ黒 1.0)
		transitionTimer += elapsedTime;

		// 完全に隠れきったらシーン切り替え
		if (transitionTimer >= transitionDuration)
		{
			transitionTimer = transitionDuration;
			if (nextScene)
			{
				SceneManager::Instance().ChangeScene(nextScene);
				nextScene = nullptr;
			}
		}
	}
}

// 描画処理
void SceneLoading::Render()
{
	Graphics& graphics = Graphics::Instance();
	ID3D11DeviceContext* dc = graphics.GetDeviceContext();
	RenderState* renderState = graphics.GetRenderState();

	// 描画準備
	RenderContext rc;
	rc.deviceContext = dc;
	rc.renderState = renderState;

	// 1. ローディングアイコン描画
	if (sprite)
	{
		float sw = graphics.GetScreenWidth();
		float sh = graphics.GetScreenHeight();
		sprite->Render(rc, sw - 150, sh - 150, 0, 128, 128, angle, 1, 1, 1, 1);
	}

	// 2. ディゾルブ演出を最前面に描画
	if (dissolve)
	{
		float t = std::clamp(transitionTimer / transitionDuration, 0.0f, 1.0f);
		// t=1.0(真っ黒) <---> t=0.0(透明)
		dissolve->Render(dc, t);
	}
}

void SceneLoading::DrawGUI()
{
}

// ローディングスレッド
void SceneLoading::LoadingThread(SceneLoading* scene)
{
	// COM関連の初期化でスレッド毎に呼ぶ必要がある
	CoInitialize(nullptr);

	// 次のシーンの初期化を行う
	scene->nextScene->Initialize();

	// スレッドが終わる前にCOM関連の終了化
	CoUninitialize();

	// 次のシーンの準備完了設定
	scene->nextScene->SetReady();	//ready = true;

}
