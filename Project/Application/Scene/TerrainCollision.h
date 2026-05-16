#pragma once
#include <vector>
#include "Mesh.h"
#include "VertexData.h"

namespace GameEngine {

    // 位置
    struct PlayerData {
        Vector3 position;   // 現在位置
        float   height;     // 足元からの身長オフセット
    };

    // 結果
    struct GroundResult {
        bool    hit;
        float   groundY;
        Vector3 normal;
    };

    // 三角形のAABB
    struct TriangleAABB {
        float minX, maxX, minZ, maxZ;
    };

    class TerrainCollision {
    public:
        TerrainCollision() = default;
        ~TerrainCollision() = default;

        /// <summary>
        /// Meshデータを取得
        /// </summary>
        /// <param name="mesh"></param>
        void Build(Mesh& mesh);

        /// <summary>
        // グリッド作成
        /// </summary>
        /// <param name="cellSize"></param>
        void BuildGrid(float cellSize);

        /// <summary>
        /// 複数プレイヤーの位置をMesh上に吸着させる
        /// </summary>
        void ResolveCollision(std::vector<PlayerData>& players) const;

        /// <summary>
        /// 1点の真下の地面高さを取得
        /// </summary>
        GroundResult GetGroundHeight(const Vector3& position) const;

    private:

        // グリッド分割用
        struct Grid2D {
            int cellsX = 0;
            int cellsZ = 0;
            float originX = 0;
            float originZ = 0;
            float cellSize = 0;
            // cells[iz * cellsX + ix]
            std::vector<std::vector<uint32_t>> cells;

            int CellIndex(int ix, int iz) const { return iz * cellsX + ix; }

            bool WorldToCell(float x, float z, int& ix, int& iz) const {
                ix = static_cast<int>((x - originX) / cellSize);
                iz = static_cast<int>((z - originZ) / cellSize);
                if (ix < 0 || ix >= cellsX || iz < 0 || iz >= cellsZ) return false;
                return true;
            }
        };

    private:
        std::vector<Vector3>   vertices_;      // 頂点位置キャッシュ
        std::vector<uint32_t>  indices_;       // インデックスキャッシュ
        uint32_t triangleCount_ = 0;

        // 三角形のAABB
        std::vector<TriangleAABB> triAABBs_;

        // 地形をXZ平面のグリッドで分割
        Grid2D grid_;

    private:
        // Möller–Trumboreアルゴリズム
        bool RayTriangleIntersect(
            const Vector3& rayOrigin,
            const Vector3& rayDir,
            const Vector3& v0,
            const Vector3& v1,
            const Vector3& v2,
            float& outT,
            Vector3& outNormal) const;
    };
}