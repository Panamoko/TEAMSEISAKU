#pragma once
#include "Player.h"

class PlayerHeal : public Player // š public‚ğ‚Â‚¯‚é
{
public:
    PlayerHeal();
    ~PlayerHeal() override;

    void Initialize() override;
    void SpawnAlly(SceneGame* scene) override;
};