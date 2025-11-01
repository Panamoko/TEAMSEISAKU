#pragma once
#include <DirectXMath.h>
#include "GameObject.h"



// 攻撃対象として扱えるもの（建物・ユニットなど）を共通化
class ITargetable : public GameObject {
public:
	virtual ~ITargetable() = default;
	virtual const DirectX::XMFLOAT3& GetPosition() const = 0; // 世界座標
	virtual float GetRadius() const = 0; // 水平円の半径（XZ 用）
	virtual bool IsAlive() const = 0; // HP>0
	virtual void TakeDamage(int amount) = 0; // 被ダメージ
};