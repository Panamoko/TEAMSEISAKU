#include "System/Graphics.h"
#include "System/Input.h"
#include "SceneGame.h"
#include "Camera.h"
#include <imgui.h>
#include "EnemyManager.h"
#include "EnemySlime.h"
#include "Player.h"
#include "BuildingManager.h"
#include <cfloat>          // ★ FLT_MAX 用
#include "System/Mouse.h"  // ★ Mouse::BTN_LEFT / GetX()/GetY() を使うなら明示的に
#include "CollisionManager.h"
#include "GimmicManager.h"
#include "StageManager.h"
#include <cmath>
#include <DirectXMath.h>
using namespace DirectX;

#include "Editor.h"


// 初期化
void SceneGame::Initialize()
{
	//ステージ初期化
	stage = new Stage();
	//カメラコントローラー初期化
	cameraController = new CameraController();


	// 2体のプレイヤーを生成
	players.emplace_back(std::make_unique<Player>());
	players.emplace_back(std::make_unique<Player>());
	players[0]->Initialize();
	players[1]->Initialize();

	// 配置
	players[0]->SetPosition(DirectX::XMFLOAT3(-2, 0, -5));
	players[1]->SetPosition(DirectX::XMFLOAT3(2, 0, -5));

	// 最初は 0 番をアクティブに
	Player::SetActive(players[0].get());

	// カメラ初期設定
	Graphics& graphics = Graphics::Instance();
	Camera& camera = Camera::Instance();
	camera.SetLookAt(
		DirectX::XMFLOAT3(0, 15, -15),	//視点 (例: Y=15, Z=-15)
		DirectX::XMFLOAT3(0, 0, 0),		//注視点 (例: 原点)
		DirectX::XMFLOAT3(0, 1, 0)		//上方向
	);
	camera.SetPerspectiveFov(
		DirectX::XMConvertToRadians(45),//視野角
		graphics.GetScreenWidth() / graphics.GetScreenHeight(),//アスペクト比
		0.1f,	//クリップ距離（近）
		1000.0f	//クリップ距離（遠）
	);

	// エネミー初期化
	EnemyManager& enemyManager = EnemyManager::Instance();

	for (int i = 0; i < 2; ++i)
	{
		EnemySlime* slime = new EnemySlime();
		slime->SetPosition(DirectX::XMFLOAT3(i * 2.0f, 0, -5));
		slime->SetTerritory(slime->GetPosition(), 10.0f);
		//enemyManager.Register(slime);
	}

	 BuildingManager::Instance().Initialize();

	 BuildingManager& bm = BuildingManager::Instance();

	 // 例：(3, 0, 3) の位置に柵を1つ生成
	 bm.SpawnFence(DirectX::XMFLOAT3(3.0f, 0, 10.0f), 1.0f, 1.2f, /*maxHP=*/150, DirectX::XM_PIDIV2);

	 // 例：(-3, 0, 3) の位置に柵を1つ生成
	 bm.SpawnFence(DirectX::XMFLOAT3(-3.0f, 0, 10.0f));

	 // 例：(3, 0, 5) の位置に別の設定で生成 (半径とHPを変更)
	 //bm.SpawnFence(DirectX::XMFLOAT3(6.0f, 0, 10.0f), 1.0f, 1.2f, /*maxHP=*/150);

	 LoadScene(objects, sprites2d, "scene.json");
}

// 終了化
void SceneGame::Finalize()
{
	//エネミーの終了化
	EnemyManager::Instance().Clear();

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
	//カメラコントローラー更新処理
	DirectX::XMFLOAT3 target = Player::Instance().GetPosition();
	target.y += 0.5f;
	cameraController->SetTarget(target);
	cameraController->Update(elapsedTime);

	BuildingManager::Instance().Update(elapsedTime);

	if (auto th = BuildingManager::Instance().GetTownHall(); th && th->IsDestroyed()) {
		// TODO: ここでゲームクリア遷移
		// FadeManager::LoadScene(SceneType::GameClear);
		// あるいはフラグを立てて UI 表示 → 入力で遷移
	}

	if (game_editor.PlayGame())
	{
		CollisionManager::Instance().Clear();
		for (auto& obj : objects)
		{
			CollisionManager::Instance().AddObject(obj.get());
		}

		//ステージ更新処理
		stage->Update(elapsedTime);

		StageManager::Instance().Update(elapsedTime);

		// 全プレイヤー更新（入力は Player 側で“アクティブのみ”にガード）
		for (auto& up : players) up->Update(elapsedTime);

		//エネミー更新処理
		EnemyManager::Instance().Update(elapsedTime);

		//ギミック更新処理
		GimmicManager::Instance().Update(elapsedTime);

		// 味方スライム更新
		for (auto& a : allies) {
			a->Update(elapsedTime);
		}

		CollisionManager::Instance().CheckAllCollision();
	}
	Player::UpdateSelectionFromMouse(players, 120.0f);

	// ※ Yボタンで Ally を追加する処理は Scene 側の責務なのでそのまま残す:
	GamePad& gamePad = Input::Instance().GetGamePad();
	if (gamePad.GetButtonDown() & GamePad::BTN_Y) {
		AddAllyFor(Player::GetActivePtr());
	}

}

// 描画処理
void SceneGame::Render()
{
	Graphics& graphics = Graphics::Instance();
	ID3D11DeviceContext* dc = graphics.GetDeviceContext();
	Player::CapturePickViewportFromRS();
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

	game_editor.render(objects, sprites2d, ModelManager::Instance().GetModels(), modelRenderer);

	// 3Dモデル描画
	{
		StageManager::Instance().Render(rc, modelRenderer);

		//ステージ描画
		//stage->Render(rc, modelRenderer);

		// 全プレイヤー描画
		for (auto& up : players) up->Render(rc, modelRenderer);

		// 味方スライム
	    for (auto& a : allies) 
		{
	        a->Render(rc, modelRenderer);
	    }

		////エディタモデル描画
		//for (auto& obj : objects)
		//{
		//	obj->Render(rc, modelRenderer);
		//}

		// エネミー描画
		EnemyManager::Instance().Render(rc, modelRenderer);

		BuildingManager::Instance().Render(rc, modelRenderer);

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
		BuildingManager::Instance().DebugDraw(rc, shapeRenderer);


	}

	// 2Dスプライト描画
	{

	}
}



// GUI描画
void SceneGame::DrawGUI()
{
	//Player::Instance().DrawDebugGUI();

	// === 建物ステータスウィンドウ ===
	ImGui::Begin("Buildings Status");

	// --- タウンホールのHP ---
	if (auto th = BuildingManager::Instance().GetTownHall())
	{
		ImGui::Text("TownHall HP:");
		int currentHP = th->GetHP();
		int maxHP = th->GetMaxHP();
		float ratio = (maxHP > 0) ? ((float)currentHP / (float)maxHP) : 0.0f;

		// HPを "1300/1500" のようにテキストで表示
		std::string hp_text = std::to_string(currentHP) + "/" + std::to_string(maxHP);
		ImGui::ProgressBar(ratio, ImVec2(200, 16), hp_text.c_str());
	}

	ImGui::Separator(); // 区切り線

	// --- 柵のHP ---
	BuildingManager& bm = BuildingManager::Instance();
	int fenceCount = bm.GetFenceCount(); // 柵の総数を取得
	if (fenceCount > 0)
	{
		ImGui::Text("Fences HP:");
		// 登録されている全ての柵をループ
		for (int i = 0; i < fenceCount; ++i)
		{
			Fence* f = bm.GetFence(i); // i番目の柵を取得
			if (!f || !f->IsAlive()) continue; // 取得失敗か、すでにHP 0 ならスキップ

			int currentHP = f->GetHP(); // ステップ1で追加した関数
			int maxHP = f->GetMaxHP();  // ステップ1で追加した関数
			float ratio = (maxHP > 0) ? ((float)currentHP / (float)maxHP) : 0.0f;

			// ラベル（"Fence 0: 150/200" のように表示）
			std::string label = "Fence " + std::to_string(i) + ": " +
				std::to_string(currentHP) + "/" + std::to_string(maxHP);

			// 各プログレスバーにユニークIDを設定 (ImGuiのお作法)
			ImGui::PushID(f);
			ImGui::ProgressBar(ratio, ImVec2(180, 16), label.c_str());
			ImGui::PopID();
		}
	}

 Player::DebugDrawSelectionOverlay(players, /*pixelRadius=*/120.0f, /*highlightActive=*/true);
 ImGui::End();
}

int SceneGame::CountAlliesFor(Player* leader) const
{
	int n = 0;
	for (auto& a : allies) if (a->GetLeader() == leader) ++n;
	return n;
}

void SceneGame::AddAllyFor(Player* leader)
{
	if (!leader) return;

	// 5体上限
	int idx = CountAlliesFor(leader);
	if (idx >= 4) return;

	// 編隊スロット = いまの数（0..4）
	auto s = std::make_unique<AllySlime>(idx);
	s->SetLeader(leader);

	// 生成直後はリーダーのちょい後ろに置く（UpdateAnchorで整列していく）
	const auto p = leader->GetPosition();
	s->SetPosition({ p.x, p.y, p.z - 1.2f - 0.3f * idx });

	allies.emplace_back(std::move(s));
}