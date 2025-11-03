#include "RayCast.h"
#include <cfloat>
#include <cmath>

#include "EnemyManager.h"
#include "Enemy.h"

using namespace DirectX;

static inline XMVECTOR ToVec(const XMFLOAT3& f)
{
    return XMLoadFloat3(&f);
}
static inline XMFLOAT3 ToFloat3(FXMVECTOR v)
{
    XMFLOAT3 r; XMStoreFloat3(&r, v); return r;
}

RayCast::Ray RayCast::ScreenPointToRay(
    int mouseX, int mouseY, int screenWidth, int screenHeight,
    const XMFLOAT4X4& view, const XMFLOAT4X4& proj)
{
    // NDC 変換（+0.5でピクセル中心寄せ：必要なければ外してOK）
    float ndcX = ((mouseX + 0.5f) / float(screenWidth)) * 2.0f - 1.0f;
    float ndcY = -((mouseY + 0.5f) / float(screenHeight)) * 2.0f + 1.0f; // 画面下→上で+に

    XMMATRIX V = XMLoadFloat4x4(&view);
    XMMATRIX P = XMLoadFloat4x4(&proj);
    XMMATRIX VP = XMMatrixMultiply(V, P);
    XMVECTOR det;
    XMMATRIX invVP = XMMatrixInverse(&det, VP);

    XMVECTOR pNear = XMVectorSet(ndcX, ndcY, 0.0f, 1.0f);
    XMVECTOR pFar = XMVectorSet(ndcX, ndcY, 1.0f, 1.0f);

    XMVECTOR wNear = XMVector4Transform(pNear, invVP);
    XMVECTOR wFar = XMVector4Transform(pFar, invVP);
    wNear = XMVectorScale(wNear, 1.0f / XMVectorGetW(wNear));
    wFar = XMVectorScale(wFar, 1.0f / XMVectorGetW(wFar));

    XMVECTOR dir = XMVector3Normalize(XMVectorSubtract(wFar, wNear));

    Ray ray;
    ray.origin = ToFloat3(wNear);
    ray.dir = ToFloat3(dir);
    return ray;
}

bool RayCast::IntersectPlaneY(const Ray& ray, float planeY, Hit* outHit)
{
    const float EPS = 1e-6f;
    if (fabsf(ray.dir.y) < EPS) return false; // 平行

    float t = (planeY - ray.origin.y) / ray.dir.y;
    if (t < 0.0f) return false;

    if (outHit)
    {
        outHit->t = t;
        XMVECTOR o = ToVec(ray.origin);
        XMVECTOR d = ToVec(ray.dir);
        XMVECTOR p = XMVectorAdd(o, XMVectorScale(d, t));
        outHit->position = ToFloat3(p);
        // 上向き or 下向き：レイの向きに応じて法線を決める
        outHit->normal = (ray.dir.y > 0.0f) ? XMFLOAT3(0, -1, 0) : XMFLOAT3(0, 1, 0);
    }
    return true;
}

bool RayCast::IntersectSphere(
    const Ray& ray, const XMFLOAT3& center, float radius, Hit* outHit)
{
    XMVECTOR O = ToVec(ray.origin);
    XMVECTOR D = ToVec(ray.dir);
    XMVECTOR C = XMLoadFloat3(&center);

    XMVECTOR OC = XMVectorSubtract(O, C);
    float a = XMVectorGetX(XMVector3Dot(D, D));
    float b = 2.0f * XMVectorGetX(XMVector3Dot(OC, D));
    float c = XMVectorGetX(XMVector3Dot(OC, OC)) - radius * radius;

    float disc = b * b - 4 * a * c;
    if (disc < 0.0f) return false;

    float sqrtDisc = sqrtf(disc);
    float t1 = (-b - sqrtDisc) / (2 * a);
    float t2 = (-b + sqrtDisc) / (2 * a);

    float t = FLT_MAX;
    if (t1 > 0.0f) t = t1;
    else if (t2 > 0.0f) t = t2;
    else return false;

    if (outHit)
    {
        outHit->t = t;
        XMVECTOR P = XMVectorAdd(O, XMVectorScale(D, t));
        outHit->position = ToFloat3(P);
        XMVECTOR N = XMVector3Normalize(XMVectorSubtract(P, C));
        outHit->normal = ToFloat3(N);
    }
    return true;
}

bool RayCast::IntersectCylinderY(
    const Ray& ray, const XMFLOAT3& center, float radius, float height, Hit* outHit)
{
    const float EPS = 1e-6f;

    // ローカル空間：円柱中心を原点に
    float oy = ray.origin.y - center.y;
    float ox = ray.origin.x - center.x;
    float oz = ray.origin.z - center.z;
    float dx = ray.dir.x;
    float dy = ray.dir.y;
    float dz = ray.dir.z;

    float halfH = height * 0.5f;

    // 候補（側面／上下キャップ）のうち最も近いものを採用
    bool hit = false;
    Hit best; // t=FLT_MAX 初期化

    // --- 側面 ---
    float a = dx * dx + dz * dz;
    if (a > EPS)
    {
        float b = 2.0f * (ox * dx + oz * dz);
        float c = ox * ox + oz * oz - radius * radius;
        float disc = b * b - 4 * a * c;
        if (disc >= 0.0f)
        {
            float s = sqrtf(disc);
            float t1 = (-b - s) / (2 * a);
            float t2 = (-b + s) / (2 * a);

            auto testT = [&](float tCandidate)
                {
                    if (tCandidate <= 0.0f) return;
                    float y = oy + dy * tCandidate;
                    if (y < -halfH || y > halfH) return; // 高さ範囲外

                    // ヒット確定：ワールドに戻す
                    XMVECTOR O = ToVec(ray.origin);
                    XMVECTOR D = ToVec(ray.dir);
                    XMVECTOR P = XMVectorAdd(O, XMVectorScale(D, tCandidate));
                    XMFLOAT3 pos = ToFloat3(P);

                    // 法線：側面は (x,z) 方向
                    float lx = ox + dx * tCandidate;
                    float lz = oz + dz * tCandidate;
                    float lenXZ = sqrtf(lx * lx + lz * lz);
                    XMFLOAT3 n(0, 0, 0);
                    if (lenXZ > EPS) { n.x = lx / lenXZ; n.z = lz / lenXZ; }

                    Hit h; h.t = tCandidate; h.position = pos; h.normal = n;
                    if (h.t < best.t) { best = h; }
                    hit = true;
                };

            testT(t1);
            testT(t2);
        }
    }

    // --- 上下キャップ ---
    auto testCap = [&](float capY, const XMFLOAT3& capNormal)
        {
            if (fabsf(dy) < EPS) return;

            float t = (capY - oy) / dy; // ローカルYで解く
            if (t <= 0.0f) return;

            float x = ox + dx * t;
            float z = oz + dz * t;
            if (x * x + z * z <= radius * radius + 1e-6f)
            {
                XMVECTOR O = ToVec(ray.origin);
                XMVECTOR D = ToVec(ray.dir);
                XMVECTOR P = XMVectorAdd(O, XMVectorScale(D, t));
                Hit h; h.t = t; h.position = ToFloat3(P); h.normal = capNormal;
                if (h.t < best.t) { best = h; }
                hit = true;
            }
        };

    testCap(+halfH, XMFLOAT3(0, 1, 0));
    testCap(-halfH, XMFLOAT3(0, -1, 0));

    if (hit && outHit) *outHit = best;
    return hit;
}

bool RayCast::PickEnemy(
    const Ray& ray, EnemyManager& enemyManager, Hit* outHit, Enemy** outEnemy)
{
    // Enemy は Character を継承しているので position / radius / height を取得できます
    //（GetPosition / GetRadius / GetHeight） :contentReference[oaicite:3]{index=3} :contentReference[oaicite:4]{index=4}

    int count = enemyManager.GetEnemyCount(); // :contentReference[oaicite:5]{index=5}
    if (count <= 0) return false;

    bool any = false;
    Hit best;
    std::shared_ptr<Enemy> picked = nullptr;

    for (int i = 0; i < count; ++i)
    {
        std::shared_ptr<Enemy> e = enemyManager.GetEnemy(i); // :contentReference[oaicite:6]{index=6}
        const XMFLOAT3& c = e->GetPosition(); // :contentReference[oaicite:7]{index=7}
        float r = e->GetRadius();             // :contentReference[oaicite:8]{index=8}
        float h = e->GetHeight();             // :contentReference[oaicite:9]{index=9}

        Hit hinfo;
        if (IntersectCylinderY(ray, c, r, h, &hinfo))
        {
            if (!any || hinfo.t < best.t)
            {
                any = true;
                best = hinfo;
                picked = e;
            }
        }
    }

    if (any)
    {
        if (outHit)  *outHit = best;
        if (outEnemy)*outEnemy = picked.get();
    }
    return any;
}
