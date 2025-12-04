#pragma once
#include <string>
#include "System/Model.h"

using namespace DirectX;

class Animator {
public:
    void SetModel(Model* m) { model = m; }

    // 名前で再生
    // blendTime を引数に追加
    void Play(const char* name, bool loop, float blendTime = 0.2f) {
        if (!model) return;
        int idx = 0;
        const auto& anims = model->GetResource()->GetAnimations();
        for (const auto& a : anims) {
            if (a.name == name) {
                Play(idx, loop, blendTime);
                return;
            }
            ++idx;
        }
    }

    // インデックスで再生
    void Play(int index, bool loop, float blendSeconds = 0.2f) {
        // 同じアニメーションを再生中ならリセットしない（重要）
        if (playing && animIndex == index) {
            looping = loop; // ループ設定のみ更新
            return;
        }

        playing = true;
        looping = loop;
        animIndex = index;
        seconds = 0.0f;

        // ブレンド開始
        currentBlendTime = 0.0f;
        blendDuration = blendSeconds;
    }

    // ★復活: 以前のコードとの互換性のために追加
    void SetBlendSeconds(float s) { blendDuration = s; }

    // ★復活: 現在時間を取得
    float GetCurrentSeconds() const { return seconds; }

    // 更新処理
    void Update(float dt) {
        if (!model || !playing || animIndex < 0) return;

        const auto& anims = model->GetResource()->GetAnimations();
        if (animIndex >= anims.size()) return;
        const auto& anim = anims.at(animIndex);

        // 時間進行
        seconds += dt;
        currentBlendTime += dt; // ブレンド用時間は常に進める

        // アニメーション再生位置の計算
        float currentAnimTime = seconds;
        if (seconds >= anim.secondsLength) {
            if (looping) {
                currentAnimTime = fmod(seconds, anim.secondsLength);
            }
            else {
                currentAnimTime = anim.secondsLength;
                // ループしない場合は再生終了とみなすが、最後のポーズは維持
                playing = false;
            }
        }

        // ブレンド率計算 (0.0 -> 1.0)
        float blend = 1.0f;
        if (blendDuration > 0.0f && currentBlendTime < blendDuration) {
            blend = currentBlendTime / blendDuration;
            // イージング（滑らかに変化）
            blend = blend * blend * (3.0f - 2.0f * blend);
        }

        // キーフレーム補間計算
        const auto& keys = anim.keyframes;
        const int keyCount = static_cast<int>(keys.size());

        // 時刻クランプ
        if (currentAnimTime > anim.secondsLength) currentAnimTime = anim.secondsLength;

        for (int i = 0; i < keyCount - 1; ++i) {
            const auto& k0 = keys[i];
            const auto& k1 = keys[i + 1];

            if (currentAnimTime >= k0.seconds && currentAnimTime <= k1.seconds) {
                float duration = k1.seconds - k0.seconds;
                float rate = (duration > 0.0f) ? (currentAnimTime - k0.seconds) / duration : 0.0f;

                auto& nodes = model->GetNodes();
                const int nodeCount = static_cast<int>(nodes.size());

                for (int n = 0; n < nodeCount; ++n) {
                    auto& node = nodes[n];
                    // リソース側のキー数が足りているかチェック
                    if (n >= k0.nodeKeys.size() || n >= k1.nodeKeys.size()) continue;

                    const auto& nk0 = k0.nodeKeys[n];
                    const auto& nk1 = k1.nodeKeys[n];

                    // ターゲット姿勢（新しいアニメーションの本来の姿）
                    XMVECTOR targetS = XMVectorLerp(XMLoadFloat3(&nk0.scale), XMLoadFloat3(&nk1.scale), rate);
                    XMVECTOR targetR = XMQuaternionSlerp(XMLoadFloat4(&nk0.rotate), XMLoadFloat4(&nk1.rotate), rate);
                    XMVECTOR targetT = XMVectorLerp(XMLoadFloat3(&nk0.translate), XMLoadFloat3(&nk1.translate), rate);

                    // ブレンド処理: 現在の姿勢(node) から ターゲット姿勢(target) へ徐々に移行
                    if (blend < 1.0f) {
                        XMVECTOR currentS = XMLoadFloat3(&node.scale);
                        XMVECTOR currentR = XMLoadFloat4(&node.rotate);
                        XMVECTOR currentT = XMLoadFloat3(&node.translate);

                        XMStoreFloat3(&node.scale, XMVectorLerp(currentS, targetS, blend));
                        XMStoreFloat4(&node.rotate, XMQuaternionSlerp(currentR, targetR, blend));
                        XMStoreFloat3(&node.translate, XMVectorLerp(currentT, targetT, blend));
                    }
                    else {
                        // ブレンド終了後はターゲット姿勢をそのまま適用
                        XMStoreFloat3(&node.scale, targetS);
                        XMStoreFloat4(&node.rotate, targetR);
                        XMStoreFloat3(&node.translate, targetT);
                    }
                }
                break;
            }
        }

        model->UpdateTransform();
    }

    bool IsPlaying() const { return playing; }

private:
    Model* model = nullptr;
    int    animIndex = -1;
    float  seconds = 0.0f;
    bool   looping = false;
    bool   playing = false;

    // ブレンド用変数
    float  currentBlendTime = 0.0f;
    float  blendDuration = 0.2f;
};