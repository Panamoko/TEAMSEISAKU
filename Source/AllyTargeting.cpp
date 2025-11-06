//#include "AllyTargeting.h"
//#include <cfloat>
//#include <cmath>
//#include "EnemyManager.h"
//#include "TownHall.h"
//
//// あなたの環境のコア参照に合わせて BuildingManager 等を include
//// 例:
//// #include "BuildingManager.h"
//
//using namespace DirectX;
//
//TownHall* GetCore()
//{
//    extern TownHall * gTownHall; // ← ここは実環境に合わせてください
//    return gTownHall;
//}
//
//bool FindBestTargetPos_EnemyOrCore(const XMFLOAT3& self, float range, XMFLOAT3& outTarget)
//{
//    const float r2 = range * range;
//    float bestD2 = FLT_MAX;
//    bool  found = false;
//
//    // 1) 敵から探索
//    auto& em = EnemyManager::Instance();
//    const int n = em.GetEnemyCount();
//    for (int i = 0; i < n; ++i) {
//        auto e = em.GetEnemy(i);
//        if (!e) continue;
//        XMFLOAT3 p = e->GetPosition();
//        p.y += e->GetHeight() * 0.5f; // 胴体あたりを狙う
//        float dx = p.x - self.x, dy = p.y - self.y, dz = p.z - self.z;
//        float d2 = dx * dx + dy * dy + dz * dz;
//        if (d2 <= r2 && d2 < bestD2) { bestD2 = d2; outTarget = p; found = true; }
//    }
//
//    // 2) コア（TownHall）も候補に
//    if (auto core = GetCore()) {
//        if (core->IsAlive()) {
//            XMFLOAT3 p = core->GetPosition();
//            p.y += core->GetHeight() * 0.5f;
//            float dx = p.x - self.x, dy = p.y - self.y, dz = p.z - self.z;
//            float d2 = dx * dx + dy * dy + dz * dz;
//            if (d2 <= r2 && d2 < bestD2) { bestD2 = d2; outTarget = p; found = true; }
//        }
//    }
//    return found;
//}
