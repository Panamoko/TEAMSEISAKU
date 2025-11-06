#pragma once
#include <DirectXMath.h>
#include "ITargetable.h"
#include "ModelManager.h"
#include "Collision.h"

struct RenderContext;
class ShapeRenderer;
class ModelRenderer;

class Fence final : public ITargetable {
public:
    Fence(
        const DirectX::XMFLOAT3& pos,
        float radius = 1.0f,
        float height = 1.2f,
        int   maxHP = 200)
        : position(pos), radius(radius), height(height),
        maxHP(maxHP), hp(maxHP) 
    {}

    void Initialize() {
        // プロジェクトの実体配置に合わせてパスを調整
        // 例: "Data/Model/fence/saku.mdl" に配置
        model = ModelManager::Instance().Load("Data/Model/bilud/saku.mdl");
        scale = { 0.05f, 0.05f, 0.05f };

        halfX = 2.75f;   // 柵の横方向 半径
        halfY = 0.75f;   // 柵の高さ/2（モデルに合わせて）
        halfZ = 0.10f;   // 柵の奥行き/2（細長い板を想定）
        angleY = 0.0f;   // Yaw 初期角
    }

    void Update(float dt) { /* アニメ無しなら空でOK */ }

    void Render(const RenderContext& rc, ModelRenderer* renderer);

    void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer);

    OBB GetOBB() const {
        OBB b{};
        b.center = position;
        b.half = { halfX, halfY, halfZ }; // モデルに合わせて調整
        b.yaw = angleY;                  // ラジアン
        return b;
    }

    // ITargetable
    const DirectX::XMFLOAT3& GetPosition() const override { return position; }
    float GetRadius() const override { return radius; }
    bool IsAlive() const override { return hp > 0; }
    void TakeDamage(int amount) override {
        if (hp <= 0) return;
        hp -= amount;
        if (hp < 0) hp = 0;
    }

    void SetAngleY(float radian) { angleY = radian; } // Y軸周りの角度をラジアンで設定

    int GetHP() const { return hp; }
    int GetMaxHP() const { return maxHP; }

public:
    float halfX{ 0 }, halfY{ 0 }, halfZ{ 0 };
    float angleY{ 0 }; // ラジアン（Yaw）

private:
    DirectX::XMFLOAT3 position{};
    DirectX::XMFLOAT3 scale{ 1,1,1 };
    DirectX::XMFLOAT3 angle{ 0,0,0 }; // 必要なら
    float radius{};
    float height{};
    int   maxHP{};
    int   hp{};
    Model* model{ nullptr };
};
