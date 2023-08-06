#include "Helpers.h"

float Gameplay::Helpers::GradientMap::Get(float value)
{
    float get = 0.f;

    for(int i = 0; i < interpolations.size(); ++i)
    {
        if((i == 0) && (value <= interpolations[i].first))
        {
            // wi::backlog::post("FAR");
            get = interpolations[i].second;
        }
        if((i+1) < interpolations.size())
        {
            // wi::backlog::post("HASBETWEEN");
            if(value > interpolations[i].first)
            {
                
                float interval = (value - interpolations[i].first)/(interpolations[i+1].first - interpolations[i].first);
                interval = wi::math::Clamp(interval, 0.f, 1.f);
                // wi::backlog::post("Interval: "+std::to_string(interval));
                get = wi::math::Lerp(interpolations[i].second, interpolations[i+1].second, interval);
            }
        }
    }
    // return wi::math::Clamp(get, interpolations[0].second, interpolations[interpolations.size()-1].second);
    return get;
}

float Gameplay::Helpers::GetAngleDiff(XMFLOAT3 euler_origin, XMFLOAT3 euler_dest)
{
    float angle_diff = std::fmod(euler_origin.y - euler_dest.y + wi::math::PI, wi::math::PI*2.f) - wi::math::PI;
    angle_diff = angle_diff < -wi::math::PI ? angle_diff + wi::math::PI*2.f : angle_diff;
    // angle_diff = wi::math::Clamp(angle_diff, -wi::math::PI*0.1f, wi::math::PI*0.1f);
    return angle_diff;
}

float Gameplay::Helpers::GetAngleDiff(XMFLOAT4 quat_origin, XMFLOAT4 quat_dest)
{
    XMFLOAT3 angle_origin, angle_dest;
    angle_origin = wi::math::QuaternionToRollPitchYaw(quat_origin);
    angle_dest = wi::math::QuaternionToRollPitchYaw(quat_dest);
    return Gameplay::Helpers::GetAngleDiff(angle_origin, angle_dest);
}