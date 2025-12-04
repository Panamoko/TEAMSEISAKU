#pragma once
#include "Player.h"

class PlayerMelee : public Player
{
public:
    PlayerMelee();
    ~PlayerMelee() override;

    void Initialize() override;
    void SpawnAlly(SceneGame* scene) override;
};