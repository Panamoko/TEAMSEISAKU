#pragma once

#include<vector>
#include <memory>
#include <queue>
#include "GridMap.h"
#include <unordered_map>
#include <cmath>

class AStar
{
public:
	/*
		startX,startZ:開始セル座標
		goalX,goalZ:目的セル座標
		gridMap:通行可能/不可情報を持つグリッドマップ
	*/
	std::vector<std::pair<int, int>>FindPath(
		int start_cellX, int start_cellZ,
		int goal_cellX, int goal_cellZ,
		const GridMap& gridMap
	);

	//動的再探索
	std::vector<std::pair<int, int>> ReplanPath(
		int startX, int startZ,
		int goalX, int goalZ,
		const GridMap& gridMap,
		int agent_cellX, int agent_cellZ
	);

private:

	float move_cost = 1.0f;//移動コスト

	//A*探索の最小単位、セルごとの情報を持つ
	struct Node
	{
		enum State { UNVISITED, OPEN, CLOSED }; // 探索状態を追加

		int node_x, node_z = 0.0f;//ノード座標
		float goal_cost = 0.0f;//スタートからこのノードまでの累計コスト
		float h_cost = 0.0f;//このノードからゴールまでの推定コスト
		float fCost() const { return goal_cost + h_cost; }//総コスト

		Node* parent = nullptr;//経路復元用の親ノード
		State node_state = UNVISITED;//探索状態

		void Reset()
		{
			parent = nullptr;
			node_state = UNVISITED;
		}
	};

	class NodePool
	{
	public:
		void Reserve(size_t size) { pool.reserve(size); }
		void Reset() { next_free_index = 0; }
		Node* GetNode()
		{
			if (next_free_index >= pool.size())
			{
				pool.emplace_back();
			}
			return &pool[next_free_index++];
		}
		size_t GetNextFreeIndex()const
		{
			return next_free_index;
		}
	private:
		std::vector<Node> pool;
		size_t next_free_index = 0;
	};

	/*メモリ効率のために、Nodeオブジェクトをあらかじめ確保し、
	再利用するためのプール管理クラスのインスタンス*/
	NodePool node_pool;

	//ノード比較用（優先度付きキューでfCostが小さい順）
	struct CompareNode
	{
		bool operator()(const Node* nodeA, const Node* nodeB)const
		{
			if (nodeA->fCost() == nodeB->fCost())
				return nodeA->h_cost > nodeB->h_cost;//同値なら推定コストが小さい方を優先
			return nodeA->fCost() > nodeB->fCost();
		}
	};

	//(x,z) の座標ペアを unordered_map で使うためのハッシュ関数
	struct pair_hash
	{
		template<class T1,class T2>
		std::size_t operator()(const std::pair<T1, T2>& p)const
		{
			auto h1 = std::hash<T1>{}(p.first);
			auto h2 = std::hash<T2>{}(p.second);
			return h1 ^ (h2 << 1);//XORで合成
		}
	};

	//ヒューリスティック関数
	float Heuristic(int current_cellX, int current_cellZ, int goal_cellX, int goal_cellZ)const;

	//隣接セルを取得
	size_t GetNeighbors(int cellX, int cellZ, const GridMap& grid_map, std::pair<int, int>* out_neighbors) const;

	//座標を配列のインデックスに変換
	size_t CoordinateToIndex(int cell_x, int cell_z)const;

	std::vector < std::pair<int, int>> SmoothPath(const std::vector<std::pair<int, int>>& path, const GridMap& gridMap)const;

	bool HasLineOfSight(int start_x, int start_z, int end_x, int end_z, const GridMap& gridMap)const;

	int map_width_ = 0;//マップの幅

	//ノード管理用マップ
	std::unordered_map<std::pair<int, int>, Node*, pair_hash> node_map;//ノード管理マップ

	std::vector<Node*> node_grid_pointers;

	std::vector<std::pair<int, int>> last_path; // 前回の経路

	int max_search_nodes = 5000;//最大探索ノード数
};

