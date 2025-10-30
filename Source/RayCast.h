#pragma once
#include <DirectXMath.h>

class Enemy;
class EnemyManager;

class RayCast
{
public:
    struct Ray
    {
        DirectX::XMFLOAT3 origin; // ワールド空間
        DirectX::XMFLOAT3 dir;    // 正規化済み（ワールド空間）
    };

    struct Hit
    {
        float t = FLT_MAX;                 // origin からの距離（レイパラメータ）
        DirectX::XMFLOAT3 position{ 0,0,0 }; // ヒット座標（ワールド）
        DirectX::XMFLOAT3 normal{ 0,1,0 };   // 法線（ワールド）
    };

    // 画面座標(px)からレイを生成（DirectX の右手NDC想定、Yは上が+）
    // view, proj はカメラの行列（XMFLOAT4X4）。必要ならカメラから取得して渡す。
    static Ray ScreenPointToRay(
        int mouseX, int mouseY, int screenWidth, int screenHeight,
        const DirectX::XMFLOAT4X4& view,
        const DirectX::XMFLOAT4X4& proj);

    // 任意高さの水平面（Y=planeY）と交差
    static bool IntersectPlaneY(
        const Ray& ray, float planeY, Hit* outHit);

    // 球と交差
    static bool IntersectSphere(
        const Ray& ray,
        const DirectX::XMFLOAT3& center, float radius,
        Hit* outHit);

    // Y軸円柱（center を中心、半径 radius、高さ height）と交差
    // 側面＋上下キャップをテスト
    static bool IntersectCylinderY(
        const Ray& ray,
        const DirectX::XMFLOAT3& center, float radius, float height,
        Hit* outHit);

    // シーン中の一番近い Enemy を拾う（EnemyManager を走査）
    // 円柱ヒットで最近点を返す
    static bool PickEnemy(
        const Ray& ray, EnemyManager& enemyManager,
        Hit* outHit, Enemy** outEnemy);
};
