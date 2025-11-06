#pragma once
#include <DirectXMath.h>
#include "ITargetable.h"
#include "ModelManager.h"
#include "Animator.h"

struct RenderContext;    
class ShapeRenderer;     

// 必要に応じてモデル系を使う場合は開放側で include：
// #include "static_mesh.h"
// #include "texture.h"


class TownHall final : public ITargetable {
public:
	TownHall();
	~TownHall();

	void Initialize();
	void Update(float dt);
	void Render(const RenderContext& rc, ModelRenderer* renderer);
	void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer);


	// ITargetable 実装
	const DirectX::XMFLOAT3& GetPosition() const override { return position; }
	float GetRadius() const override { return radius; }
	bool IsAlive() const override { return hp > 0; }
	void TakeDamage(int amount) override;

	// これを追加（publicに1つだけ）
	float GetHeight() const { return height; }

	// 追加ユーティリティ
	bool IsDestroyed() const { return hp <= 0; }
	int GetHP() const { return hp; }
	int GetMaxHP() const { return maxHP; }

	void OnCollision(GameObject* object)override;


private:
	DirectX::XMFLOAT3 position{}; // 中心（地面上）

	float radius{}; // 円柱の半径（XZ判定）
	float height{ 6.0f }; // デバッグ用の可視高さ（必要に応じて）

	int maxHP{};
	int hp{};

	Model* model = nullptr;
	Animator animator;               // 追加
	bool     playingDeath = false;   // 破壊演出中フラグ
	CylinderCollider* cylinder;
	// モデルを使うならここに保持（例）
	// StaticMesh* mesh{nullptr};
	// Texture* tex{nullptr};
};