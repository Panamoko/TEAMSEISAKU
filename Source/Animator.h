#pragma once
#include <string>
#include "System/Model.h"          // AnimationScene と同じ Model / ModelResource を想定

using namespace DirectX;
class Animator {
public:
    void SetModel(Model* m) { model = m; }

    // クリップ再生
    void Play(int index, bool loop) {
        playing = true; looping = loop; animIndex = index; seconds = 0.0f;
    }
    void Play(const char* name, bool loop) {
        if (!model) return;
        int idx = 0;
        const auto& anims = model->GetResource()->GetAnimations();
        for (const auto& a : anims) {
            if (a.name == name) { Play(idx, loop); return; }
            ++idx;
        }
    }

    // 経過時間更新＋ノード補間
    void Update(float dt) {
        if (!model || !playing || animIndex < 0) return;

        // ブレンド率（切替直後のスムーズ化）
        float blend = 1.0f;
        if (seconds < blendLen) {
            blend = seconds / blendLen;
            blend *= blend; // 二乗で立ち上がりを滑らかに
        }

        // アニメ取得
        const auto& anims = model->GetResource()->GetAnimations();
        const auto& anim = anims.at(animIndex);

        // 時間進行
        seconds += dt;
        if (seconds >= anim.secondsLength) {
            if (looping) seconds = 0.0f;
            else { playing = false; seconds = anim.secondsLength; }
        }

        // キーフレーム探索
        const auto& keys = anim.keyframes;
        const int keyCount = static_cast<int>(keys.size());
        for (int i = 0; i < keyCount - 1; ++i) {
            const auto& k0 = keys[i];
            const auto& k1 = keys[i + 1];
            if (seconds >= k0.seconds && seconds < k1.seconds) {
                float rate = (seconds - k0.seconds) / (k1.seconds - k0.seconds);

                auto& nodes = model->GetNodes();
                const int nodeCount = static_cast<int>(nodes.size());
                for (int n = 0; n < nodeCount; ++n) {
                    auto& node = nodes[n];
                    const auto& nk0 = k0.nodeKeys[n];
                    const auto& nk1 = k1.nodeKeys[n];

                    using namespace DirectX;
                    if (blend < 1.0f) {
                        // 直前姿勢(node) → 新姿勢(nk1)へ “ブレンド”
                        XMVECTOR S = XMVectorLerp(XMLoadFloat3(&node.scale), XMLoadFloat3(&nk1.scale), blend);
                        XMVECTOR R = XMQuaternionSlerp(XMLoadFloat4(&node.rotate), XMLoadFloat4(&nk1.rotate), blend);
                        XMVECTOR T = XMVectorLerp(XMLoadFloat3(&node.translate), XMLoadFloat3(&nk1.translate), blend);
                        XMStoreFloat3(&node.scale, S);
                        XMStoreFloat4(&node.rotate, R);
                        XMStoreFloat3(&node.translate, T);
                    }
                    else {
                        // 本来の補間（k0→k1 を rate で）
                        XMVECTOR S = XMVectorLerp(XMLoadFloat3(&nk0.scale), XMLoadFloat3(&nk1.scale), rate);
                        XMVECTOR R = XMQuaternionSlerp(XMLoadFloat4(&nk0.rotate), XMLoadFloat4(&nk1.rotate), rate);
                        XMVECTOR T = XMVectorLerp(XMLoadFloat3(&nk0.translate), XMLoadFloat3(&nk1.translate), rate);
                        XMStoreFloat3(&node.scale, S);
                        XMStoreFloat4(&node.rotate, R);
                        XMStoreFloat3(&node.translate, T);
                    }
                }
                break; // 見つかったら抜ける
            }
        }

        // ノード反映（親子合成やボーン行列を内部で更新）
        model->UpdateTransform();
    }

    void SetBlendSeconds(float s) { blendLen = s; }

    bool IsPlaying() const { return playing; }

private:
    Model* model = nullptr;
    int    animIndex = -1;
    float  seconds = 0.0f;
    float  blendLen = 0.2f;
    bool   looping = false;
    bool   playing = false;
};
