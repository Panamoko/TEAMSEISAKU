#include "System/Graphics.h"
#include "SceneGame.h"
#include "Camera.h"
#include <imgui.h>
#include "EnemyManager.h"
#include "EnemySlime.h"
#include "Player.h"
#include <cmath>
#include <cfloat> 
#include <DirectXMath.h>
using namespace DirectX;



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
	players[0]->SetPosition(DirectX::XMFLOAT3(-2, 0, 0));
	players[1]->SetPosition(DirectX::XMFLOAT3(2, 0, 0));

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
	const float aspect =static_cast<float>(graphics.GetScreenWidth()) /static_cast<float>(graphics.GetScreenHeight());
	camera.SetPerspectiveFov(

		DirectX::XMConvertToRadians(45),
		aspect,
		0.1f,
		1000.0f
	);

	// エネミー初期化
	EnemyManager& enemyManager = EnemyManager::Instance();

	for (int i = 0; i < 2; ++i)
	{
		EnemySlime* slime = new EnemySlime();
		slime->SetPosition(DirectX::XMFLOAT3(i * 2.0f, 0, 5));
		slime->SetTerritory(slime->GetPosition(), 10.0f);
		enemyManager.Register(slime);
	}
	 // 味方スライム初期化（ピクミン風フォロワー）
     const int kFollowerCount = 4; // 体数はお好みで
     allies.reserve(kFollowerCount);
     for (int i = 0; i < kFollowerCount; ++i) {
         auto s = std::make_unique<AllySlime>(i);
         // 初期はプレイヤーのやや後方に配置（Updateで整列追従する）
         auto p = Player::Instance().GetPosition();                           // :contentReference[oaicite:14]{index=14}
         s->SetPosition({ p.x, p.y, p.z - 1.0f - 0.3f * i });
         allies.emplace_back(std::move(s));
     }

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

	if (game_editor.PlayGame())
	{
		//ステージ更新処理
		stage->Update(elapsedTime);

		// 全プレイヤー更新（入力は Player 側で“アクティブのみ”にガード）
		for (auto& up : players) up->Update(elapsedTime);

		//エネミー更新処理
		EnemyManager::Instance().Update(elapsedTime);

		//エディタモデル更新
		for (auto& obj : objects)
		{
			obj->Update(elapsedTime);
		}

		// 味方スライム更新
		for (auto& a : allies) {
			a->Update(elapsedTime);
		}
	}
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

	game_editor.render(objects, sprites2d, ModelManager::Instance().GetModels(), modelRenderer);

	// 3Dモデル描画
	{
		//ステージ描画
		stage->Render(rc, modelRenderer);

		// 全プレイヤー描画
		for (auto& up : players) up->Render(rc, modelRenderer);

		// 味方スライム
	    for (auto& a : allies) 
		{
	        a->Render(rc, modelRenderer);
	    }

		//エディタモデル描画
		for (auto& obj : objects)
		{
			obj->Render(rc, modelRenderer);
		}

		// エネミー描画
		EnemyManager::Instance().Render(rc, modelRenderer);

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
