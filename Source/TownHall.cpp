#include "TownHall.h"
#include "System/ShapeRenderer.h"
#include "System/RenderContext.h"

//#include "static_mesh.h"
//#include "texture.h"
//#include "misc.h" // デバッグ描画ヘルパなどがあるなら
#include <imgui.h> // HPバーを画面に出す場合
#include <CollisionManager.h>
using namespace DirectX;


TownHall::TownHall(const XMFLOAT3& pos, float radius, int maxHP)
	: position(pos), radius(radius), maxHP(maxHP), hp(maxHP) {}


void TownHall::Initialize() {

	// 必要ならモデル読み込みなど。
	// mesh = new StaticMesh("assets/townhall.fbx");
	// tex = new Texture("assets/townhall_albedo.dds");

	class_name = "TownHall";
	model = ModelManager::Instance().Load("Data/Model/bilud/Core.mdl");

	// Animator にモデルを渡して Idle ループ
	animator.SetModel(model /* or model.get() */);
	animator.SetBlendSeconds(0.2f);
	animator.Play("Take 001", true);

	scale = { 0.3f, 0.3f, 0.3f };

	collider = std::make_unique<CylinderCollider>();
	collider->type = ColliderType::Cylinder;
	cylinder = static_cast<CylinderCollider*>(collider.get());

	cylinder->center = position;
	cylinder->height = height;
	cylinder->radius = radius;

	CollisionManager::Instance().AddObject(this);
}


void TownHall::Update(float elapsedtime) {
	animator.Update(elapsedtime);
}


void TownHall::Render(const RenderContext& rc, ModelRenderer* renderer) {
	if (IsDestroyed()) return;

	using namespace DirectX;
	XMMATRIX S = XMMatrixScaling(scale.x, scale.y, scale.z);
	XMMATRIX R = XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z); // rotationを持っていなければ省略
	XMMATRIX T = XMMatrixTranslation(position.x, position.y, position.z);

	XMFLOAT4X4 transform;
	XMStoreFloat4x4(&transform, S * R * T);

	renderer->Render(rc, transform, model, ShaderId::Lambert);
	// モデルを持っている場合は描画。
	 //if (model) model->Draw(position, /*rotation*/{}, /*scale*/{1,1,1}, tex);
	for (auto& a : model->GetResource()->GetAnimations()) {
		printf("anim: %s (%.3fs)\n", a.name.c_str(), a.secondsLength);
	}
}


void TownHall::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
	if (!renderer || IsDestroyed()) return;

	// 建物の当たり（円柱）を可視化：赤
	renderer->RenderCylinder(
		rc,
		cylinder->center,    // 中心（地面に置く想定）
		cylinder->radius,      // 水平半径
		cylinder->height,      // 高さ
		XMFLOAT4(1, 0, 0, 1)
	);
}

void TownHall::TakeDamage(int amount) {
	if (hp <= 0) return;
	hp -= amount;
	if (hp < 0) hp = 0;
}

void TownHall::OnCollision(GameObject* object)
{
	//TakeDamage();
}
