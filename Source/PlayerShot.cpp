#include "PlayerShot.h"
#include "SceneGame.h"

PlayerShot::PlayerShot() {}
PlayerShot::~PlayerShot() {}

void PlayerShot::Initialize()
{
    InitializeCommon("Data/Model/Slime/B_Player.mdl", "Data/Sprite/B_Player.png");
    color = { 1.0f, 1.0f, 1.0f, 1.0f };
}

void PlayerShot::SpawnAlly(SceneGame* scene)
{
    if (scene) scene->AddAllyStraightFor(this);
}