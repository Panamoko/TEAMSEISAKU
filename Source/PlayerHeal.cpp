#include "PlayerHeal.h"
#include "SceneGame.h"

PlayerHeal::PlayerHeal() {}
PlayerHeal::~PlayerHeal() {}

void PlayerHeal::Initialize()
{
    InitializeCommon("Data/Model/Slime/G_Player.mdl", "Data/Sprite/G_Player.png");
    color = { 1.0f, 1.0f, 1.0f, 1.0f };
}

void PlayerHeal::SpawnAlly(SceneGame* scene)
{
    if (scene) scene->AddAllyHomingFor(this);
}