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

	int GetWidth()const { return width; }
	int GetHeight()const { return height; }

 private:
	int width;//マップの横方向のセル数
	int height;//マップの縦方向のセル数
	float cell_size;//セルのサイズ
	std::vector<std::vector<int>> grid;//0 通れる 1 障害物 
	OBB obb;
 };