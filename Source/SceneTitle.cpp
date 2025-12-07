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
#include "System/Audio.h"
#include "ModelManager.h"
#include "MathUtils.h"

// 3D座標計算用
using namespace DirectX;

// タイトル画面用の簡易移動ロジック（ローカル関数）
static void UpdateWander(Character* chara, float& timer, float elapsedTime)
{
	timer -= elapsedTime;
	if (timer <= 0.0f)
	{
		// ランダムな位置へ移動開始（または待機）
		float range = 8.0f;
		float tx = MathUtils::RandomRenge(-range, range);
		float tz = MathUtils::RandomRenge(-5.0f, 5.0f);

		// 向きを変える
		DirectX::XMFLOAT3 currentPos = chara->GetPosition();
		float dx = tx - currentPos.x;
		float dz = tz - currentPos.z;

		// 簡易的に向きだけ変える
		chara->SetAngle({ 0, atan2f(dx, dz), 0 });

		timer = MathUtils::RandomRenge(1.0f, 3.0f); // 次の行動までの時間
	}

	// 前に進む
	DirectX::XMFLOAT3 angle = chara->GetAngle();
	float speed = 2.0f; // タイトル画面なのでゆっくり動かす
	float vx = sinf(angle.y);
	float vz = cosf(angle.y);

	DirectX::XMFLOAT3 pos = chara->GetPosition();
	pos.x += vx * speed * elapsedTime;
	pos.z += vz * speed * elapsedTime;

	// 画面外に出ないように制限
	if (pos.x < -10.0f || pos.x > 10.0f || pos.z < -10.0f || pos.z > 10.0f)
	{
		pos.x = -pos.x * 0.9f; // 反対側へ飛ばすか、反転させる
		pos.z = -pos.z * 0.9f;
	}
	chara->SetPosition(pos);
}

SceneTitle::SceneTitle()
{
	Audio::Instance().Initialize();
}

// 初期化
void SceneTitle::Initialize()
{
	Graphics& graphics = Graphics::Instance();

	// --- 2Dスプライト初期化 ---
	sprite = SpriteManager::Instance().Load("Data/Sprite/Title.png");
	spriteStart = SpriteManager::Instance().Load("Data/Sprite/start.png");
	startButtonPos = { 650, 650, 0 };
	startButtonSize = { 500, 300, 0 };

	spriteTutorial = SpriteManager::Instance().Load("Data/Sprite/Tutorial.png");
	tutorialButtonPos = { 650, 750, 0 };
	tutorialButtonSize = { 510, 300, 0 };

	// --- 3D背景演出の初期化 ---

	// 1. カメラの設定 (初期値)
	// ※ Update内で動かすので、ここでは最低限の設定でOK
	Camera& camera = Camera::Instance();
	float aspect = static_cast<float>(graphics.GetScreenWidth()) / static_cast<float>(graphics.GetScreenHeight());
	camera.SetPerspectiveFov(XMConvertToRadians(45.0f), aspect, 0.1f, 1000.0f);

	// 2. 地面モデルの読み込み
	// Stageクラスで使っているモデルを流用します
	groundModel = ModelManager::Instance().Load("Data/Model/Stage/Zimensi.mdl");

	// 地面の行列計算 (原点に配置、少し広めにスケールしても良い)
	XMMATRIX S = XMMatrixScaling(1.0f, 1.0f, 1.0f);
	XMMATRIX T = XMMatrixTranslation(0.0f, 0.0f, 0.0f); // 足元(Y=0)に配置
	XMStoreFloat4x4(&groundTransform, S * T);

	// 3. プレイヤーモデルの配置
	player = std::make_unique<Player>();
	player->Initialize();
	player->SetPosition({ 0.0f, 0.0f, 0.0f });
	player->SetAngle({ 0.0f, DirectX::XMConvertToRadians(180.0f), 0.0f });

	// 4. 味方スライムたちの配置
	auto slime1 = std::make_shared<AllySlimeMelee>(0, player.get());
	slime1->SetPosition({ -3.0f, 0.0f, 1.0f });
	slime1->SetAngle({ 0.0f, DirectX::XMConvertToRadians(150.0f), 0.0f });
	slimes.push_back(slime1);

	auto slime2 = std::make_shared<AllySlimeMelee>(1, player.get());
	slime2->SetPosition({ 3.0f, 0.0f, 1.0f });
	slime2->SetAngle({ 0.0f, DirectX::XMConvertToRadians(210.0f), 0.0f });
	slimes.push_back(slime2);

	moveTimers.resize(slimes.size() + 1, 0.0f);

	scene_name = "scene_title";

	// --- 演出・システム初期化 ---
	dissolve = std::make_unique<Dissolve>();
	dissolve->Initialize(Graphics::Instance().GetDevice(), "Data/Sprite/DissolveNoise.png");

	change_scene = std::make_unique<ChangeSceneSytem>();

	isSceneChanging = false;
	transitionTimer = 0.0f;
	pendingNextScene = nullptr;

	Stage_BGM = Audio::Instance().LoadAudioSource("Data/Sound/BGM_Title.wav");
}

// 終了化
void SceneTitle::Finalize()
{
	delete Stage_BGM;
}

// 更新処理
void SceneTitle::Update(float elapsedTime)
{
	Stage_BGM->Play(true);

	// カメラ演出タイマー
	cameraTimer += elapsedTime;
	XMFLOAT3 baseEye = { 0.0f, 5.0f, -8.0f };
	XMFLOAT3 baseFocus = { 0.0f, 1.0f, 0.0f };
	float swayX = sinf(cameraTimer * 0.5f) * 0.5f;
	float swayY = cosf(cameraTimer * 0.3f) * 0.2f;
	Camera& camera = Camera::Instance();
	camera.SetLookAt({ baseEye.x + swayX, baseEye.y + swayY, baseEye.z }, baseFocus, { 0.0f, 1.0f, 0.0f });

	// 3D更新
	if (player) { UpdateWander(player.get(), moveTimers[0], elapsedTime); player->Update(elapsedTime); }
	for (size_t i = 0; i < slimes.size(); ++i) { if (slimes[i]) { UpdateWander(slimes[i].get(), moveTimers[i + 1], elapsedTime); slimes[i]->Update(elapsedTime); } }

	if (isSceneChanging)
	{
		transitionTimer += elapsedTime;
		if (transitionTimer >= transitionDuration)
		{
			if (pendingNextScene) SceneManager::Instance().ChangeScene(pendingNextScene);
			else SceneManager::Instance().ChangeScene(new SceneGame());
		}
		return;
	}

	// --- 入力とホバー判定 ---
	Mouse& mouse = Input::Instance().GetMouse();
	DirectX::XMFLOAT2 mousePos = { static_cast<float>(mouse.GetPositionX()), static_cast<float>(mouse.GetPositionY()) };

	// 各ボタンのホバー判定
	isStartHover = Collision::IntersectPosSquare(mousePos, { startButtonPos.x, startButtonPos.y + 100.0f }, { startButtonSize.x - 80.0f, startButtonSize.y - 200.0f });
	isTutorialHover = Collision::IntersectPosSquare(mousePos, { tutorialButtonPos.x, tutorialButtonPos.y + 100.0f }, { tutorialButtonSize.x - 80.0f, tutorialButtonSize.y - 200.0f});

	// クリック判定
	if (mouse.GetButtonDown() & Mouse::BTN_LEFT)
	{
		if (isStartHover)
		{
			isSceneChanging = true;
			transitionTimer = 0.0f;
			pendingNextScene = new SceneGame("scene_play");
		}
		else if (isTutorialHover)
		{
			isSceneChanging = true;
			transitionTimer = 0.0f;
			// チュートリアルシーンへ遷移（例として scene_tutorial という名前で作成）
			pendingNextScene = new SceneGame("scene_tutorial");
		}
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
	Camera& camera = Camera::Instance();
	rc.view = camera.GetView();
	rc.projection = camera.GetProjection();
	rc.lightDirection = { 0.0f, -1.0f, 0.0f };

	// 1. 3Dシーン描画
	modelRenderer->BeginFrame(rc);
	if (groundModel) modelRenderer->Render(rc, groundTransform, groundModel, ShaderId::Lambert);
	if (player) player->Render(rc, modelRenderer);
	for (auto& s : slimes) s->Render(rc, modelRenderer);
	//game_editor.render(objects, sprites, ModelManager::Instance().GetModels(), modelRenderer);

	// 2. 2D UI描画
	float sw = static_cast<float>(graphics.GetScreenWidth());
	float sh = static_cast<float>(graphics.GetScreenHeight());

	// ロゴのふわふわ演出
	float logoOffsetY = sinf(cameraTimer * 2.0f) * 15.0f; // 上下にふわふわ
	if (sprite)
		sprite->Render(rc, 150, 0 + logoOffsetY, 0, sw * 0.8f, sh * 0.8f, 0, 1, 1, 1, 1);

	// ボタン描画用ヘルパー関数
	auto DrawButton = [&](Sprite* sp, const XMFLOAT3& pos, const XMFLOAT3& size, bool isHover, bool blink)
		{
			if (!sp) return;

			// ホバー時は少し大きくする (1.1倍)
			float scale = isHover ? 1.1f : 1.0f;
			float w = size.x * scale;
			float h = size.y * scale;

			// 中心を基準に拡大するための座標補正
			float x = pos.x - (w - size.x) * 0.5f;
			float y = pos.y - (h - size.y) * 0.5f;

			// アルファ値（ホバー時は点滅させず、常時表示）
			float alpha = (blink && !isHover) ? render_color.w : 1.0f;

			sp->Render(rc, x, y, pos.z, w, h, 0, 1, 1, 1, alpha);
		};

	// ボタンを描画
	DrawButton(spriteStart, startButtonPos, startButtonSize, isStartHover, true);       // Startは点滅あり
	DrawButton(spriteTutorial, tutorialButtonPos, tutorialButtonSize, isTutorialHover, false); // Tutorialは点滅なし

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