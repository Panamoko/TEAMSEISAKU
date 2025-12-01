#pragma once

#include "EnemySlime.h"

// スライム (近距離攻撃型)
class EnemySlimeMelee : public EnemySlime
{
public:
	EnemySlimeMelee();
	~EnemySlimeMelee() override;

protected:
	// ★ 基底クラスの攻撃ロジックを上書き (オーバーライド)

	//攻撃ステートへ偏移
	void SetAttackState(Character* target) override;

	//攻撃ステート更新処理
	void UpdateAttackState(float elapsedTime) override;


private:
	// 近距離型専用のパラメータ
	float	attackRange = 1.5f; // 攻撃が届く距離 (例: 1.5m)
	float	attackInterval = 1.0f; // 攻撃間隔 (例: 1秒)
	int		attackDamage = 1; // 攻撃力


};