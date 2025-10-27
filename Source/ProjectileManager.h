#pragma once

#include <vector>
#include <set>
#include "Projectile.h"

// ’eŠÛƒ}ƒl[ƒWƒƒ[
class ProjectileManager
{
public:
	ProjectileManager();
	~ProjectileManager();

	// XVˆ—
	void Update(float elapsedTime);

	// •`‰æˆ—
	void Render(const RenderContext& rc, ModelRenderer* renderer);

	// ƒfƒoƒbƒOƒvƒŠƒ~ƒeƒBƒu•`‰æ
	void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer);

	// ’eŠÛ“o˜^
	void Register(Projectile* projectile);

	// ’eŠÛ‘Síœ
	void Clear();

	// ’eŠÛ”æ“¾
	int GetProjectileCount() const { return static_cast<int>(projectiles.size()); }

	// ’eŠÛæ“¾
	Projectile* GetProjectile(int index) { return projectiles.at(index); }

	//’eŠÛ“o˜^
	void Remove(Projectile* projectile);
private:
	std::vector<Projectile*>	projectiles;
	std::set<Projectile*>		removes;
};
