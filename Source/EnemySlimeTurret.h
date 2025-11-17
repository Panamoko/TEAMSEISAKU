#pragma once
#include "EnemySlime.h"

// 固定砲台型スライム（タワー上の敵）
class EnemySlimeTurret : public EnemySlime
{
public:
    EnemySlimeTurret(); // 配置座標を引数で受け取る
    ~EnemySlimeTurret() override {}

protected:
    // 親クラスの動きを上書きして「動かない」ようにする

    // 徘徊設定（呼ばれても強制的に待機にする）
    void SetWanderState() override;

    // 待機更新（タイマーで移動せず、ひたすら索敵のみ行う）
    void UpdateIdleState(float elapsedTime) override;

    // 攻撃更新（回転はするが移動はしない）
    void UpdateAttackState(float elapsedTime) override;
};