#pragma once
#include "pch.h"

namespace display
{
enum eResolution
{
    k720p,
    k900p,
    k1080p,
    k1440p,
    k1800p,
    k2160p
};

void ResolutionToUINT(eResolution res, uint32_t& width, uint32_t& height)
{
    switch (res)
    {
        default:
        case k720p:
            width  = 1280;
            height = 720;
            break;
        case k900p:
            width  = 1600;
            height = 900;
            break;
        case k1080p:
            width  = 1920;
            height = 1080;
            break;
        case k1440p:
            width  = 2560;
            height = 1440;
            break;
        case k1800p:
            width  = 3200;
            height = 1800;
            break;
        case k2160p:
            width  = 3840;
            height = 2160;
            break;
    }
}

} // namespace display