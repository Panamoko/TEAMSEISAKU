#pragma once
#include "Character.h"
#include "System/Model.h"
#include "System/ModelRenderer.h"

class Player;
class Enemy;
class GimmicBase;

class AllySlimeMelee : public Character
{
public:
    AllySlimeMelee(int formationIndex);
    ~AllySlimeMelee() override;

    // 基本更新・描画
    void Update(float elapsedTime) override;
    void Render(const RenderContext& rc, ModelRenderer* renderer) override;
    void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) override;

    // 隊列管理用
    void SetIndex(int idx) { index = idx; }
    void SetLeader(Player* p) { leader = p; }
    Player* GetLeader() const { return leader; }

private:
    // 状態管理
    enum class State {
        Follow, // プレイヤー追従（隊列）
        Chase,  // 敵へ接近
        Attack, // 攻撃モーション（体当たり）
        Return  // 隊列へ復帰
    };

    void UpdateAnchor(); // 隊列位置計算
    void SearchTarget(); // 攻撃対象を探す
    void UpdateState(float elapsedTime); // 状態ごとの挙動

    // 衝突・ダメージ判定
    void CheckAttackCollision();

private:
    // --- パラメータ ---
    int   index = 0;
    int   rowWidth = 3;
    float followDistance = 1.2f;
    float lateralSpacing = 0.9f;
    float moveSpeed = 5.0f;      // 移動速度
    float turnSpeed = 10.0f;     // 旋回速度

    // --- 戦闘用 ---
    State state = State::Follow;
    float searchRange = 8.0f;    // プレイヤー中心の索敵範囲
    float attackRange = 1.5f;    // 攻撃届く距離
    float damageCooldown = 1.0f; // 攻撃間隔
    float attackTimer = 0.0f;    // タイマー
    int   attackDamage = 2;      // ダメージ量

    // ターゲット情報
    // 敵かギミックのどちらかを保持する
    std::weak_ptr<Enemy> targetEnemy;    // Character継承の敵
    std::weak_ptr<GimmicBase> targetGimmic;  // ギミック(壁・コア)
    DirectX::XMFLOAT3 targetPos = { 0,0,0 }; // ターゲットの場所

    // --- 見た目 ---
    DirectX::XMFLOAT3 anchor = { 0,0,0 };
    Model* slimeModel = nullptr;
    Player* leader = nullptr;
};