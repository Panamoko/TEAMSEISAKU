#pragma once
#include <DirectXMath.h>

class EnemyManager;
class Enemy;
class GimmicBase;

namespace AllyTargeting
{
    enum class Kind { None, Core, BreakWall, Enemy };

    struct TargetInfo {
        Kind kind = Kind::None;
        DirectX::XMFLOAT3 pos{ 0,0,0 };  // íeÇ™ë_Ç§ç¿ïWÅiÇ‚Ç‚è„îºêgÇë_Ç§Åj
        Enemy* enemy = nullptr;         // Enemy ÇÃÇ∆Ç´ÇÃÇ›
        GimmicBase* gimmick = nullptr;  // Core / BreakWall ÇÃÇ∆Ç´ÇÃÇ›
        float distSq = FLT_MAX;
        explicit operator bool() const { return kind != Kind::None; }
    };

    // óDêÊìx: Core > BreakWall > EnemyÅirange à»ì‡ÇÃÇ›Åj
    TargetInfo FindBestTarget(const DirectX::XMFLOAT3& selfPos,
        float range,
        EnemyManager* enemyMgr = nullptr);
}
