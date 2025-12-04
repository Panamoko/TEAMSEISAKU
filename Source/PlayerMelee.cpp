#include "PlayerMelee.h"
#include "SceneGame.h"

PlayerMelee::PlayerMelee() {}
PlayerMelee::~PlayerMelee() {}

void PlayerMelee::Initialize()
{
    InitializeCommon("Data/Model/Slime/R_Player.mdl");
    color = { 1.0f, 1.0f, 1.0f, 1.0f };
}

void PlayerMelee::SpawnAlly(SceneGame* scene)
{
    if (scene) scene->AddAllyMeleeFor(this);
}