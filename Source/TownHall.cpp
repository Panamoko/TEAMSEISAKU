#include "TownHall.h"
#include "System/ShapeRenderer.h"
#include "System/RenderContext.h"
//#include "static_mesh.h"
//#include "texture.h"
//#include "misc.h" // デバッグ描画ヘルパなどがあるなら
#include <imgui.h> // HPバーを画面に出す場合
using namespace DirectX;


TownHall::TownHall(const XMFLOAT3& pos, float radius, int maxHP)
	: position(pos), radius(radius), maxHP(maxHP), hp(maxHP) {}


void TownHall::Initialize() {
	// 必要ならモデル読み込みなど。
	// mesh = new StaticMesh("assets/townhall.fbx");
	// tex = new Texture("assets/townhall_albedo.dds");
}


void TownHall::Update(float /*dt*/) {
	// 静的オブジェクトなので基本は何もしない。
}


void TownHall::Render() {
	if (IsDestroyed()) return;
	// モデルを持っている場合は描画。
	// if (mesh) mesh->Draw(position, /*rotation*/{}, /*scale*/{1,1,1}, tex);
}


void TownHall::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
	if (!renderer || IsDestroyed()) return;

	// 建物の当たり（円柱）を可視化：赤
	renderer->RenderCylinder(
		rc,
		position,    // 中心（地面に置く想定）
		radius,      // 水平半径
		height,      // 高さ
		XMFLOAT4(1, 0, 0, 1)
	);
}

void TownHall::TakeDamage(int amount) {
	if (hp <= 0) return;
	hp -= amount;
	if (hp < 0) hp = 0;
}