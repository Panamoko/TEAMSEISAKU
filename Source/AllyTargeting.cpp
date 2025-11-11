#include "AllyTargeting.h"
#include <cfloat>
#include <cmath>
#include "GimmicManager.h"
#include "Gimmic_BreakWall.h"
#include "Core.h"
#include "EnemyManager.h"
#include "Enemy.h"

using namespace DirectX;

namespace {
    inline float DistSq(const XMFLOAT3& a, const XMFLOAT3& b) {
        const float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
        return dx * dx + dy * dy + dz * dz;
    }
    inline XMFLOAT3 AimUpper(const XMFLOAT3& p, float up = 0.5f) {
        return XMFLOAT3{ p.x, p.y + up, p.z };
    }
}

AllyTargeting::TargetInfo AllyTargeting::FindBestTarget(const XMFLOAT3& selfPos,
    float range,
    EnemyManager* enemyMgr)
{
    TargetInfo best;
    const float rangeSq = range * range;

    // 1) Core Çç≈óDêÊ
    for (auto& g : GimmicManager::Instance().GetAll()) {
        if (!g) continue;
        if (auto* core = dynamic_cast<Core*>(g.get())) {
            const XMFLOAT3 pos = core->GetPosition();
            const float d2 = DistSq(selfPos, pos);
            if (d2 <= rangeSq && d2 < best.distSq) {
                best.kind = Kind::Core;
                best.gimmick = core;
                best.pos = AimUpper(pos, 0.7f);
                best.distSq = d2;
            }
        }
    }
    if (best) return best;

    // 2) BreakWall
    for (auto& g : GimmicManager::Instance().GetAll()) {
        if (!g) continue;
        if (auto* wall = dynamic_cast<Gimmic_BreakWall*>(g.get())) {
            const XMFLOAT3 pos = wall->GetPosition();
            const float d2 = DistSq(selfPos, pos);
            if (d2 <= rangeSq && d2 < best.distSq) {
                best.kind = Kind::BreakWall;
                best.gimmick = wall;
                best.pos = AimUpper(pos, 0.3f); // íÜêSÇ‚Ç‚è„
                best.distSq = d2;
            }
        }
    }
    if (best) return best;

    // 3) EnemyÅiè]óàí ÇËÇÃÅgç≈ãﬂê⁄ÅhÅj
    if (enemyMgr) {
        const int n = enemyMgr->GetEnemyCount();
        for (int i = 0; i < n; ++i) {
            auto e = enemyMgr->GetEnemy(i);
            if (!e) continue;
            const XMFLOAT3 pos = e->GetPosition();
            const float d2 = DistSq(selfPos, pos);
            if (d2 <= rangeSq && d2 < best.distSq) {
                best.kind = Kind::Enemy;
                best.enemy = e.get();
                // è„îºêgÇë_Ç§
                best.pos = AimUpper(pos, e->GetHeight() * 0.5f);
                best.distSq = d2;
            }
        }
    }
    return best; // None or Enemy
}
