#pragma once
//
// AllySlime.h
// プレイヤーの後方で隊列を組みつつ追従し、敵を攻撃する味方スライム。
//

#include "System/Model.h"
#include "System/ModelRenderer.h"
#include "Character.h"
#include "ProjectileManager.h"
#include "System/Sprite.h"
#include "Animator.h"  // ★追加: これがないと Animator 型が使えません

class Player; // 前方宣言

class AllySlime : public Character
{
public:
    // formationIndex: 隊列内でのスロット番号（0 始まり）
    explicit AllySlime(int formationIndex, Player* leader);
    ~AllySlime() override {}

    // メインフレーム更新処理
    void Update(float elapsedTime);
    void Render(const RenderContext& rc, ModelRenderer* renderer);
    void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) override;

    void SetIndex(int idx) { index = idx; } // 隊列での配置を入れ替える際に使用

    // リーダーの設定／取得
    void SetLeader(Player* p) { leader = p; }
    Player* GetLeader() const { return leader; }

    // 衝突処理のオーバーライド
    void OnCollision(GameObject* object) override;

    // 全味方スライムのリスト管理用
    static void RegisterAlly(Character* ally);
    static void UnregisterAlly(Character* ally);
    static const std::vector<Character*>& GetAllAllies();

    void RenderUI(const RenderContext& rc, float x, float y, float size);

    void OnDead() override;

protected: // ★private ではなく protected にすることで、継承したクラス(MeleeやHeal)からもアクセスできるようにします

    // ★追加: アニメーション管理クラス
    Animator animator;

    // ★追加: 回復などのアクション中かどうかを管理するフラグ
    bool isAction = false;

private:
    // 内部処理: 隊列アンカーや自動攻撃、衝突判定を更新
    void UpdateAnchor();
    void AutoAttackUpdate(float elapsedTime);
    void CollisionProjectilesVsEnemies();

private:
    // ====== 隊列（フォーメーション）パラメータ ======
    int   index = 0;                          // 自身の隊列インデックス
    int   rowWidth = 3;                       // 横方向の人数（例: 4 なら 0..3 が 1 行目）
    float followDistance = 1.2f;              // 縦方向の間隔
    float lateralSpacing = 0.9f;              // 横方向の間隔
    float moveSpeed = 5.0f;                   // 追従時の最大移動速度
    float turnSpeed = DirectX::XMConvertToRadians(540); // 旋回速度（度/秒を弧度法に変換）

    // ====== オートアタック設定（味方行動の制御） ======
    bool  autoAttackEnabled = true;           // ON / OFF
    float autoAttackRange = 8.0f;             // 射程
    float autoAttackInterval = 1.5f;          // 攻撃間隔（秒）
    float autoAttackTimer = 0.0f;             // クールダウンタイマー

    // ====== 隊列アンカー関連 ======
    DirectX::XMFLOAT3 anchor = { 0,0,0 };     // プレイヤー周辺に設定する追従基準位置
    Model* slimeModel = nullptr;              // 描画用モデル（味方スライム）

    // ProjectileManager を直接メンバに持ち、生成/更新/描画を一括管理
    ProjectileManager projectileManager;      // 発射体の生成・更新・描画管理

    // 追従対象のプレイヤー（null の場合は Player::Instance() を利用）
    Player* leader = nullptr;

    // 味方スライムアイコン（UI描画用）
    Sprite* icon = nullptr;
    Sprite* hpBarSprite = nullptr;

    static std::vector<Character*> s_allies;
};