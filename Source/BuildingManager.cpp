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
	if (townHall) return townHall; // 1棟制御（必要なら複数対応へ拡張）
	townHall = new TownHall(pos, radius, maxHP);
	townHall->Initialize();
	return townHall;
}


void BuildingManager::Update(float dt) {
	if (townHall) townHall->Update(dt);
}


void BuildingManager::Render() {
	if (townHall) townHall->Render();
}


void BuildingManager::DebugDraw(const RenderContext& rc, ShapeRenderer* renderer)
{
	if (townHall) {
		townHall->RenderDebugPrimitive(rc, renderer);
	}
}