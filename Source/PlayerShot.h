#pragma once
#include "Player.h"

class PlayerShot : public Player // ★ publicをつける
{
public:
    PlayerShot();
    ~PlayerShot() override; // デストラクタも定義しておくのが安全

    void Initialize() override;
    void SpawnAlly(SceneGame* scene) override;
};