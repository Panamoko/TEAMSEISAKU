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
#include "System//Audio.h"
#include "EffectManager.h"

// 派生クラスのインクルード
#include "PlayerMelee.h"
#include "PlayerHeal.h"
#include "PlayerShot.h"

static constexpr int kAutoSpawnLimit = 6;  // 自動スポーンの上限
static constexpr int kMaxSquadSize = 9;    // 部隊の最大人数（ドラッグ＆ドロップ用）
using namespace DirectX;

float SceneGame::s_timeScale = 1.0f;
float SceneGame::s_slowTimer = 0.0f;

void SceneGame::SetSlowMotion(float scale, float duration)
{
	s_timeScale = scale;
	s_slowTimer = duration;
}

SceneGame::SceneGame(const std::string& name)
{
	scene_name = name;
	Audio::Instance().Initialize();
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

	if (scene_name == "scene_tutorial")
	{
		// 新しいインスタンスを作成
		tutorial_sprite = std::make_unique<TutorialSprite>();
	}


	// ディゾルブの初期化と開始設定
	dissolve = std::make_unique<Dissolve>();
	dissolve->Initialize(Graphics::Instance().GetDevice(), "Data/Sprite/DissolveNoise.png");

	isSceneStarting = true; // フェードイン開始
	startTransitionTimer = startTransitionDuration;

	Stage_BGM = Audio::Instance().LoadAudioSource("Data/Sound/BGM_Play.wav");

	// エフェクトの読み込み
	EffectManager::Instance().Load("Hit", L"Data/Effect/Hit.efk");
}

void SceneGame::Finalize()
{
	EnemyManager::Instance().Clear();
	CollisionManager::Instance().Clear();
	GimmicManager::Instance().Clear();
	GameObjectManager::Instance().Clear();
	delete Stage_BGM;

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
	Stage_BGM->Play(true);
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

	// ★追加: 右クリックでエフェクト再生テスト
	Mouse& mouse = Input::Instance().GetMouse();
	if (mouse.GetButtonDown() & Mouse::BTN_RIGHT) // 右クリック
	{
		// ピッキングレイ（カメラからマウス位置へ伸びる線）を取得
		DirectX::XMFLOAT3 rayOrigin = pickingRay.GetRayOrigin();
		DirectX::XMFLOAT3 rayDir = pickingRay.GetRayDirection();

		// 地面(Y=0)との交差判定（簡易計算）
		// レイの方程式: P = Origin + t * Dir
		// Y = Origin.y + t * Dir.y = 0 となる t を求める
		if (std::abs(rayDir.y) > 0.001f) // ゼロ除算防止
		{
			float t = -rayOrigin.y / rayDir.y;
			if (t > 0.0f)
			{
				// 交点座標を計算
				DirectX::XMFLOAT3 hitPos;
				hitPos.x = rayOrigin.x + t * rayDir.x;
				hitPos.y = 0.0f; // 地面の高さ
				hitPos.z = rayOrigin.z + t * rayDir.z;

				// エフェクト再生！
				EffectManager::Instance().Play("Hit", hitPos);
			}
		}
	}

	// キーボード切り替え
	Player::UpdateActiveByKeyboard(players);

	if (scene_name == "scene_tutorial" && tutorial_sprite)
	{
		tutorial_sprite->Update();
	}

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
	// PiP内部の描画領域を計算して渡す
	{
		const float pipW = 400.0f;
		const float pipH = 225.0f;
		const float tabW = 60.0f;
		float margin = 10.0f;
		float marginRight = tabW + margin;

		float pipFrameX = isPipExpanded ? 0.0f : (-pipW + tabW);
		float pipFrameY = 100.0f;

		// RenderTextureが表示されている正確なスクリーン領域
		float contentX = pipFrameX + margin;
		float contentY = pipFrameY + margin;
		float contentW = pipW - (margin + marginRight);
		float contentH = pipH - (margin * 2.0f);

		// ここで正しい領域を渡すことで、クリック判定がズレなくなります
		UpdateDragDrop(contentX, contentY, contentW, contentH);
	}
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
		if (dragState.isDragging && dragState.draggedAlly)
		{
			// マウス位置を中心に描画
			float iconSize = 100.0f; // 少し大きくしても良い
			float dx = dragState.dragIconPos.x - iconSize * 0.5f;
			float dy = dragState.dragIconPos.y - iconSize * 0.5f;

			// Characterは汎用RenderUIを持っていないため、キャストして呼ぶ
			// ※ RenderUIはRenderContextを受け取るので、ここでは rc をそのまま使う
			// ※ 注意: RenderUIはZ深度テストを無効にしているか、Z=0で描画することを確認

			if (auto* s = dynamic_cast<AllySlime*>(dragState.draggedAlly))      s->RenderUI(rc, dx, dy, iconSize);
			else if (auto* h = dynamic_cast<AllySlimeHeal*>(dragState.draggedAlly))  h->RenderUI(rc, dx, dy, iconSize);
			else if (auto* m = dynamic_cast<AllySlimeMelee*>(dragState.draggedAlly)) m->RenderUI(rc, dx, dy, iconSize);
		}
		if (isSceneStarting && dissolve)
		{
			// タイマーが Max(開始時) -> 0(終了時) へ減っていく
			// 閾値: 1.0(黒) -> 0.0(透明)
			float t = std::clamp(startTransitionTimer / startTransitionDuration, 0.0f, 1.0f);
			dissolve->Render(dc, t);
		}

		if (scene_name == "scene_tutorial" && tutorial_sprite)
		{
			tutorial_sprite->Render();
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
	// ★変更: kAutoSpawnLimit (6) を使用
	if (CountAlliesFor(leader) >= kAutoSpawnLimit) return;

	int slot = CountAlliesFor(leader);
	auto p = std::make_unique<AllySlime>(slot, leader);
	alliesStraight.emplace_back(std::move(p));
}

void SceneGame::AddAllyHomingFor(Player* leader)
{
	// ★変更: kAutoSpawnLimit (6) を使用
	if (CountAlliesFor(leader) >= kAutoSpawnLimit) return;

	int slot = CountAlliesFor(leader);
	auto p = std::make_unique<AllySlimeHeal>(slot, leader);
	alliesHoming.emplace_back(std::move(p));
}

void SceneGame::AddAllyMeleeFor(Player* leader)
{
	// ★変更: kAutoSpawnLimit (6) を使用
	if (CountAlliesFor(leader) >= kAutoSpawnLimit) return;

	int slot = CountAlliesFor(leader);
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

	// 背景クリアとレンダーターゲットの有効化
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
		// Character* を Player* にキャスト
		Player* pLeader = dynamic_cast<Player*>(players[i].get());
		if (!pLeader) continue;

		// リーダーアイコンの描画位置
		float currentY = startY + static_cast<float>(i) * (playerIconSize + padding);

		// リーダー（プレイヤー）のUI描画
		pLeader->RenderUI(rc, startX, currentY, playerIconSize);

		// 味方スライムの描画開始X座標
		float currentAllyX = startX + playerIconSize + 10.0f;
		float allyYOffset = (playerIconSize - allyIconSize) * 0.5f;

		// ★修正: 味方アイコン描画用のヘルパーラムダ関数
		// 引数を auto* にすることで、AllySlime* などの具体的な型を保持したまま受け取る
		auto DrawAllyIcon = [&](auto* ally) {
			// allyが有効、かつ現在のループのリーダーに所属している場合のみ処理
			if (ally && ally->GetLeader() == pLeader) {

				// ★ドラッグ中の処理: 
				// 現在ドラッグしている対象の場合は描画をスキップする（マウスカーソル位置に描画するため）
				// ただし、場所（隙間）は空けておくためにX座標の加算は行う
				if (dragState.isDragging && dragState.draggedAlly == ally) {
					currentAllyX += allyIconSize + 5.0f;
					return;
				}

				// 味方アイコンの描画
				ally->RenderUI(rc, currentAllyX, currentY + allyYOffset, allyIconSize);

				// 次のアイコンのためにX座標を進める
				currentAllyX += allyIconSize + 5.0f;
			}
			};

		// 各味方リストに対してラムダ関数を実行
		for (auto& ally : alliesStraight) DrawAllyIcon(ally.get());
		for (auto& ally : alliesHoming)   DrawAllyIcon(ally.get());
		for (auto& ally : alliesMelee)    DrawAllyIcon(ally.get());
	}

	// レンダーターゲットを無効化（バックバッファに戻す）
	pipRenderTarget->Deactivate(dc);
}

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

void SceneGame::RebalanceFormation(Player* leader)
{
	if (!leader) return;

	int currentIndex = 0;

	// このリーダーに付いている味方を特定し、インデックスを 0, 1, 2... と振り直す
	// (Straight -> Heal -> Melee の順に並べる例)

	for (auto& a : alliesStraight) {
		if (a->GetLeader() == leader) {
			a->SetIndex(currentIndex);
			currentIndex++;
		}
	}
	for (auto& a : alliesHoming) {
		if (a->GetLeader() == leader) {
			a->SetIndex(currentIndex);
			currentIndex++;
		}
	}
	for (auto& a : alliesMelee) {
		if (a->GetLeader() == leader) {
			a->SetIndex(currentIndex);
			currentIndex++;
		}
	}
}

// ★追加: ドラッグアンドドロップのメインロジック
void SceneGame::UpdateDragDrop(float pipX, float pipY, float pipW, float pipH)
{
	Mouse& mouse = Input::Instance().GetMouse();
	float mx = (float)mouse.GetPositionX();
	float my = (float)mouse.GetPositionY();

	// レンダーターゲットの解像度 (RenderTarget.cppの初期化サイズと合わせる)
	const float rtWidth = 1280.0f;
	const float rtHeight = 720.0f;

	// マウス座標を、PiP内のローカル座標(0.0~1.0)に変換
	// pipX, pipY はウィンドウ枠を除いた「中身」の左上座標である前提
	float u = (mx - pipX) / pipW;
	float v = (my - pipY) / pipH;

	// レンダーターゲット上の絶対座標に変換
	float localX = u * rtWidth;
	float localY = v * rtHeight;

	// UIパラメータ (RenderPiPと完全に一致させる)
	float playerIconSize = 128.0f;
	float allyIconSize = 100.0f;
	float padding = 12.0f;
	float startX = 20.0f;
	float startY = 20.0f;

	// --- ドラッグ開始 (左クリック) ---
	if (mouse.GetButtonDown() & Mouse::BTN_LEFT)
	{
		// PiPの範囲外ならクリック判定しない
		if (mx >= pipX && mx <= pipX + pipW && my >= pipY && my <= pipY + pipH)
		{
			for (size_t i = 0; i < players.size(); ++i)
			{
				Player* pLeader = dynamic_cast<Player*>(players[i].get());
				if (!pLeader) continue;

				float currentY = startY + static_cast<float>(i) * (playerIconSize + padding);
				float currentAllyX = startX + playerIconSize + 10.0f;
				float allyYOffset = (playerIconSize - allyIconSize) * 0.5f;
				float allyY = currentY + allyYOffset;

				// 味方アイコンの判定用ラムダ
				auto CheckHit = [&](auto* ally) -> bool {
					if (ally && ally->GetLeader() == pLeader) {
						// アイコンの矩形判定
						if (localX >= currentAllyX && localX <= currentAllyX + allyIconSize &&
							localY >= allyY && localY <= allyY + allyIconSize)
						{
							// ヒット！ドラッグ開始
							dragState.isDragging = true;
							dragState.draggedAlly = ally; // ポインタ保存
							dragState.oldLeader = pLeader;
							dragState.dragIconPos = { mx, my };
							return true;
						}
						// 次のアイコン位置へ
						currentAllyX += allyIconSize + 5.0f;
					}
					return false;
					};

				// 全リスト走査
				bool hit = false;
				for (auto& a : alliesStraight) if (CheckHit(a.get())) { hit = true; break; }
				if (hit) break;
				for (auto& a : alliesHoming)   if (CheckHit(a.get())) { hit = true; break; }
				if (hit) break;
				for (auto& a : alliesMelee)    if (CheckHit(a.get())) { hit = true; break; }
				if (hit) break;
			}
		}
	}

	// --- ドラッグ中 ---
	if (dragState.isDragging)
	{
		// アイコン表示位置を更新
		dragState.dragIconPos = { mx, my };

		// ドロップ (左クリック離した)
		if (mouse.GetButtonUp() & Mouse::BTN_LEFT)
		{
			// ドロップ先のリーダー判定
			for (size_t i = 0; i < players.size(); ++i)
			{
				Player* pTarget = dynamic_cast<Player*>(players[i].get());
				if (!pTarget) continue;

				float targetY = startY + static_cast<float>(i) * (playerIconSize + padding);

				// 判定エリア: プレイヤーアイコンのY座標の帯状のエリアにドロップしたらOKとする
				// (横幅はPiP全体、高さはアイコン分)
				if (localY >= targetY && localY <= targetY + playerIconSize &&
					localX >= 0 && localX <= rtWidth)
				{
					// リーダーが異なる場合のみ移動処理
					if (pTarget != dragState.oldLeader && dragState.draggedAlly)
					{
						// ★追加: 移動先(pTarget)の部隊人数が最大(9体)に達していないかチェック
						if (CountAlliesFor(pTarget) < kMaxSquadSize)
						{
							// 1. リーダーを書き換え
							if (auto* s = dynamic_cast<AllySlime*>(dragState.draggedAlly)) s->SetLeader(pTarget);
							else if (auto* h = dynamic_cast<AllySlimeHeal*>(dragState.draggedAlly)) h->SetLeader(pTarget);
							else if (auto* m = dynamic_cast<AllySlimeMelee*>(dragState.draggedAlly)) m->SetLeader(pTarget);

							// 2. 隊列を再計算
							RebalanceFormation(dragState.oldLeader);
							RebalanceFormation(pTarget);
						}
						// ※ else の場合は「満員」なので移動させない（何もしない）
					}
					break;
				}
			}

			// ドラッグ終了
			dragState.isDragging = false;
			dragState.draggedAlly = nullptr;
			dragState.oldLeader = nullptr;
		}
	}
}


REGISTER_SCENE(SceneGame);
