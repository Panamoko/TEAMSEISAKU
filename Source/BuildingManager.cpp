#include "BuildingManager.h"
#include "System/ShapeRenderer.h"
#include "System/RenderContext.h"
using namespace DirectX;


BuildingManager& BuildingManager::Instance() {
	static BuildingManager s; return s;
}


void BuildingManager::Initialize() {
	// とりあえず中央に 1 棟スポーン（ゲーム開始時に Scene から明示でもOK）
	if (!townHall) {
		SpawnTownHall(XMFLOAT3{ 0, 0, 0 });
	}
}


TownHall* BuildingManager::SpawnTownHall(const XMFLOAT3& pos, float radius, int maxHP) {
	if (townHall) return townHall.get(); // 1棟制御（必要なら複数対応へ拡張）
	townHall = std::make_unique<TownHall>();
	townHall->Initialize(pos, radius, maxHP);
	return townHall.get();
}

Fence* BuildingManager::SpawnFence(const XMFLOAT3& pos, float radius, float height, int maxHP, float angleY) {
	auto f = std::make_unique<Fence>(pos, radius, height, maxHP);
	f->Initialize();
	f->SetAngleY(angleY);
	Fence* raw = f.get();
	fences.emplace_back(std::move(f));
	return raw;
}

int BuildingManager::GetFenceCount() const {
	return static_cast<int>(fences.size());
}

Fence* BuildingManager::GetFence(int index) const {
	if (index < 0 || index >= static_cast<int>(fences.size())) return nullptr;
	return fences[index].get();
}

void BuildingManager::Update(float dt) {
	if (townHall) townHall->Update(dt);
	for (auto& f : fences) f->Update(dt);
}


void BuildingManager::Render(const RenderContext& rc, ModelRenderer* renderer) {
	if (townHall) townHall->Render(rc, renderer);
	for (auto& f : fences) f->Render(rc, renderer);
}


void BuildingManager::DebugDraw(const RenderContext& rc, ShapeRenderer* renderer)
{
	if (townHall) townHall->RenderDebugPrimitive(rc, renderer);
	for (auto& f : fences) f->RenderDebugPrimitive(rc, renderer);
}