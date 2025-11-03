#pragma once
#include <DirectXMath.h>
#include "TownHall.h"
#include "Fence.h"

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

	Fence* SpawnFence(const DirectX::XMFLOAT3& pos,
		float radius = 1.0f,
		float height = 1.2f,
		int   maxHP = 200,
		float angleY = 0.0f
	);


	TownHall* GetTownHall() const { return townHall; }

	int     GetFenceCount() const;              // ò‚Ì”
	Fence* GetFence(int index) const;          // index‚ª”ÍˆÍŠO‚È‚çnullptr
private:
	BuildingManager() = default;
	TownHall* townHall{ nullptr };
	std::vector<std::unique_ptr<Fence>> fences;
};