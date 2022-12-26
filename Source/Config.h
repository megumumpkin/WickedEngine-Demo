#pragma once
#include "stdafx.h"

#define softinfo_title "Final Arc"
#define softinfo_titleshort "FinalArcGame"
#define softinfo_version_major 0
#define softinfo_version_revision 1

struct AppSettings
{
    uint32_t video_width;
    uint32_t video_height;
    uint32_t video_fullscreen;
};

void AppSettings_Save(AppSettings& settings);
AppSettings AppSettings_Load();