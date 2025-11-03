#pragma once
#include <DirectXMath.h>
#include "TownHall.h"

struct RenderContext;    
class ShapeRenderer;     

class BuildingManager {
public:
	static BuildingManager& Instance();


	void Initialize();
	void Update(float dt);
	void Render(const RenderContext& rc, ModelRenderer* renderer);
	void DebugDraw(const RenderContext& rc, ShapeRenderer* renderer);


	TownHall* SpawnTownHall(const DirectX::XMFLOAT3& pos,
		float radius = 3.0f,
		int maxHP = 1500);


	TownHall* GetTownHall() const { return townHall; }


private:
	BuildingManager() = default;
	TownHall* townHall{ nullptr };
};