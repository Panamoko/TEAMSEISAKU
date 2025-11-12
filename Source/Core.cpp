#include "Core.h"
#include "Factory.h"
#include "ModelManager.h"
#include "SceneManager.h"
#include "SceneTitle.h"
#include "SceneLoading.h"
#include <imgui.h>

Core::Core()
{
	model = ModelManager::Instance().Load("Data/Model/bilud/Core.mdl");

	animator.SetModel(model /* or model.get() */);
	animator.SetBlendSeconds(0.2f);
	animator.Play("Take 001", true);

	collider = std::make_unique<CylinderCollider>();
	collider->type = ColliderType::Cylinder;
	collider->owner = this;
	cylinder = static_cast<CylinderCollider*>(collider.get());
	cylinder->height = 7.0f;
	cylinder->radius = 3.5f;
	CollisionManager::Instance().AddObject(this);

	class_name = "Core";
	scale = { 0.3f, 0.3f, 0.3f };
	hp = 1500.0f;
}

Core::~Core()
{
}

void Core::init()
{
	CollisionManager::Instance().AddObject(this);
}

void Core::Update(float elapsedTime)
{
	if (hp <= 0.0f)
	{
		GimmicManager::Instance().Remove(this);
		CollisionManager::Instance().Remove(this);
		SceneManager::Instance().ChangeScene(new SceneLoading(new SceneTitle));
		return;
	}

	animator.Update(elapsedTime);

	if (cylinder)
	{
		cylinder->center = position;
	}
}

void Core::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	renderer->Render(rc, transform, model, ShaderId::Lambert);
}



void Core::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
	if (hp <= 0.0f || !cylinder) return;

	// コアの当たり判定（円柱）を赤色で表示
	renderer->RenderCylinder(
		rc,
		cylinder->center,    // 中心（地面に置く座標）
		cylinder->radius,      // 半径
		cylinder->height,      // 高さ
		XMFLOAT4(1, 0, 0, 1)
	);
}

void Core::OnCollision(GameObject* object)
{
	if (hp <= 0.0f) return;

	if (object->type == Type::PlayerAttack)
	{
		hp -= 10.0f;
	}
}

void Core::OnImGui()
{
	if (ImGui::CollapsingHeader("Core"))
	{
		ImGui::DragFloat("HP", &hp, 0.1f, 0.0f, 1500.0f, "%.1f");
	}
}

REGISTER_GAMEOBJECT(Core);

