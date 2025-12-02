#include "System/Graphics.h"
#include "System/Input.h"
#include "SceneGame.h"
#include "Camera.h"
#include <imgui.h>
#include "EnemyManager.h"
#include "EnemySlime.h"
#include "Player.h"
#include "Core.h"
#include "AllySlime.h"         // 既存＝直線弾
#include "AllySlimeHeal.h"   // 新規＝追尾弾
#include <cfloat>          // ★ FLT_MAX 用
#include "System/Mouse.h"  // ★ Mouse::BTN_LEFT / GetX()/GetY() を使うなら明示的に
#include "CollisionManager.h"
#include "GimmicManager.h"
#include "StageManager.h"
#include "GameObjectManager.h"
#include <cmath>
#include <DirectXMath.h>
#include <algorithm>
static constexpr int kMaxAlliesPerPlayer = 6;
using namespace DirectX;
#include "SceneManager.h"
#include "SceneTitle.h"
#include "SceneLoading.h"
#include "SceneFactory.h"

float SceneGame::s_timeScale = 1.0f;
float SceneGame::s_slowTimer = 0.0f;

// スロー設定
void SceneGame::SetSlowMotion(float scale, float duration)
{
	s_timeScale = scale;
	s_slowTimer = duration;
}

SceneGame::~SceneGame() = default;  // ★これを追加
// 初期化
void SceneGame::Initialize()
{
	//ステージ初期化
	stage = new Stage();
	//カメラコントローラー初期化
	cameraController = new CameraController();



	// 最初は 0 番をアクティブに
	Player::SetActive(nullptr);

	// カメラ初期設定
	Graphics& graphics = Graphics::Instance();
	Camera& camera = Camera::Instance();

	camera.SetQuarterView(
		DirectX::XMFLOAT3(0, 0, 0), // 注視点 (Focus)
		0.0f,                       // Yaw (0度 = 手前側から奥を見る)
		45.0f,                      // Pitch (既存の 15, -15 の角度に合わせるなら45度)
		75.0f                       // Distance (CameraController::maxDistance と同じ値)
	);

	camera.SetPerspectiveFov(
		DirectX::XMConvertToRadians(45),//視野角
		graphics.GetScreenWidth() / graphics.GetScreenHeight(),//アスペクト比
		0.1f,	//クリップ距離（近）
		1000.0f	//クリップ距離（遠）
	);

	// エネミー初期化
	EnemyManager& enemyManager = EnemyManager::Instance();

	GimmicManager::Instance().GetAll();

	Scene* currentScene = SceneManager::Instance().GetCurrentScene();
	currentScene->SetSceneName(scene_name);
	std::string scene_file_name = "JSON /" + scene_name + ".json";

	if (currentScene)
	{
		// アクティブなシーンがあれば、その名前をファイル名として使用
		scene_file_name = "JSON/" + currentScene->GetSceneName() + ".json";
	}

	Serializer::LoadScene(objects, sprites2d, scene_file_name);

	// ★追加: レンダーターゲット作成 (解像度はメイン画面と同じか、少し落としても良い)
	pipRenderTarget = new RenderTarget(1280, 720);

	// PiP表示用の枠やダミーSpriteを用意（Render関数でRenderTargetのテクスチャを使って描画するため、中身は適当でOK）
	// ここではSpriteの機能だけ借りたいのでダミーロード
	pipFrameSprite = SpriteManager::Instance().Load("Data/Sprite/Window.png");

	grid_map.Initialize(150, 150, 0.8f);
}

// 終了化
void SceneGame::Finalize()
{
	//エネミーの終了化
	EnemyManager::Instance().Clear();

	CollisionManager::Instance().Clear();

	GimmicManager::Instance().Clear();

	GameObjectManager::Instance().Clear();

	//カメラコントローラー終了化
	if (cameraController != nullptr)
	{
		delete cameraController;
		cameraController = nullptr;
	}

	for (auto& up : players)
	{
		up->Finalize();
	}
	players.clear();

	//ステージ終了化
	if (stage != nullptr)
	{
		delete stage;
		stage = nullptr;
	}

	if (pipRenderTarget) delete pipRenderTarget;
}

// 更新処理
void SceneGame::Update(float elapsedTime)
{
	// スロータイマーの更新
	if (s_slowTimer > 0.0f)
	{
		s_slowTimer -= elapsedTime;
		if (s_slowTimer <= 0.0f)
		{
			s_timeScale = 1.0f; // 時間経過でスロー解除
		}
	}

	// オブジェクト更新に使うスケーリング済み時間
	float scaledElapsedTime = elapsedTime * s_timeScale;

	pickingRay.Update(); // レイ情報の更新

	//Playerクラスの関数に丸投げするだけ
	Player::UpdateSpawn(players, pickingRay);

	//カメラコントローラー更新処理
	if (Player::GetActivePtr() != nullptr)
	{
		DirectX::XMFLOAT3 target = Player::Instance().GetPosition();
		target.y += 0.5f;
		cameraController->SetTarget(target);
	}

	cameraController->Update(elapsedTime);

	Player::UpdateActiveByKeyboard(players);

	if (game_editor.PlayGame())
	{
		//ステージ更新処理
		stage->Update(scaledElapsedTime);

		StageManager::Instance().Update(scaledElapsedTime);

		// 全プレイヤーにマップ情報を渡す
		for (auto& up : players)
		{
			up->SetGridMap(&grid_map);
		}

		// エネミーにもマップ情報を渡す
		EnemyManager& em = EnemyManager::Instance();
		int enemyCount = em.GetEnemyCount();
		for (int i = 0; i < enemyCount; ++i)
		{
			auto enemy = em.GetEnemy(i);
			// EnemySlime型（またはその派生）であればキャストしてセット
			if (auto slime = std::dynamic_pointer_cast<EnemySlime>(enemy))
			{
				slime->SetGridMap(&grid_map);
			}
		}

		// 全プレイヤー更新（入力は Player 側で“アクティブのみ”にガード）
		for (auto& up : players) up->Update(scaledElapsedTime);

		//エネミー更新処理
		EnemyManager::Instance().Update(scaledElapsedTime);

		//ギミック更新処理
		GimmicManager::Instance().Update(scaledElapsedTime);

		// 味方スライム更新
		for (auto& a : alliesStraight) a->Update(scaledElapsedTime);
		for (auto& a : alliesHoming)  a->Update(scaledElapsedTime);
		for (auto& a : alliesMelee)   a->Update(scaledElapsedTime);

		CollisionManager::Instance().CheckAllCollision();

		grid_map.Build(GameObjectManager::Instance().GetAllObjects());
	}

	// === 追加: C/Vでスポーン ===
	auto& gp = Input::Instance().GetGamePad();
	auto down = gp.GetButtonDown();
	Player* active = Player::GetActivePtr(); // あなたのAPIに合わせて

	if (active) {
		if (down & GamePad::BTN_X) {      // Cキー（エミュ）＝直線弾
			AddAllyStraightFor(active);
		}
		if (down & GamePad::BTN_Y) {      // Vキー（エミュ）＝追尾弾
			AddAllyHomingFor(active);
		}
		if (down & GamePad::BTN_B) {
			AddAllyMeleeFor(active);
		}
	}
	// ★変更: PiPのクリック判定 (左端配置 & スライド式)
	Mouse& mouse = Input::Instance().GetMouse();
	if (mouse.GetButtonDown() & Mouse::BTN_LEFT)
	{
		float mx = (float)mouse.GetPositionX();
		float my = (float)mouse.GetPositionY();

		// PiPのサイズ設定
		// 常に一定サイズで表示し、出し入れだけを行う形にします
		float pipW = 400.0f; // ウィンドウ全体の幅
		float pipH = 225.0f; // 高さ
		float tabW = 60.0f;  // 矢印（タブ）部分の幅（クリック判定用）

		// 表示位置（X座標）の計算
		// Expanded(開): 0.0f (左端にピッタリ)
		// Collapsed(閉): -pipW + tabW (タブだけ残して画面外へ)
		float currentPipX = isPipExpanded ? 0.0f : (-pipW + tabW);
		float currentPipY = 100.0f; // 上から少し下げた位置

		// クリック判定エリア
		// 画像の「右端のタブ部分」をクリックしたらトグルする
		// 判定エリア： (ウィンドウ右端 - タブ幅) ～ ウィンドウ右端
		float clickAreaLeft = currentPipX + pipW - tabW;
		float clickAreaRight = currentPipX + pipW;
		float clickAreaTop = currentPipY;
		float clickAreaBottom = currentPipY + pipH;

		if (mx >= clickAreaLeft && mx < clickAreaRight &&
			my >= clickAreaTop && my < clickAreaBottom)
		{
			isPipExpanded = !isPipExpanded;
		}
	}


}

// 描画処理
void SceneGame::Render()
{
	Graphics& graphics = Graphics::Instance();
	ID3D11DeviceContext* dc = graphics.GetDeviceContext();
	ModelRenderer* modelRenderer = graphics.GetModelRenderer();
	Camera& camera = Camera::Instance();
	ShapeRenderer* shapeRenderer = graphics.GetShapeRenderer(); // デバッグ描画用

	game_editor.render(objects, sprites2d, ModelManager::Instance().GetModels(), modelRenderer);

	// ---------------------------------------------------
	// 1. PiP画面（UIのみ）の描画
	// ---------------------------------------------------
	// ★変更: 関数呼び出しだけになりスッキリします
	RenderPiP(dc);

	// ---------------------------------------------------
	// 2. メイン画面の描画
	// ---------------------------------------------------

	// 描画準備
	RenderContext rc;
	rc.deviceContext = dc;
	rc.lightDirection = { 0.0f, -1.0f, 0.0f };
	rc.renderState = graphics.GetRenderState();
	rc.view = camera.GetView();
	rc.projection = camera.GetProjection();
	game_editor.render(objects, sprites2d, ModelManager::Instance().GetModels(), modelRenderer);
	modelRenderer->BeginFrame(rc);

	// 3Dモデル描画
	{
		StageManager::Instance().Render(rc, modelRenderer);
		for (auto& up : players) up->Render(rc, modelRenderer);

		// 味方スライム描画
		for (auto& a : alliesStraight) a->Render(rc, modelRenderer);
		for (auto& a : alliesHoming)   a->Render(rc, modelRenderer);
		for (auto& a : alliesMelee)    a->Render(rc, modelRenderer);

		EnemyManager::Instance().Render(rc, modelRenderer);
		GimmicManager::Instance().Render(rc, modelRenderer);
	}

	// 3Dデバッグ描画
	{
		for (auto& up : players) up->RenderDebugPrimitive(rc, shapeRenderer);
		EnemyManager::Instance().RenderDebugPrimitive(rc, shapeRenderer);
		// 必要に応じて他のデバッグ描画を追加
		GimmicManager::Instance().RenderDebugPrimitive(rc, shapeRenderer);
	}

	// 2Dスプライト描画 (PiPウィンドウ自体の描画含む)
	{
		for (const auto& game_sprite : this->sprites2d)
		{
			if (game_sprite) game_sprite->Render();
		}

		// PiPウィンドウ（左側・スライド式）の描画
		if (pipFrameSprite && pipRenderTarget)
		{
			// ... (ここはウィンドウ枠と、RenderPiPで作ったテクスチャを表示する処理なので残す) ...
			// ※ 長くなるようならここも `RenderPiPWindow(rc)` のように関数化できます

			float pipW = 400.0f;
			float pipH = 225.0f;
			float tabW = 60.0f;

			float pipX = isPipExpanded ? 0.0f : (-pipW + tabW);
			float pipY = 100.0f;

			// 枠の描画
			pipFrameSprite->Render(rc, pipX, pipY, 0.0f, pipW, pipH, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f);

			// 中身（RenderPiPで描いた映像）の描画
			float margin = 10.0f;
			float marginRight = tabW + margin;

			// RenderTextureを使って描画
			pipFrameSprite->RenderTexture(
				rc,
				pipRenderTarget->GetSRV(),
				pipX + margin, pipY + margin,
				pipW - (margin + marginRight), pipH - (margin * 2.0f)
			);
		}
	}
}

// GUI描画
void SceneGame::DrawGUI()
{
	//Player::Instance().DrawDebugGUI();


}

int SceneGame::CountAlliesFor(Player* leader) const
{
	int n = 0;
	for (auto& a : alliesStraight) if (a->GetLeader() == leader) ++n;
	for (auto& a : alliesHoming)  if (a->GetLeader() == leader) ++n;
	for (auto& a : alliesMelee)   if (a->GetLeader() == leader) ++n;
	return n;
}

void SceneGame::AddAllyStraightFor(Player* leader)
{
	if (CountAlliesFor(leader) >= kMaxAlliesPerPlayer)
	{
		// （任意）ImGui::SetTooltip("Allies capped per player (5)");
		return;
	}
	int slot = CountAlliesFor(leader);
	auto p = std::make_unique<AllySlime>(slot); // 既存クラス＝直線弾
	p->SetLeader(leader);
	alliesStraight.emplace_back(std::move(p));
}

void SceneGame::AddAllyHomingFor(Player* leader)
{
	// 新:
	if (CountAlliesFor(leader) >= kMaxAlliesPerPlayer) {
		return;
	}
	int slot = CountAlliesFor(leader);
	auto p = std::make_unique<AllySlimeHeal>(slot); // 新規クラス＝追尾弾
	p->SetLeader(leader);
	alliesHoming.emplace_back(std::move(p));
}

void SceneGame::AddAllyMeleeFor(Player* leader)
{
	if (CountAlliesFor(leader) >= kMaxAlliesPerPlayer)
	{
		return;
	}
	int slot = CountAlliesFor(leader);
	auto p = std::make_unique<AllySlimeMelee>(slot);
	p->SetLeader(leader);
	alliesMelee.emplace_back(std::move(p));
}

int SceneGame::CountAlliesGlobal() const
{
	return static_cast<int>(alliesStraight.size() + alliesHoming.size() + alliesMelee.size());
}

void SceneGame::RenderPiP(ID3D11DeviceContext* dc)
{
	// レンダーターゲットがなければ何もしない
	if (!pipRenderTarget) return;

	Graphics& graphics = Graphics::Instance();

	// 1. レンダーターゲット有効化
	pipRenderTarget->Clear(dc, 0.2f, 0.2f, 0.2f, 1.0f);
	pipRenderTarget->Activate(dc);

	// 2. 描画コンテキスト設定
	RenderContext rc;
	rc.deviceContext = dc;
	rc.lightDirection = { 0.0f, -1.0f, 0.0f };
	rc.renderState = graphics.GetRenderState();

	// 3. UI（アイコン）の描画
	float playerIconSize = 128.0f; // プレイヤー用
	float allyIconSize = 100.0f;    // 味方用
	float padding = 12.0f;         // 行間
	float startX = 20.0f;          // 左余白
	float startY = 20.0f;          // 上余白

	for (size_t i = 0; i < players.size(); ++i) {
		Player* pLeader = players[i].get();
		if (!pLeader) continue;

		// Y座標計算
		float currentY = startY + static_cast<float>(i) * (playerIconSize + padding);

		// (A) プレイヤー描画
		pLeader->RenderUI(rc, startX, currentY, playerIconSize);

		// (B) 従属する味方を描画
		float currentAllyX = startX + playerIconSize + 10.0f;
		float allyYOffset = (playerIconSize - allyIconSize) * 0.5f; // 高さ合わせ

		// 直進型
		for (auto& ally : alliesStraight) {
			if (ally && ally->GetLeader() == pLeader) {
				ally->RenderUI(rc, currentAllyX, currentY + allyYOffset, allyIconSize);
				currentAllyX += allyIconSize + 5.0f;
			}
		}
		// 追尾型 (Heal)
		for (auto& ally : alliesHoming) {
			if (ally && ally->GetLeader() == pLeader) {
				ally->RenderUI(rc, currentAllyX, currentY + allyYOffset, allyIconSize);
				currentAllyX += allyIconSize + 5.0f;
			}
		}
		// 近接型 (Melee)
		for (auto& ally : alliesMelee) {
			if (ally && ally->GetLeader() == pLeader) {
				ally->RenderUI(rc, currentAllyX, currentY + allyYOffset, allyIconSize);
				currentAllyX += allyIconSize + 5.0f;
			}
		}
	}

	// 4. レンダーターゲット解除（バックバッファに戻す）
	pipRenderTarget->Deactivate(dc);
}
REGISTER_SCENE(SceneGame);

