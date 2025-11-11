#pragma once
#include <vector>
#include <memory>
#include "GameObject.h"
 class GridMap 
 { 
 public:
	 //目的：グリッド（マップ）を初期化
	GridMap(int width, int height, float cell);
	//ゲーム内のオブジェクトを見て障害物マップを作る
	void Build(const std::vector < std::shared_ptr<GameObject>>& objects);
	//指定セルが障害物かどうか調べる
	bool IsBloked(int x, int z)const;
 private:
	int width;//マップの縦の大きさ
	int height;//マップの横の大きさ
	float cell_size;//セルのサイズ
	std::vector<int> grid;//0 通れる 1 障害物 
	OBB obb;
 };