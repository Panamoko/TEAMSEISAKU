#pragma once
// =============================================
// AllySlimeHoming.h
// 追尾弾型 味方スライム（ヒーラー仕様）
// ・隊列を組んでプレイヤーに追従
// ・HPが減っている味方（プレイヤー）を探して回復弾を発射
// =============================================

#include "System/Model.h"
#include "System/ModelRenderer.h"
#include "Character.h"
#include "ProjectileManager.h"
#include "System/Sprite.h"
class Player;
// class Enemy; // 不要になったため削除
struct RenderContext;
class ShapeRenderer;

class AllySlimeHeal : public Character
{
public:
    AllySlimeHeal(int formationIndex, Player* leader);
    ~AllySlimeHeal() override;

    // 公開 API
    void SetIndex(int idx) { index = idx; }
    void SetLeader(Player* p) { leader = p; }
    Player* GetLeader() const { return leader; }

    // 更新／描画
    void Update(float elapsedTime) override;
    void Render(const RenderContext& rc, ModelRenderer* renderer) override;
    void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) override;

    // 回復行動の更新（必要に応じてON/OFFできるよう公開）
    void UpdateHealing(float elapsedTime);

    void RenderUI(const RenderContext& rc, float x, float y, float size);
    void OnDead() override;


private:
    void UpdateAnchor();

private:
    // ===== 編隊・追従パラメータ =====
    int   index = 0;               // 編隊内スロット番号
    int   rowWidth = 3;            // 1列あたり
    float followDistance = 1.2f;   // 前後間隔
    float lateralSpacing = 0.9f;   // 左右間隔
    float moveSpeed = 5.0f;        // 追従移動速度
    float turnSpeed = 6.0f;        // 追従回頭速度

    // ===== 自動回復設定 =====
    bool  autoHealEnabled = true;     // 自動回復 ON/OFF
    float autoHealRange = 15.0f;      // 索敵半径（回復なので少し広めに設定）
    float autoHealInterval = 2.0f;    // 発射間隔（秒）
    float autoHealTimer = 0.0f;       // クールダウンタイマー

    // ===== 見た目・弾管理 =====
    DirectX::XMFLOAT3 anchor = { 0,0,0 };
    Model* slimeModel = nullptr;
    ProjectileManager projectileManager;   // 自前で所有

    // ===== 編隊リーダー =====
    Player* leader = nullptr; // null の場合は Player::Instance() を参照

    Sprite* icon = nullptr;
    Sprite* hpBarSprite = nullptr;
};