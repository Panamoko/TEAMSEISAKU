#include "MathUtils.h"
#include <stdlib.h>



//指定のランダム値を計算する
float MathUtils::RandomRenge(float min, float max)
{
	//0.0f～1.0fまでの間のランダム値
	float value = static_cast<float>(rand()) / RAND_MAX;

	//min～maxまでのランダム値に変換
	return min + (max - min) * value;
}
