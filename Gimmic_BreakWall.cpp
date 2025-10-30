#include "Gimmic_BreakWall.h"
#include "Factory.h"

Gimmic_BreakWall::Gimmic_BreakWall()
{
	class_name = "Gimmic_BreakWall";
	model = ModelManager::Instance().Load("Data/Model/bilud/saku.mdl");
}

//攻撃が当たったときに呼ばれる
void Gimmic_BreakWall::OnTrigger(GameObject* objects)
{
	if (objects->type == Type::Player && !isBroken && hp > 0.0f)
	{
		hp--;
	}
	if (hp <= 0.0f && !isBroken)
	{
		isBroken = true;
		delete model;
	}
}

void Gimmic_BreakWall::Update(float elapsedTime)
{
	//壁のAABB（軸平行境界ボックス）を求める
	halfSize = { scale.x * 0.5f,scale.y * 0.5f,scale.z * 0.5f };
	boxMin = {
		position.x - halfSize.x,
		position.y - halfSize.y,
		position.z - halfSize.z
	};
	boxMax = {
		position.x + halfSize.x,
		position.y + halfSize.y,
		position.z + halfSize.z
	};
}

REGISTER_GAMEOBJECT(Gimmic_BreakWall);

