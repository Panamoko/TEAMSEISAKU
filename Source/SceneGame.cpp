#include "System/Graphics.h"
#include "System/Input.h"
#include "SceneGame.h"
#include "Camera.h"
#include <imgui.h>
#include "EnemyManager.h"
#include "EnemySlime.h"
#include "Player.h"
#include "AllySlime.h"         // 既存＝直線弾
#include "AllySlimeHoming.h"   // 新規＝追尾弾
#include <cfloat>          // ★ FLT_MAX 用
#include "System/Mouse.h"  // ★ Mouse::BTN_LEFT / GetX()/GetY() を使うなら明示的に
#include "CollisionManager.h"
#include "GimmicManager.h"
#include "StageManager.h"
#include "GameObjectManager.h"
#include <cmath>
#include <DirectXMath.h>
#include <algorithm>
static constexpr int kMaxAlliesPerPlayer = 5;
using namespace DirectX;
#include "SceneManager.h"
#include "SceneTitle.h"
#include "SceneLoading.h"

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

	Serializer::LoadScene(objects, sprites2d, "scene.json");

	 grid_map.Initialize(101, 101, 1.1);
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
		cameraController->Update(elapsedTime);
	}

	Player::UpdateActiveByKeyboard(players);

	if (game_editor.PlayGame())
	{
		//ステージ更新処理
		stage->Update(scaledElapsedTime);

		StageManager::Instance().Update(scaledElapsedTime);

		// 全プレイヤー更新（入力は Player 側で“アクティブのみ”にガード）
		for (auto& up : players) up->Update(scaledElapsedTime);

		//エネミー更新処理
		EnemyManager::Instance().Update(scaledElapsedTime);

		//ギミック更新処理
		GimmicManager::Instance().Update(scaledElapsedTime);

		// 味方スライム更新
		for (auto& a : alliesStraight) a->Update(scaledElapsedTime);
		for (auto& a : alliesHoming)  a->Update(scaledElapsedTime);

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
	}
	// === 直線・追尾をそれぞれ更新 ===


}

// 描画処理
void SceneGame::Render()
{
	Graphics& graphics = Graphics::Instance();
	ID3D11DeviceContext* dc = graphics.GetDeviceContext();
	ShapeRenderer* shapeRenderer = graphics.GetShapeRenderer();
	ModelRenderer* modelRenderer = graphics.GetModelRenderer();
	// 描画準備
	RenderContext rc;
	rc.deviceContext = dc;
	rc.lightDirection = { 0.0f, -1.0f, 0.0f };	// ライト方向（下方向）
	rc.renderState = graphics.GetRenderState();

	// カメラパラメータ設定
	Camera& camera = Camera::Instance();
	rc.view       = camera.GetView();
	rc.projection = camera.GetProjection();

#if 1
	game_editor.render(objects, sprites2d, ModelManager::Instance().GetModels(), modelRenderer);
#endif
	// 3Dモデル描画
	{
		StageManager::Instance().Render(rc, modelRenderer);

		//ステージ描画
		//stage->Render(rc, modelRenderer);

		// 全プレイヤー描画
		for (auto& up : players) up->Render(rc, modelRenderer);

		// 味方スライム


		for (auto& a : alliesStraight) a->Render(rc, modelRenderer);
		for (auto& a : alliesHoming)  a->Render(rc, modelRenderer);

		// エネミー描画
		EnemyManager::Instance().Render(rc, modelRenderer);

		//BuildingManager::Instance().Render(rc, modelRenderer);

		//ギミック描画
		GimmicManager::Instance().Render(rc, modelRenderer);		

	}

	// 3Dデバッグ描画
	{
        // 全プレイヤーのデバッグ描画（選択リングは Player 側でアクティブ時のみ表示）
        for (auto& up : players) up->RenderDebugPrimitive(rc, shapeRenderer);

		//エネミーデバッグプリミティブ描画
		EnemyManager::Instance().RenderDebugPrimitive(rc, shapeRenderer);
		for (auto& a : allies) 
		{
	        a->RenderDebugPrimitive(rc, shapeRenderer);
	    }

		for (auto& obj : objects)
		{
			obj->RenderDebugPrimitive(rc, shapeRenderer);
		}
		for (auto& up : players) {
			up->RenderDebugPrimitive(rc, shapeRenderer); // 既存
			// ★ 追加：簡易にここでも描ける
			auto* p = up.get();
			shapeRenderer->RenderCylinder(
				rc, p->GetPosition(), 1.2f, 0.05f, DirectX::XMFLOAT4(1, 1, 0, 0.5f));
		}
		//BuildingManager::Instance().DebugDraw(rc, shapeRenderer);
		//grid_map.RenderDebugPrimitive(rc, shapeRenderer);
	}

	// 2Dスプライト描画
	{

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
	auto p = std::make_unique<AllySlimeHoming>(slot); // 新規クラス＝追尾弾
	p->SetLeader(leader);
	alliesHoming.emplace_back(std::move(p));
}

int SceneGame::CountAlliesGlobal() const
{
    return static_cast<int>(alliesStraight.size() + alliesHoming.size());
}