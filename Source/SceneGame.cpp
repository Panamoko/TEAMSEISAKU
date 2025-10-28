#include "System/Graphics.h"
#include "SceneGame.h"
#include "Camera.h"
#include <imgui.h>
#include "EnemyManager.h"
#include "EnemySlime.h"
#include "Player.h"
#include <cmath>
#include <DirectXMath.h>
using namespace DirectX;

#include "Editor.h"
// スクリーン座標からワールド空間のレイ（原点・方向）を作る
static void MakeMouseRay(float sx, float sy, XMFLOAT3& outOrigin, XMFLOAT3& outDir)
{
	Graphics& g = Graphics::Instance();
	Camera& cam = Camera::Instance();

	float W = g.GetScreenWidth();
	float H = g.GetScreenHeight();

	XMMATRIX view = DirectX::XMLoadFloat4x4(&cam.GetView());
	XMMATRIX proj = DirectX::XMLoadFloat4x4(&cam.GetProjection());
	XMMATRIX world = XMMatrixIdentity();

	XMVECTOR np = XMVector3Unproject(XMVectorSet(sx, sy, 0.0f, 1.0f), 0, 0, W, H, 0.0f, 1.0f, proj, view, world);
	XMVECTOR fp = XMVector3Unproject(XMVectorSet(sx, sy, 1.0f, 1.0f), 0, 0, W, H, 0.0f, 1.0f, proj, view, world);

	XMVECTOR dir = XMVector3Normalize(XMVectorSubtract(fp, np));
	XMStoreFloat3(&outOrigin, np);
	XMStoreFloat3(&outDir, dir);
}

// レイと縦円柱（XZ円＋高さ）の交差（true=ヒット）
static bool IntersectRayVsVerticalCylinder(
	const XMFLOAT3& ro, const XMFLOAT3& rd,
	const XMFLOAT3& base, float radius, float height,
	float* outT = nullptr)
{
	// XZ 平面のレイ vs 円
	float ox = ro.x - base.x, oz = ro.z - base.z;
	float dx = rd.x, dz = rd.z;

	float a = dx * dx + dz * dz;
	if (a < 1e-8f) {
		float dist2 = ox * ox + oz * oz;
		if (dist2 > radius * radius) return false;
		// y 到達チェック（上下方向）
		if (rd.y > 0 && ro.y > base.y + height) return false;
		if (rd.y < 0 && ro.y < base.y)         return false;
		if (outT) *outT = 0.0f;
		return true;
	}

	float b = 2.0f * (dx * ox + dz * oz);
	float c = ox * ox + oz * oz - radius * radius;
	float disc = b * b - 4 * a * c;
	if (disc < 0) return false;

	float t1 = (-b - std::sqrt(disc)) / (2 * a);
	float t2 = (-b + std::sqrt(disc)) / (2 * a);
	float t = (t1 >= 0) ? t1 : ((t2 >= 0) ? t2 : -1.0f);
	if (t < 0) return false;

	float y = ro.y + rd.y * t;
	if (y < base.y || y > base.y + height) return false;

	if (outT) *outT = t;
	return true;
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
	// --- 左クリックでアクティブ切替（ピッキング） ---
    {
        ImGuiIO& io = ImGui::GetIO();
        if (ImGui::IsMouseClicked(0) && !io.WantCaptureMouse) {
            XMFLOAT3 ro, rd;
            MakeMouseRay(io.MousePos.x, io.MousePos.y, ro, rd);

            // 一番手前を拾う：t の最小値を採用
            float bestT = FLT_MAX;
            Player* best = nullptr;
            for (auto& up : players) {
                Player* p = up.get();
                float t;
                if (IntersectRayVsVerticalCylinder(ro, rd, p->GetPosition(), p->GetRadius(), p->GetHeight(), &t)) {
                    if (t < bestT) { bestT = t; best = p; }
                }
            }
            if (best) Player::SetActive(best);
        }
    }
	//カメラコントローラー更新処理
	DirectX::XMFLOAT3 target = Player::Instance().GetPosition();
	target.y += 0.5f;
	cameraController->SetTarget(target);
	cameraController->Update(elapsedTime);

	//ステージ更新処理
	stage->Update(elapsedTime);

	// 全プレイヤー更新（入力は Player 側で“アクティブのみ”にガード）
	for (auto& up : players) up->Update(elapsedTime);

	//エネミー更新処理
	EnemyManager::Instance().Update(elapsedTime);

	for (auto& obj : objects)
	{
		obj->Update(elapsedTime);
	}

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
