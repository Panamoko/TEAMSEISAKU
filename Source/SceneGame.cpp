#include "System/Graphics.h"
#include "SceneGame.h"
#include "Camera.h"
#include <imgui.h>
#include "EnemyManager.h"
#include "EnemySlime.h"
#include "Player.h"


#include "Editor.h"

// 初期化
void SceneGame::Initialize()
{
	//ステージ初期化
	stage = new Stage();

	//プレイヤー初期化
	Player::Instance().Initialize();

	//カメラコントローラー初期化
	cameraController = new CameraController();


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

	Player::Instance().Finalize();

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

	//ステージ更新処理
	stage->Update(elapsedTime);

	//プレイヤー更新処理
	Player::Instance().Update(elapsedTime);

	//エネミー更新処理
	EnemyManager::Instance().Update(elapsedTime);

	// 味方スライム更新
    for (auto& a : allies) {
        a->Update(elapsedTime);
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
	rc.view = camera.GetView();
	rc.projection = camera.GetProjection();


	// 3Dモデル描画
	{
		//ステージ描画
		stage->Render(rc, modelRenderer);

		//プレイヤー描画
		Player::Instance().Render(rc, modelRenderer);

		// 味方スライム
	    for (auto& a : allies) 
		{
	        a->Render(rc, modelRenderer);
	    }

		// エネミー描画
		EnemyManager::Instance().Render(rc, modelRenderer);

	}

	// 3Dデバッグ描画
	{
		//プレイヤーデバッグプリミティブ描画
		Player::Instance().RenderDebugPrimitive(rc, shapeRenderer);

		//エネミーデバッグプリミティブ描画
		EnemyManager::Instance().RenderDebugPrimitive(rc, shapeRenderer);
		for (auto& a : allies) 
		{
	        a->RenderDebugPrimitive(rc, shapeRenderer);
	    }
	}

	// 2Dスプライト描画
	{

	}
}



// GUI描画
void SceneGame::DrawGUI()
{
	Player::Instance().DrawDebugGUI();
}
