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
#include <cmath>
#include <DirectXMath.h>
using namespace DirectX;

#include "Editor.h"
static float gSelectPixelRadius = 120.0f; // ←好みで 80～160 に調整可
static D3D11_VIEWPORT gPickViewport = { 0,0,0,0,0,1 };

// 画面→ビューポートの情報を取得
static D3D11_VIEWPORT GetMainViewport()
{
	D3D11_VIEWPORT vp{};
	UINT n = 1;
	Graphics::Instance().GetDeviceContext()->RSGetViewports(&n, &vp);
	return vp;
}
static bool WorldToViewportPixel(const DirectX::XMFLOAT3& world, float& outX, float& outY)
{
	Camera& cam = Camera::Instance();

	XMMATRIX view = XMLoadFloat4x4(&cam.GetView());
	XMMATRIX proj = XMLoadFloat4x4(&cam.GetProjection());
	XMVECTOR p = XMLoadFloat3(&world);

	XMVECTOR clip = XMVector4Transform(XMVectorSetW(p, 1.0f), XMMatrixMultiply(view, proj));
	float cx = XMVectorGetX(clip);
	float cy = XMVectorGetY(clip);
	float cw = XMVectorGetW(clip);
	if (cw <= 0.0f) return false;

	float ndcX = cx / cw;
	float ndcY = cy / cw;

	// ★ キャッシュしたビューポートでピクセル化（TopLeftX/Yを必ず考慮）
	outX = gPickViewport.TopLeftX + (ndcX * 0.5f + 0.5f) * gPickViewport.Width;
	outY = gPickViewport.TopLeftY + (1.0f - (ndcY * 0.5f + 0.5f)) * gPickViewport.Height;
	return true;
}

// マウス位置(px)からの距離が gSelectPixelRadius 未満の Player を最短距離で選ぶ
// players は SceneGame 内のプレイヤー配列を想定
static Player* PickPlayerByScreenCircle(float mouseX, float mouseY,const std::vector<std::unique_ptr<Player>>& players)
{
	Player* best = nullptr;
	float bestDist2 = FLT_MAX;

	for (auto& up : players)
	{
		Player* p = up.get();
		DirectX::XMFLOAT3 pos = p->GetPosition();
		// pos.y += 0.8f; // 必要ならクリックしやすい高さへ

		float sx, sy;
		if (!WorldToViewportPixel(pos, sx, sy)) continue;

		float dx = sx - mouseX;
		float dy = sy - mouseY;
		float d2 = dx * dx + dy * dy;
		if (d2 <= gSelectPixelRadius * gSelectPixelRadius && d2 < bestDist2)
		{
			bestDist2 = d2;
			best = p;
		}
	}
	return best;
}
static void CapturePickViewportFromRS()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	UINT n = 1;
	D3D11_VIEWPORT vp{};
	dc->RSGetViewports(&n, &vp);
	if (n == 1 && vp.Width > 0.0f && vp.Height > 0.0f) {
		gPickViewport = vp;
	}
	else {
		// フォールバック：画面サイズを使用
		gPickViewport.TopLeftX = 0.0f;
		gPickViewport.TopLeftY = 0.0f;
		gPickViewport.Width = (float)Graphics::Instance().GetScreenWidth();
		gPickViewport.Height = (float)Graphics::Instance().GetScreenHeight();
		gPickViewport.MinDepth = 0.0f;
		gPickViewport.MaxDepth = 1.0f;
	}
}
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

	 BuildingManager::Instance().Initialize();

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
	CapturePickViewportFromRS();
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

		BuildingManager::Instance().Render();

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

	// HP を簡易表示するなら（ImGui 等）：
 ImGui::Begin("TownHall");
 if (auto th = BuildingManager::Instance().GetTownHall()) {
 float ratio = (float)th->GetHP() / (float)th->GetMaxHP();
 ImGui::ProgressBar(ratio, ImVec2(200, 16));
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