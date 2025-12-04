#include "System/Graphics.h"
#include "SceneTitle.h"
#include "System/Input.h"
#include "SceneGame.h"
#include "SceneManager.h"
#include "SceneLoading.h"
#include "SceneFactory.h"
#include "SpriteManager.h"
#include "Collision.h" // IntersectPosSquare用
#include <algorithm>

// 初期化
void SceneTitle::Initialize()
{
	// スプライト初期化
	sprite = SpriteManager::Instance().Load("Data/Sprite/Title.png");

	// ボタン用スプライトと座標設定
	spriteStart = SpriteManager::Instance().Load("Data/Sprite/GameStage.png");
	startButtonPos = { 430, 370, 0 };
	startButtonSize = { 500, 300, 0 };

	// ★追加: チュートリアルボタン
	spriteTutorial = SpriteManager::Instance().Load("Data/Sprite/Tutorial.png"); // ※画像名は確認してください
	tutorialButtonPos = { 430, 400, 0 }; // Startボタンの下に配置
	tutorialButtonSize = { 500, 400, 0 }; // サイズ調整

	scene_name = "scene_title";

	// Dissolve初期化
	dissolve = std::make_unique<Dissolve>();
	dissolve->Initialize(Graphics::Instance().GetDevice(), "Data/Sprite/DissolveNoise.png");

	change_scene = std::make_unique<ChangeSceneSytem>();

	isSceneChanging = false;
	transitionTimer = 0.0f;
	pendingNextScene = nullptr;
}

// 終了化
void SceneTitle::Finalize()
{
	// unique_ptr以外の後始末があればここに記述
	// SpriteManagerからLoadしたポインタはManager管理ならdelete不要ですが、
	// newした場合はdeleteが必要です。SpriteManagerの仕様に合わせます。
}

// 更新処理
void SceneTitle::Update(float elapsedTime)
{
	// 点滅処理
	alpha_timer += elapsedTime;
	if (std::fmod(alpha_timer, blink_interval * 2.0f) < blink_interval) render_color.w = 1.0f;
	else render_color.w = 0.0f;

	// --- シーン遷移中の処理 ---
	if (isSceneChanging)
	{
		// 遷移中ならタイマーを進める (0.0 -> 1.0 へ)
		transitionTimer += elapsedTime;

		// 完全に暗転したら、保存しておいたシーンへ切り替える
		if (transitionTimer >= transitionDuration)
		{
			if (pendingNextScene)
			{
				// Loadingシーンを経由せず、直接次のシーンへ切り替える
				SceneManager::Instance().ChangeScene(pendingNextScene);
			}
			else
			{
				// 万が一nullなら通常ゲームへ直接切り替える
				SceneManager::Instance().ChangeScene(new SceneGame());
			}
		}
		// 遷移中は入力処理を行わない
		return;
	}

	// --- 入力判定 ---
	Mouse& mouse = Input::Instance().GetMouse();
	DirectX::XMFLOAT2 mousePos = {
		static_cast<float>(mouse.GetPositionX()),
		static_cast<float>(mouse.GetPositionY())
	};

	bool isClick = (mouse.GetButtonDown() & Mouse::BTN_LEFT);

	if (isClick)
	{
		// 1. Game Start ボタンの判定
		bool hitStart = Collision::IntersectPosSquare(
			mousePos,
			{ startButtonPos.x, startButtonPos.y },
			{ startButtonSize.x, startButtonSize.y }
		);

		if (hitStart)
		{
			isSceneChanging = true;
			transitionTimer = 0.0f;
			pendingNextScene = new SceneGame("scene_play");
		}
		// else if に変更して、Startが押されたらTutorial判定をスキップする
		
		change_scene->Update(elapsedTime);

		//else
		//{
		//	// 2. Tutorial ボタンの判定
		//	bool hitTutorial = Collision::IntersectPosSquare(
		//		mousePos,
		//		{ tutorialButtonPos.x, tutorialButtonPos.y },
		//		{ tutorialButtonSize.x, tutorialButtonSize.y }
		//	);

		//	if (hitTutorial)
		//	{
		//		isSceneChanging = true;
		//		transitionTimer = 0.0f;
		//		pendingNextScene = new SceneGame("scene_tutorial");
		//	}
		//}
	}
}

// 描画処理
void SceneTitle::Render()
{
	Graphics& graphics = Graphics::Instance();
	ID3D11DeviceContext* dc = graphics.GetDeviceContext();
	RenderState* renderState = graphics.GetRenderState();
	ModelRenderer* modelRenderer = graphics.GetModelRenderer();

	RenderContext rc;
	rc.deviceContext = dc;
	rc.renderState = renderState;

	// 背景や3Dオブジェクト
	game_editor.render(objects, sprites, ModelManager::Instance().GetModels(), modelRenderer);

	// 2Dスプライト描画
	float sw = static_cast<float>(graphics.GetScreenWidth());
	float sh = static_cast<float>(graphics.GetScreenHeight());

	// タイトルロゴ
	if (sprite)
		sprite->Render(rc, 150, 0, 0, sw * 0.8f, sh * 0.8f, 0, 1, 1, 1, 1);

	// Game Start ボタン (点滅)
	if (spriteStart)
		spriteStart->Render(rc,
			startButtonPos.x, startButtonPos.y, startButtonPos.z,
			startButtonSize.x, startButtonSize.y,
			0, 1, 1, 1, render_color.w);

	// ★追加: Tutorial ボタン (こちらは点滅させず表示する例)
	
	change_scene->Render();

	//if (spriteTutorial)
	//	spriteTutorial->Render(rc,
	//		tutorialButtonPos.x, tutorialButtonPos.y, tutorialButtonPos.z,
	//		tutorialButtonSize.x, tutorialButtonSize.y,
	//		0, 1, 1, 1, 1.0f); // 常に表示

	// ディゾルブ演出 (最前面)
	if (isSceneChanging && dissolve)
	{
		float t = std::clamp(transitionTimer / transitionDuration, 0.0f, 1.0f);
		dissolve->Render(dc, t);
	}
}

// GUI描画
void SceneTitle::DrawGUI()
{
}

REGISTER_SCENE(SceneTitle);