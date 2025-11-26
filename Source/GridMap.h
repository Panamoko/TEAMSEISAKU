#pragma once
#include <vector>
#include <memory>
#include "GameObject.h"
class GridMap
{
public:
	//目的：グリッド（マップ）を初期化
	GridMap();
	void Initialize(int map_width, int map_height, float cell);
	//ゲーム内のオブジェクトを見て障害物マップを作る
	void Build(const std::vector < std::shared_ptr<GameObject>>& objects);
	//指定セルが障害物かどうか調べる
	bool IsBlocked(int x, int z)const;

	void  RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer);

	//動的ブロック管理
	void SetBlocked(int x, int z, bool blocked);

	std::pair<int, int> WorldToCell(float worldX, float worldZ) const;

	//セル座標からワールド座標（中心）を取得
	DirectX::XMFLOAT3 GetWorldPosition(int cellX, int cellZ) const;
	//セルサイズ取得
	float GetCellSize() const { return cell_size; }

	int GetWidth()const { return width; }
	int GetHeight()const { return height; }

	bool IsOnMap(int cell_x, int cell_z)const;

private:
	int width;//マップの横方向のセル数
	int height;//マップの縦方向のセル数
	float cell_size;//セルのサイズ
	std::vector<std::vector<int>> grid;//0 通れる 1 障害物 
	OBB obb;
};