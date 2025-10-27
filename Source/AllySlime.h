#pragma once
//
// AllySlime.h
// プレイヤーの「後方に整列して追従」し、敵を自動攻撃する“ちびスライム”。
// ・隊列（編隊）アンカーをプレイヤー後方に敷き、そこへ追従
// ・最寄りの敵が射程に入ったら、プレイヤー同様の直進弾を発射
// ・弾の更新／当たり判定は AllySlime 内で完結
//

#include "System/Model.h"
#include "System/ModelRenderer.h"
#include "Character.h"          // ※環境によって "character.h" かも。プロジェクトに合わせてください。
#include "ProjectileManager.h"  // プレイヤーでも使っている弾マネージャ

class AllySlime : public Character
{
public:
    // formationIndex: 編隊内でのスロット番号（0始まり）
    explicit AllySlime(int formationIndex);
    ~AllySlime() override {}

    // 毎フレーム更新／描画
    void Update(float elapsedTime);
    void Render(const RenderContext& rc, ModelRenderer* renderer);
    void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) override;

    void SetIndex(int idx) { index = idx; } // 途中で並び替えたいとき用

private:
    // 内部処理：編隊アンカー更新／自動攻撃更新／弾と敵の当たり判定
    void UpdateAnchor();
    void AutoAttackUpdate(float elapsedTime);
    void CollisionProjectilesVsEnemies();

private:
    // ====== 隊列（編隊）パラメータ ======
    int   index = 0;                          // 自分の編隊インデックス
    int   rowWidth = 4;                       // 横に並べる体数（例：4なら 0..3 が1列目）
    float followDistance = 1.2f;              // 縦方向（前後）の間隔
    float lateralSpacing = 0.9f;              // 横方向の間隔
    float moveSpeed = 4.0f;                   // 追従時の最大移動速度
    float turnSpeed = DirectX::XMConvertToRadians(540); // 追従時の旋回速度（度/秒 → ラジアン）

    // ====== オートアタック設定（プレイヤー既定に合わせた初期値） ======
    bool  autoAttackEnabled = true;         // ON/OFF
    float autoAttackRange = 8.0f;         // 射程
    float autoAttackInterval = 1.5f;         // 連射間隔（秒）
    float autoAttackTimer = 0.0f;         // 残りクールダウン

    // ====== 見た目と内部状態 ======
    DirectX::XMFLOAT3 anchor = { 0,0,0 };       // 追従ターゲット（プレイヤー後方の“目標座標”）
    Model* slimeModel = nullptr;              // 表示用モデル（敵スライムと共用）

    // ★これが無いと「projectileManager が未定義」エラーになります
    ProjectileManager projectileManager;      // 自身の弾管理（生成/更新/描画/破棄）
};
