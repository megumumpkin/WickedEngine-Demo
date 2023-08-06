#include <WickedEngine.h>

namespace Gameplay
{
    namespace Helpers
    {
        class GradientMap
        {
        public:
            GradientMap() {}
            GradientMap(wi::vector<std::pair<float,float>> interpolations) : interpolations(interpolations) {}
            float Get(float value);
        private:
            wi::vector<std::pair<float,float>> interpolations;
        };

        float GetAngleDiff(XMFLOAT3 euler_origin, XMFLOAT3 euler_dest);
        float GetAngleDiff(XMFLOAT4 quat_origin, XMFLOAT4 quat_dest);
    }
}