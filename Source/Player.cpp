#include "Collision.h"
#include "ProjectileStraite.h"
#include "ProjectileHoming.h"
#include "Player.h"
#include "EnemyManager.h"  // ← 追記
#include "Enemy.h"         // ← 追記
#include "Camera.h"
#include "System/Input.h"
#include <imgui.h>
#include <cfloat>          // FLT_MAX
#include <cmath>           // sqrtf
#include "System/Graphics.h"   // 画面サイズフォールバック用（ビューポート未設定時）
#include <DirectXMath.h>
#include <d3d11.h>

#include "BuildingManager.h"
#include "TownHall.h"
using namespace DirectX;


Player* Player::sActive = nullptr;
Player& Player::Instance() { return *sActive; }
void Player::SetActive(Player* p) { sActive = p; }
Player* Player::GetActivePtr() { return sActive; }

namespace {
	struct PickViewport { float x = 0, y = 0, w = 0, h = 0; };
	static PickViewport gPV;

	// NDC → 「ビューポートのピクセル座標」へ（TopLeftX/Y考慮）
	static bool WorldToViewportPixel(const XMFLOAT3& world, float& outX, float& outY)
	{
		Camera& cam = Camera::Instance();
		XMMATRIX view = XMLoadFloat4x4(&cam.GetView());
		XMMATRIX proj = XMLoadFloat4x4(&cam.GetProjection());
		XMVECTOR p = XMLoadFloat3(&world);

		XMVECTOR clip = XMVector4Transform(XMVectorSetW(p, 1.0f), XMMatrixMultiply(view, proj));
		const float cx = XMVectorGetX(clip);
		const float cy = XMVectorGetY(clip);
		const float cw = XMVectorGetW(clip);
		if (cw <= 0.0f) return false;

		const float ndcX = cx / cw;
		const float ndcY = cy / cw;

		const float W = (gPV.w > 0 ? gPV.w : (float)Graphics::Instance().GetScreenWidth());
		const float H = (gPV.h > 0 ? gPV.h : (float)Graphics::Instance().GetScreenHeight());
		outX = gPV.x + (ndcX * 0.5f + 0.5f) * W;
		outY = gPV.y + (1.0f - (ndcY * 0.5f + 0.5f)) * H;
		return true;
	}
}

// ========= Player の static 実装 =========
void Player::SetPickViewport(float topLeftX, float topLeftY, float width, float height)
{
	gPV.x = topLeftX; gPV.y = topLeftY; gPV.w = width; gPV.h = height;
}

void Player::CapturePickViewportFromRS()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	UINT n = 1;
	D3D11_VIEWPORT vp{};
	dc->RSGetViewports(&n, &vp);
	if (n == 1 && vp.Width > 0.0f && vp.Height > 0.0f) {
		SetPickViewport(vp.TopLeftX, vp.TopLeftY, vp.Width, vp.Height);
	}
	else {
		SetPickViewport(0.0f, 0.0f,
			(float)Graphics::Instance().GetScreenWidth(),
			(float)Graphics::Instance().GetScreenHeight());
	}
}

Player* Player::PickNearestByScreenCircle(
	float mouseX, float mouseY,
	const std::vector<std::unique_ptr<Player>>& players,
	float pixelRadius)
{
	Player* best = nullptr;
	float bestD2 = FLT_MAX;

	for (auto& up : players)
	{
		Player* p = up.get();
		XMFLOAT3 pos = p->GetPosition();
		// pos.y += 0.8f; // クリックしやすく少し上げたい場合

		float sx, sy;
		if (!WorldToViewportPixel(pos, sx, sy)) continue;

		const float dx = sx - mouseX;
		const float dy = sy - mouseY;
		const float d2 = dx * dx + dy * dy;
		if (d2 <= pixelRadius * pixelRadius && d2 < bestD2) {
			bestD2 = d2; best = p;
		}
	}
	return best;
}

bool Player::SelectActiveByScreenClick(
	float mouseX, float mouseY,
	const std::vector<std::unique_ptr<Player>>& players,
	float pixelRadius)
{
	if (Player* p = PickNearestByScreenCircle(mouseX, mouseY, players, pixelRadius)) {
		Player::SetActive(p);     // sActive に直接代入しない
		return true;
	}
	return false;
}

bool Player::UpdateSelectionFromMouse(
	const std::vector<std::unique_ptr<Player>>& players,
	float pixelRadius)
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse) return false;  // UI上の操作は無視

	Input& input = Input::Instance();
	Mouse& mouse = input.GetMouse();

	static unsigned int prevButtons = 0;
	const unsigned int buttons = mouse.GetButton();
	const bool leftTriggered = ((buttons & Mouse::BTN_LEFT) && !(prevButtons & Mouse::BTN_LEFT));
	prevButtons = buttons;

	if (!leftTriggered) return false;

	// ImGui座標 → DPIを掛けてフレームバッファ座標へ
	const float mx = io.MousePos.x * io.DisplayFramebufferScale.x;
	const float my = io.MousePos.y * io.DisplayFramebufferScale.y;

	return SelectActiveByScreenClick(mx, my, players, pixelRadius);
}
void Player::DebugDrawSelectionOverlay(
	const std::vector<std::unique_ptr<Player>>& players,
	float pixelRadius,
	bool highlightActive)
{
	ImGuiIO& io = ImGui::GetIO();
	ImDrawList* dl = ImGui::GetForegroundDrawList();

	// マウス座標（フレームバッファpx）※選択判定と同じ座標系に合わせる
	const float mx_fb = io.MousePos.x * io.DisplayFramebufferScale.x;
	const float my_fb = io.MousePos.y * io.DisplayFramebufferScale.y;

	// 描画座標は ImGui座標系なので、FB→ImGui へ逆変換して渡す
	const float inv_scale_x = (io.DisplayFramebufferScale.x != 0.f) ? (1.0f / io.DisplayFramebufferScale.x) : 1.0f;
	const float inv_scale_y = (io.DisplayFramebufferScale.y != 0.f) ? (1.0f / io.DisplayFramebufferScale.y) : 1.0f;

	// 半径も ImGui座標系に合わせる（基本はXスケールで十分。非等方DPIなら平均やminでも可）
	const float radius_imgui = pixelRadius * inv_scale_x;

	Player* active = Player::GetActivePtr();

	for (auto& up : players)
	{
		Player* p = up.get();
		DirectX::XMFLOAT3 pos = p->GetPosition();
		// pos.y += 0.8f; // 円の中心を少し上へずらしたい場合

		float sx_fb, sy_fb; // フレームバッファpx
		if (!WorldToViewportPixel(pos, sx_fb, sy_fb)) continue;

		// ImGui座標系へ変換して描画
		const ImVec2 center_imgui(sx_fb * inv_scale_x, sy_fb * inv_scale_y);

		// ホバー判定はFB座標で（選択ロジックと同じ計算）
		const float dx = sx_fb - mx_fb;
		const float dy = sy_fb - my_fb;
		const bool hovered = (dx * dx + dy * dy) <= (pixelRadius * pixelRadius);

		// 色・太さ
		ImU32 col = IM_COL32(255, 255, 0, 180);  // 基本：黄
		float thickness = 2.0f;
		if (hovered) { col = IM_COL32(0, 255, 255, 220); thickness = 3.0f; } // ホバー：シアン
		if (highlightActive && p == active) { col = IM_COL32(0, 255, 0, 220); thickness = 3.0f; } // アクティブ：緑

		// 円（当たり判定そのもの）
		dl->AddCircle(center_imgui, radius_imgui, col, 48, thickness);

		// 中心の小点（見やすさ用）
		dl->AddCircleFilled(center_imgui, 3.0f, IM_COL32(255, 255, 255, 200));

		// ラベル（任意）
		// dl->AddText(ImVec2(center_imgui.x + 8, center_imgui.y + 8), IM_COL32(255,255,255,200), "Pick");
	}
}

//初期化
void Player::Initialize()
{
	model = ModelManager::Instance().Load("Data/Model/Mr.Incredible/Mr.Incredible.mdl");
	// モデルが大きいのでスケーリング
	scale.x = scale.y = scale.z = 0.01f;
}

//終了化
void Player::Finalize()
{
	//delete model;
}

void Player::Update(float elapsedTime)
{
     const bool isActive = (this == GetActivePtr());
     if (isActive) {
         // 入力は“選択された個体のみ”
		 InputToggleAttackPriority();
         InputMove(elapsedTime);
         InputJump();
         InputProjectile();
         AutoAttackUpdate(elapsedTime);   // ← 非アクティブでも撃たせたいなら下へ移動
     } else {
         // 非アクティブでも自動攻撃させたいならこちらで呼ぶ
         AutoAttackUpdate(elapsedTime);
     }

	 if (!IsActive() && autoMoveToEnemyEnabled) 
	 {
		 UpdateAutoMoveToEnemy(elapsedTime);
	 }

	//速力更新処理
	UpdateVelocity(elapsedTime);

	//弾丸更新処理
	projectileManager.Update(elapsedTime);

	//プレイヤーと敵との衝突処理
	CollisionPlayerVsEnemies();

	//プレイヤーと柵との衝突処理
	CollisionPlayerVsFences();

	//弾丸と敵の衝突処理
	CollisionProjectilesVsEnemies();

	// オブジェクト行列を更新
	UpdateTransform();

	// モデル行列更新
	model->UpdateTransform();
}

void Player::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	renderer->Render(rc, transform, model, ShaderId::Lambert);

	//弾丸描画処理
	projectileManager.Render(rc, renderer);
}

// デバッグ用GUI描画
void Player::DrawDebugGUI()
{
	ImVec2 pos = ImGui::GetMainViewport()->GetWorkPos();
	ImGui::SetNextWindowPos(ImVec2(pos.x + 10, pos.y + 10), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Player", nullptr, ImGuiWindowFlags_None))
	{
		// トランスフォーム
		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// 位置
			ImGui::InputFloat3("Position", &position.x);

			// 回転
			DirectX::XMFLOAT3 a;
			a.x = DirectX::XMConvertToDegrees(angle.x);
			a.y = DirectX::XMConvertToDegrees(angle.y);
			a.z = DirectX::XMConvertToDegrees(angle.z);
			ImGui::InputFloat3("Angle", &a.x);

			angle.x = DirectX::XMConvertToRadians(a.x);
			angle.y = DirectX::XMConvertToRadians(a.y);
			angle.z = DirectX::XMConvertToRadians(a.z);

			// スケール
			ImGui::InputFloat3("Scale", &scale.x);

			// 回転値
			//ImGui::InputFloat("rot", &rot);

		}
		 // 自動攻撃のデバッグ設定
         if (ImGui::CollapsingHeader("Auto Attack", ImGuiTreeNodeFlags_DefaultOpen))
         {
             ImGui::Checkbox("Enabled", &autoAttackEnabled);
             ImGui::DragFloat("Range", &autoAttackRange, 0.1f, 0.0f, 100.0f);
             ImGui::DragFloat("Interval (sec)", &autoAttackInterval, 0.01f, 0.1f, 10.0f);
             // 現在のタイマー表示
             ImGui::Text("Timer: %.2f", autoAttackTimer);
			 // ★ 優先度切り替えボタン
			 const char* modeLabel = (attackPriority == AttackPriority::CoreFirst)
				 ? "Core → Enemy"
				 : "Enemy → Core";
			 if (ImGui::Button(modeLabel, ImVec2(150, 0))) 
			 {
				 attackPriority = (attackPriority == AttackPriority::CoreFirst)
					 ? AttackPriority::EnemyFirst
					 : AttackPriority::CoreFirst;
			 }
         }
	}

	ImGui::End();
}



DirectX::XMFLOAT3 Player::GetMoveVec() const
{
	// 入力情報を取得
	GamePad& gamePad = Input::Instance().GetGamePad();
	float ax = gamePad.GetAxisLX(); // スティックのX軸入力
	float ay = gamePad.GetAxisLY(); // スティックのY軸入力

	//  カメラ方向ではなく、ワールド軸（X軸:右、Z軸:前）を基準に移動ベクトルを計算する
	DirectX::XMFLOAT3 vec;

	// スティックX軸 (ax) をワールドX軸（右方向）に、
	// スティックY軸 (ay) をワールドZ軸（前方向）に割り当てる
	vec.x = ax; // ax: -1.0(左)  +1.0(右) -> ワールドX軸
	vec.z = ay; // ay: -1.0(下)  +1.0(上) -> ワールドZ軸
	vec.y = 0.0f; // Y軸方向には移動しない

	// ベクトルの正規化（斜め移動の速度が速くならないように）
	float lengthSq = vec.x * vec.x + vec.z * vec.z;
	if (lengthSq > 1.0f)
	{
		float length = sqrtf(lengthSq);
		vec.x /= length;
		vec.z /= length;
	}

	return vec;
}

// 移動入力処理
void Player::InputMove(float elapsedTime)
{
	// 進行ベクトル取得
	DirectX::XMFLOAT3 moveVec = GetMoveVec();

	// 移動処理
	Move(elapsedTime, moveVec.x, moveVec.z, moveSpeed);

	// 旋回処理
	Turn(elapsedTime, moveVec.x, moveVec.z, turnSpeed);
}


//プレイヤーと敵との衝突処理
void Player::CollisionPlayerVsEnemies()
{
	EnemyManager& enemyManager = EnemyManager::Instance();

	// 全ての敵と総当たりで衝突処理
	int enemyCount = enemyManager.GetEnemyCount();
	for (int i = 0; i < enemyCount; ++i)
	{
		std::shared_ptr<Enemy> enemy = enemyManager.GetEnemy(i);

		// 衝突処理
		DirectX::XMFLOAT3 outPosition;

		if (Collision::IntersectCylinderVsCylinder(
			position, 
			radius,
			height,
			enemy->GetPosition(),
			enemy->GetRadius(),
			enemy->GetHeight(),
			outPosition))
		{
			// 押し出し後の位置設定
			//enemy->SetPosition(outPosition);
						
			// 敵の真上付近に当たったかを判定
			DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&position);
			DirectX::XMVECTOR E = DirectX::XMLoadFloat3(&enemy->GetPosition());
			DirectX::XMVECTOR V = DirectX::XMVectorSubtract(P, E);
			DirectX::XMVECTOR N = DirectX::XMVector3Normalize(V);
			DirectX::XMFLOAT3 normal;
			DirectX::XMStoreFloat3(&normal, N);

			// 上から踏んづけた場合は小ジャンプする
			if (normal.y > 0.8f)
			{
				// 小ジャンプする
				Jump(jumpSpeed * 0.5f);

				//踏みつけ処理も実装しているので、ダメージ処理を実装してみる。
				//enemy->ApplyDamage(1);
				enemy->ApplyDamage(1,0.5f);
			}
			else
			{
				// 押し出し後の位置設定
				enemy->SetPosition(outPosition);
			}

		}
	}
}

void Player::CollisionPlayerVsFences()
{
	{
		auto& bm = BuildingManager::Instance();
		const int n = bm.GetFenceCount();
		for (int i = 0; i < n; ++i) {
			const Fence* f = bm.GetFence(i);
			if (!f || !f->IsAlive()) continue;
			const OBB& box = bm.GetFence(i)->GetOBB();
			DirectX::XMFLOAT3 mtd;
			if (Collision::IntersectCylinderVsOBB(position, radius, height, box, &mtd)) {
				position.x += mtd.x; position.y += mtd.y; position.z += mtd.z;
			}
		}
	}
}


void Player::InputJump()
{
	GamePad& gamePad = Input::Instance().GetGamePad();
	if (gamePad.GetButtonDown() & GamePad::BTN_A)
	{
		//Jump(jumpSpeed);
		//ジャンプ回数制限
		if (jumpCount < jumpLimit)
		{
			//ジャンプ
			jumpCount++;
			Jump(jumpSpeed);
		}
	}
}

void Player::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
	//基底クラスの関数呼び出し
	Character::RenderDebugPrimitive(rc, renderer);

	//弾丸デバッグプリミティブ描画
	projectileManager.RenderDebugPrimitive(rc, renderer);

	renderer->RenderCylinder(
		rc,
		GetPosition(),                           // 中心
		/*radius=*/GetRadius() + 0.2f,                         // 半径（ワールド単位：調整可）
		/*height=*/0.05f,                        // 薄い円柱でOK
		DirectX::XMFLOAT4(1, 1, 0, 0.5f));       // 色（半透明の黄）
	 // --- 自動攻撃範囲の可視化 ---
    if (autoAttackEnabled)
    {
        // 攻撃範囲を円柱で表示（スライムの索敵円柱を参考に）:contentReference[oaicite:1]{index=1}
        renderer->RenderCylinder(
            rc,
            position,              // プレイヤーの位置を中心に
            autoAttackRange,       // 半径
            1.0f,                  // 高さ（見やすくするために低め）
            DirectX::XMFLOAT4(0, 1, 1, 1.0f) // 色：青・半透明
        );
    }
	// --- 選択中マーカー（薄い黄色リング） ---
    if (this == GetActivePtr()) {
        DirectX::XMFLOAT3 ringPos = position; ringPos.y += 0.02f;
        renderer->RenderCylinder(
            rc, ringPos, radius + 0.15f, 0.05f,
            DirectX::XMFLOAT4(1, 1, 0, 0.8f)
        );
    }
}

void Player::OnLanding()
{
	jumpCount = 0;
}

void Player::InputProjectile()
{
	GamePad& gamePad = Input::Instance().GetGamePad();

	//直進弾丸発射
	if (gamePad.GetButtonDown() & GamePad::BTN_X)
	{
		// 前方向
		DirectX::XMFLOAT3 dir;
		dir.x = sinf(angle.y);
		dir.y = 0.0f;
		dir.z = cosf(angle.y);

		// 発射位置（プレイヤーの腰あたり）
		DirectX::XMFLOAT3 pos;
		pos.x = position.x;
		pos.y = position.y + height * 0.5f;
		pos.z = position.z;

		// 発射
		//ProjectileStraite* projectile = new ProjectileStraite();
		ProjectileStraite* projectile = new ProjectileStraite(&projectileManager);
		projectile->Launch(dir, pos);

		//弾丸クラスのコンストラクタで呼び出すようになったので削除
		//projectileManager.Register(projectile);
	}

	// 追尾弾丸発射
	if (gamePad.GetButtonDown() & GamePad::BTN_Y)
	{
		// 前方向
		DirectX::XMFLOAT3 dir;
		dir.x = sinf(angle.y);
		dir.y = 0.0f;
		dir.z = cosf(angle.y);

		// 発射位置（プレイヤーの腰あたり）
		DirectX::XMFLOAT3 pos;
		pos.x = position.x;
		pos.y = position.y + height * 0.5f;
		pos.z = position.z;

		// ターゲット（デフォルトではプレイヤーの前方）
		DirectX::XMFLOAT3 target;
		target.x = pos.x + dir.x * 1000.0f;
		target.y = pos.y + dir.y * 1000.0f;
		target.z = pos.z + dir.z * 1000.0f;

		// 一番近くの敵をターゲットにする
		float dist = FLT_MAX;
		EnemyManager& enemyManager = EnemyManager::Instance();
		int enemyCount = enemyManager.GetEnemyCount();
		for (int i = 0; i < enemyCount; ++i)
		{
			// 敵との距離判定
			std::shared_ptr<Enemy> enemy = EnemyManager::Instance().GetEnemy(i);
			DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&position);
			DirectX::XMVECTOR E = DirectX::XMLoadFloat3(&enemy->GetPosition());
			DirectX::XMVECTOR V = DirectX::XMVectorSubtract(E, P);
			DirectX::XMVECTOR D = DirectX::XMVector3LengthSq(V);
			float d;
			DirectX::XMStoreFloat(&d, D);
			if (d < dist)
			{
				dist = d;
				target = enemy->GetPosition();
				target.y += enemy->GetHeight() * 0.5f;
			}
		}

		// 発射
		ProjectileHoming* projectile = new ProjectileHoming(&projectileManager);
		projectile->Launch(dir, pos, target);
	}

}

// 弾丸と敵の衝突処理
void Player::CollisionProjectilesVsEnemies()
{
	EnemyManager& enemyManager = EnemyManager::Instance();
	TownHall* townHall = BuildingManager::Instance().GetTownHall();

	const int projectileCount = projectileManager.GetProjectileCount();
	const int enemyCount = enemyManager.GetEnemyCount();

	for (int i = 0; i < projectileCount; ++i)
	{
		Projectile* projectile = projectileManager.GetProjectile(i);
		if (!projectile) continue;

		bool destroyed = false;

		// 1) まず TownHall との衝突
		if (townHall && townHall->IsAlive())
		{
			DirectX::XMFLOAT3 outPos;
			if (Collision::IntersectSphereVsCylinder(
				projectile->GetPosition(),
				projectile->GetRadius(),
				townHall->GetPosition(),
				townHall->GetRadius(),
				townHall->GetHeight(),
				outPos))
			{
				townHall->TakeDamage(1);
				projectile->Destroy();
				destroyed = true;
			}
		}
		if (destroyed) continue;

		// 2) つぎに「弾 × 敵」
		for (int j = 0; j < enemyCount; ++j)
		{
			std::shared_ptr<Enemy> enemy = enemyManager.GetEnemy(j);
			if (!enemy) continue;

			DirectX::XMFLOAT3 outPosition;
			if (Collision::IntersectSphereVsCylinder(
				projectile->GetPosition(),
				projectile->GetRadius(),
				enemy->GetPosition(),
				enemy->GetRadius(),
				enemy->GetHeight(),
				outPosition))
			{
				if (enemy->ApplyDamage(1, 0.5f))
				{
					// 軽いノックバック（XZ）
					DirectX::XMFLOAT3 impulse{};
					const float power = 10.0f;
					const auto& e = enemy->GetPosition();
					const auto& p = projectile->GetPosition();
					float vx = e.x - p.x;
					float vz = e.z - p.z;
					float lenXZ = std::sqrt(vx * vx + vz * vz);
					if (lenXZ > 1e-4f) { vx /= lenXZ; vz /= lenXZ; }
					impulse.x = vx * power;
					impulse.y = power * 0.5f;
					impulse.z = vz * power;
					enemy->AddImpulse(impulse);
				}
				projectile->Destroy();
				break;
			}
		}
	}
}


// 自動攻撃（スライムのように一定間隔で自動発射）
void Player::AutoAttackUpdate(float elapsedTime)
{
	if (!autoAttackEnabled) return;

	// クールダウン
	if (autoAttackTimer > 0.0f) {
		autoAttackTimer -= elapsedTime;
		return;
	}

	// 発射位置（腰あたり）
	DirectX::XMFLOAT3 pos{ position.x, position.y + height * 0.5f, position.z };
	const float rangeSq = autoAttackRange * autoAttackRange;

	// ==============================
	// 1) ターゲット候補を検索
	// ==============================
		TownHall* townHall = BuildingManager::Instance().GetTownHall();
    	std::shared_ptr<Enemy> nearestEnemy = FindNearestEnemy(); // XZ平面での最寄り
    
    	// 射程内のターゲットの「狙うべき座標」を格納
    	DirectX::XMFLOAT3 coreTargetPos{};
    	bool coreInRange = false;
    	if (townHall && townHall->IsAlive())
    	{
    		coreTargetPos = townHall->GetPosition();
    		coreTargetPos.y += townHall->GetHeight() * 0.5f; // 中心の高さを狙う
    		float dx = coreTargetPos.x - pos.x;
    		float dy = coreTargetPos.y - pos.y;
    		float dz = coreTargetPos.z - pos.z;
    		if (dx * dx + dy * dy + dz * dz <= rangeSq) {
    			coreInRange = true;
    		}
    	}
    
    	DirectX::XMFLOAT3 enemyTargetPos{};
    	bool enemyInRange = false;
    	if (nearestEnemy)
    	{
    		enemyTargetPos = nearestEnemy->GetPosition();
    		enemyTargetPos.y += nearestEnemy->GetHeight() * 0.5f; // 中心の高さを狙う
    		float dx = enemyTargetPos.x - pos.x;
    		float dy = enemyTargetPos.y - pos.y;
    		float dz = enemyTargetPos.z - pos.z;
    		// ※ FindNearestEnemy は XZ 距離なので、Yも含めた射程を再計算
    		if (dx * dx + dy * dy + dz * dz <= rangeSq) {
    			enemyInRange = true;
    		}
    	}
    
    	// ==============================
    	// 2) 優先度に基づいて最終ターゲットを決定
    	// ==============================
    	DirectX::XMFLOAT3 finalTargetPos{};
    	bool hasTarget = false;
    
    	if (attackPriority == AttackPriority::CoreFirst)
    	{
    		if (coreInRange) {
    			finalTargetPos = coreTargetPos;
    			hasTarget = true;
    		} else if (enemyInRange) {
    			finalTargetPos = enemyTargetPos;
    			hasTarget = true;
    		}
    	}
    	else // (attackPriority == AttackPriority::EnemyFirst)
    	{
    		if (enemyInRange) {
    			finalTargetPos = enemyTargetPos;
    			hasTarget = true;
    		} else if (coreInRange) {
    			finalTargetPos = coreTargetPos;
    			hasTarget = true;
    		}
    	}
    
    	// ========= 3) 発射 =========
	if (hasTarget) {
		DirectX::XMFLOAT3 dir{
			finalTargetPos.x - pos.x,
			finalTargetPos.y - pos.y,
			finalTargetPos.z - pos.z
		};
		float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
		if (len < 1e-3f) return;
		dir.x /= len; dir.y /= len; dir.z /= len;

		auto* projectile = new ProjectileStraite(&projectileManager);
		projectile->Launch(dir, pos);

		autoAttackTimer = autoAttackInterval;
	}
}


bool Player::IsActive() const
{
	return sActive == this;
}

// 最寄りの敵を取得（XZ 平面距離）
std::shared_ptr<Enemy> Player::FindNearestEnemy() const
{
	int count = EnemyManager::Instance().GetEnemyCount();
	std::shared_ptr<Enemy> nearest = nullptr;
	float bestDist2 = FLT_MAX;

	for (int i = 0; i < count; ++i) {
		std::shared_ptr<Enemy> e = EnemyManager::Instance().GetEnemy(i);
		if (!e) continue;
		const auto ep = e->GetPosition();
		float dx = ep.x - position.x;
		float dz = ep.z - position.z;
		float d2 = dx * dx + dz * dz;
		if (d2 < bestDist2) {
			bestDist2 = d2;
			nearest = e;
		}
	}
	return nearest;
}

// 非アクティブ時の自動移動（敵に向かい、近づきすぎたら停止）
void Player::UpdateAutoMoveToEnemy(float dt) 
{
	std::shared_ptr<Enemy> target = FindNearestEnemy();
	if (!target) return;

	const auto tp = target->GetPosition();
	float vx = tp.x - position.x;
	float vz = tp.z - position.z;
	float dist = std::sqrt(vx * vx + vz * vz);
	if (dist < 1e-3f) return;

	vx /= dist; vz /= dist;

	// 常に敵の方向へ向く
	Turn(dt, vx, vz, turnSpeed * autoMoveTurnRate);

	// 一定距離より遠ければ前進、近ければ止まる
	if (dist > autoMoveStopDistance) {
		Move(dt, vx, vz, moveSpeed * autoMoveSpeedRate);
	}
	else {
		Move(dt, 0.0f, 0.0f, 0.0f);
	}
}

// ★関数を丸ごと追加
// 攻撃優先度切り替え入力
void Player::InputToggleAttackPriority()
{
	// Input.h から GamePad インスタンスを取得
	Input& input = Input::Instance();
	GamePad& gamePad = input.GetGamePad();

	// 'X'キー（GamePad::BTN_B としてエミュレートされている）が押された瞬間
	if (gamePad.GetButtonDown() & GamePad::BTN_B)
	{
		if (attackPriority == AttackPriority::CoreFirst) {
			attackPriority = AttackPriority::EnemyFirst;
		} else {
			attackPriority = AttackPriority::CoreFirst;
		}
	}
}
