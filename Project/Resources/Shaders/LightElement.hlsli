
struct DirectionalLight
{
    float32_t4 color; // ライトの色
    float32_t3 direction; // ライトの向き
    float32_t intensity; // 輝度
    int32_t active;
    uint32_t isDepthTexture; // 深度値を持ったテクスチャ
    float32_t2 padding;
    float32_t4x4 vpMatrix;
};

struct PointLight
{
    float32_t4 color; // ライトの色
    float32_t3 position; // ライトの位置
    float32_t intensity; // 輝度
    int32_t active; // 有効化
    float32_t radius; // ライトの届く最大距離
    float32_t decay; // 減衰率
};

struct SpotLight
{
    float32_t4 color; // ライトの色
    float32_t3 position; // ライトの位置
    float32_t intensity; // 輝度
    float32_t3 direction; // ライトの方向
    float32_t distance; // ライトの最大距離
    float32_t decay; // 減衰率
    float32_t cosAngle; // 減衰率
    float32_t cosFalloffStart; // 
    int32_t active; // 有効化
};