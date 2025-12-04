#include "System/Graphics.h"
#include "System/Input.h"
#include "SceneGame.h"
#include "Camera.h"
#include <imgui.h>
#include "EnemyManager.h"
#include "EnemySlime.h"
#include "Player.h"
#include "Core.h"
#include "AllySlime.h"
#include "AllySlimeHeal.h"
#include <cfloat>
#include "System/Mouse.h"
#include "CollisionManager.h"
#include "GimmicManager.h"
#include "StageManager.h"
#include "GameObjectManager.h"
#include <cmath>
#include <DirectXMath.h>
#include <algorithm>
#include "SceneManager.h"
#include "SceneTitle.h"
#include "SceneLoading.h"
#include "SceneFactory.h"

// 派生クラスのインクルード
#include "PlayerMelee.h"
#include "PlayerHeal.h"
#include "PlayerShot.h"

static constexpr int kMaxAlliesPerPlayer = 6;
using namespace DirectX;

float SceneGame::s_timeScale = 1.0f;
float SceneGame::s_slowTimer = 0.0f;

void SceneGame::SetSlowMotion(float scale, float duration)
{
	s_timeScale = scale;
	s_slowTimer = duration;
}

SceneGame::~SceneGame() = default;

void SceneGame::Initialize()
{
	stage = new Stage();
	cameraController = new CameraController();

	Player::SetActive(nullptr);
	players.clear();

	Graphics& graphics = Graphics::Instance();
	Camera& camera = Camera::Instance();

	camera.SetQuarterView(
		DirectX::XMFLOAT3(0, 0, 0),
		0.0f,
		45.0f,
		75.0f
	);

	camera.SetPerspectiveFov(
		DirectX::XMConvertToRadians(45),
		graphics.GetScreenWidth() / graphics.GetScreenHeight(),
		0.1f,
		1000.0f
	);

	EnemyManager& enemyManager = EnemyManager::Instance();
	GimmicManager::Instance().GetAll();

	Scene* currentScene = SceneManager::Instance().GetCurrentScene();
	currentScene->SetSceneName(scene_name);
	std::string scene_file_name = "JSON/" + currentScene->GetSceneName() + ".json";

	Serializer::LoadScene(objects, sprites2d, scene_file_name);

	pipRenderTarget = new RenderTarget(1280, 720);
	pipFrameSprite = SpriteManager::Instance().Load("Data/Sprite/Window.png");

	grid_map.Initialize(150, 150, 0.8f);

	// ディゾルブの初期化と開始設定
	dissolve = std::make_unique<Dissolve>();
	dissolve->Initialize(Graphics::Instance().GetDevice(), "Data/Sprite/DissolveNoise.png");

	isSceneStarting = true; // フェードイン開始
	startTransitionTimer = startTransitionDuration;
}

void SceneGame::Finalize()
{
	EnemyManager::Instance().Clear();
	CollisionManager::Instance().Clear();
	GimmicManager::Instance().Clear();
	GameObjectManager::Instance().Clear();

	if (cameraController != nullptr)
	{
		delete cameraController;
		cameraController = nullptr;
	}

	players.clear();

	if (stage != nullptr)
	{
		delete stage;
		stage = nullptr;
	}

	if (pipRenderTarget) delete pipRenderTarget;
}

// unique_ptr用の削除関数（既存）
template <typename T>
void RemoveInactiveObjects(std::vector<std::unique_ptr<T>>& objects)
{
	auto it = std::remove_if(objects.begin(), objects.end(),
		[](const std::unique_ptr<T>& obj) {
			return !obj->IsActive();
		});
	objects.erase(it, objects.end());
}

// ★追加: shared_ptr用の削除関数 (エラー「識別子が見つかりません」の修正)
template <typename T>
void RemoveInactiveSharedObjects(std::vector<std::shared_ptr<T>>& objects)
{
	auto it = std::remove_if(objects.begin(), objects.end(),
		[](const std::shared_ptr<T>& obj) {
			return !obj->IsActive();
		});
	objects.erase(it, objects.end());
}

void SceneGame::Update(float elapsedTime)
{
	// 時間経過とともにタイマーを減らし、0になったらフラグを下ろす
	if (isSceneStarting)
	{
		startTransitionTimer -= elapsedTime;
		if (startTransitionTimer <= 0.0f)
		{
			startTransitionTimer = 0.0f;
			isSceneStarting = false;
		}
	}

	if (s_slowTimer > 0.0f)
	{
		s_slowTimer -= elapsedTime;
		if (s_slowTimer <= 0.0f) s_timeScale = 1.0f;
	}

	float scaledElapsedTime = elapsedTime * s_timeScale;

	if (Player::GetActivePtr() != nullptr)
	{
		DirectX::XMFLOAT3 target = Player::Instance().GetPosition();
		target.y += 0.5f;
		cameraController->SetTarget(target);
	}
	cameraController->Update(elapsedTime);

	// スポーン処理
	pickingRay.Update();
	UpdatePlayerSpawn();

	// キーボード切り替え
	Player::UpdateActiveByKeyboard(players);

	if (game_editor.PlayGame())
	{
		stage->Update(scaledElapsedTime);
		StageManager::Instance().Update(scaledElapsedTime);

		// マップ情報セット & 更新
		for (auto& character : players)
		{
			// Characterとして扱う（共通）
			// Player型にキャストしてマップをセット
			if (auto p = std::dynamic_pointer_cast<Player>(character)) {
				p->SetGridMap(&grid_map);
			}

			// 更新
			character->Update(scaledElapsedTime);

			// 自動スポーン処理 (Player型であれば)
			if (auto p = std::dynamic_pointer_cast<Player>(character))
			{
				if (p->UpdateAutoSpawn(scaledElapsedTime))
				{
					p->SpawnAlly(this);
				}
			}
		}

		EnemyManager& em = EnemyManager::Instance();
		int enemyCount = em.GetEnemyCount();
		for (int i = 0; i < enemyCount; ++i)
		{
			auto enemy = em.GetEnemy(i);
			if (auto slime = std::dynamic_pointer_cast<EnemySlime>(enemy))
			{
				slime->SetGridMap(&grid_map);
			}
		}

		EnemyManager::Instance().Update(scaledElapsedTime);
		GimmicManager::Instance().Update(scaledElapsedTime);

		for (auto& a : alliesStraight) a->Update(scaledElapsedTime);
		for (auto& a : alliesHoming)  a->Update(scaledElapsedTime);
		for (auto& a : alliesMelee)   a->Update(scaledElapsedTime);

		CollisionManager::Instance().CheckAllCollision();

		// ★修正: ここで定義した RemoveInactiveSharedObjects を呼ぶ
		RemoveInactiveSharedObjects(players);

		RemoveInactiveObjects(alliesStraight);
		RemoveInactiveObjects(alliesHoming);
		RemoveInactiveObjects(alliesMelee);
		grid_map.Build(GameObjectManager::Instance().GetAllObjects());
	}

	UpdatePiP();
}

void SceneGame::Render()
{
	Graphics& graphics = Graphics::Instance();
	ID3D11DeviceContext* dc = graphics.GetDeviceContext();
	ModelRenderer* modelRenderer = graphics.GetModelRenderer();
	Camera& camera = Camera::Instance();
	ShapeRenderer* shapeRenderer = graphics.GetShapeRenderer();

	game_editor.render(objects, sprites2d, ModelManager::Instance().GetModels(), modelRenderer);

	RenderPiP(dc);

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

		// プレイヤー描画
		for (auto& character : players) {
			// CharacterのRenderを呼ぶ（Playerでオーバーライドされている）
			character->Render(rc, modelRenderer);
		}

		// 味方スライム描画
		for (auto& a : alliesStraight) a->Render(rc, modelRenderer);
		for (auto& a : alliesHoming)   a->Render(rc, modelRenderer);
		for (auto& a : alliesMelee)    a->Render(rc, modelRenderer);

		EnemyManager::Instance().Render(rc, modelRenderer);
		GimmicManager::Instance().Render(rc, modelRenderer);
	}

	// 3Dデバッグ描画
	{
		for (auto& character : players) character->RenderDebugPrimitive(rc, shapeRenderer);
		EnemyManager::Instance().RenderDebugPrimitive(rc, shapeRenderer);
	}

	// 2Dスプライト描画
	{
		for (const auto& game_sprite : this->sprites2d)
		{
			if (game_sprite) game_sprite->Render();
		}

		if (pipFrameSprite && pipRenderTarget)
		{
			float pipW = 400.0f;
			float pipH = 225.0f;
			float tabW = 60.0f;

			float pipX = isPipExpanded ? 0.0f : (-pipW + tabW);
			float pipY = 100.0f;

			pipFrameSprite->Render(rc, pipX, pipY, 0.0f, pipW, pipH, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f);

			float margin = 10.0f;
			float marginRight = tabW + margin;

			pipFrameSprite->RenderTexture(
				rc,
				pipRenderTarget->GetSRV(),
				pipX + margin, pipY + margin,
				pipW - (margin + marginRight), pipH - (margin * 2.0f)
			);
		}

		if (isSceneStarting && dissolve)
		{
			// タイマーが Max(開始時) -> 0(終了時) へ減っていく
			// 閾値: 1.0(黒) -> 0.0(透明)
			float t = std::clamp(startTransitionTimer / startTransitionDuration, 0.0f, 1.0f);
			dissolve->Render(dc, t);
		}
	}
}

void SceneGame::DrawGUI()
{
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
	if (CountAlliesFor(leader) >= kMaxAlliesPerPlayer) return;
	int slot = CountAlliesFor(leader);

	// ★修正: コンストラクタに leader を渡す
	auto p = std::make_unique<AllySlime>(slot, leader);
	// p->SetLeader(leader); // コンストラクタで設定済みなので不要（残しても無害）

	alliesStraight.emplace_back(std::move(p));
}

void SceneGame::AddAllyHomingFor(Player* leader)
{
	if (CountAlliesFor(leader) >= kMaxAlliesPerPlayer) return;
	int slot = CountAlliesFor(leader);

	// ★修正
	auto p = std::make_unique<AllySlimeHeal>(slot, leader);

	alliesHoming.emplace_back(std::move(p));
}

void SceneGame::AddAllyMeleeFor(Player* leader)
{
	if (CountAlliesFor(leader) >= kMaxAlliesPerPlayer) return;
	int slot = CountAlliesFor(leader);

	// ★修正
	auto p = std::make_unique<AllySlimeMelee>(slot, leader);

	alliesMelee.emplace_back(std::move(p));
}
int SceneGame::CountAlliesGlobal() const
{
	return static_cast<int>(alliesStraight.size() + alliesHoming.size() + alliesMelee.size());
}

void SceneGame::RenderPiP(ID3D11DeviceContext* dc)
{
	if (!pipRenderTarget) return;

	Graphics& graphics = Graphics::Instance();

	pipRenderTarget->Clear(dc, 0.2f, 0.2f, 0.2f, 1.0f);
	pipRenderTarget->Activate(dc);

	RenderContext rc;
	rc.deviceContext = dc;
	rc.lightDirection = { 0.0f, -1.0f, 0.0f };
	rc.renderState = graphics.GetRenderState();

	float playerIconSize = 128.0f;
	float allyIconSize = 100.0f;
	float padding = 12.0f;
	float startX = 20.0f;
	float startY = 20.0f;

	for (size_t i = 0; i < players.size(); ++i) {
		// ★修正: players[i] は Character なので Player* にキャスト
		Player* pLeader = dynamic_cast<Player*>(players[i].get());
		if (!pLeader) continue;

		float currentY = startY + static_cast<float>(i) * (playerIconSize + padding);

		pLeader->RenderUI(rc, startX, currentY, playerIconSize);

		float currentAllyX = startX + playerIconSize + 10.0f;
		float allyYOffset = (playerIconSize - allyIconSize) * 0.5f;

		for (auto& ally : alliesStraight) {
			if (ally && ally->GetLeader() == pLeader) {
				ally->RenderUI(rc, currentAllyX, currentY + allyYOffset, allyIconSize);
				currentAllyX += allyIconSize + 5.0f;
			}
		}
		for (auto& ally : alliesHoming) {
			if (ally && ally->GetLeader() == pLeader) {
				ally->RenderUI(rc, currentAllyX, currentY + allyYOffset, allyIconSize);
				currentAllyX += allyIconSize + 5.0f;
			}
		}
		for (auto& ally : alliesMelee) {
			if (ally && ally->GetLeader() == pLeader) {
				ally->RenderUI(rc, currentAllyX, currentY + allyYOffset, allyIconSize);
				currentAllyX += allyIconSize + 5.0f;
			}
		}
	}

	pipRenderTarget->Deactivate(dc);
}
REGISTER_SCENE(SceneGame);

bool SceneGame::UpdatePiP()
{
	Mouse& mouse = Input::Instance().GetMouse();

	if (mouse.GetButtonDown() & Mouse::BTN_LEFT)
	{
		float mx = (float)mouse.GetPositionX();
		float my = (float)mouse.GetPositionY();

		const float pipW = 400.0f;
		const float pipH = 225.0f;
		const float tabW = 60.0f;

		float currentPipX = isPipExpanded ? 0.0f : (-pipW + tabW);
		float currentPipY = 100.0f;

		float tabLeft = currentPipX + pipW - tabW;
		float tabRight = currentPipX + pipW;
		float winTop = currentPipY;
		float winBottom = currentPipY + pipH;

		float uiLeft = isPipExpanded ? currentPipX : tabLeft;
		float uiRight = currentPipX + pipW;

		if (mx >= uiLeft && mx < uiRight &&
			my >= winTop && my < winBottom)
		{
			if (mx >= tabLeft && mx < tabRight)
			{
				isPipExpanded = !isPipExpanded;
			}
			return true;
		}
	}
	return false;
}

void SceneGame::UpdatePlayerSpawn()
{
	bool isClick = (Input::Instance().GetMouse().GetButtonDown() & Mouse::BTN_LEFT);
	if (!ImGui::GetIO().WantCaptureMouse && isClick)
	{
		if (players.size() < 5)
		{
			Player::UpdateSpawn(players, pickingRay);
		}
	}
}