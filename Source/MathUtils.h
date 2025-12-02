#pragma once
#include <DirectXMath.h>

//浮動小数算術
class MathUtils
{
public:
	//指定のランダム値を計算する
	static float RandomRenge(float min, float max);

    // XZ平面での距離の2乗を返す（高速）
    static float DistSqXZ(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b) {
        float dx = a.x - b.x;
        float dz = a.z - b.z;
        return dx * dx + dz * dz;
    }
};

