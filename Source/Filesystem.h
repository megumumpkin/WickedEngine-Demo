#pragma once
#include "stdafx.h"
#include <filesystem>

namespace Game
{
    namespace Filesystem
    {
        // Register a filesystem directory before working on loading and saving a data
        void Register_FS(std::string virtualpath, std::string actualpath, bool file);
        // Use overlay for situations such as patches, DLC, and the like
        void Register_FSOverlay(std::string virtualpath, std::string actualpath, bool file);

        std::string GetActualPath(const std::string& file);
        bool FileRead(const std::string& file, wi::vector<uint8_t>& data);
        bool FileWrite(const std::string& file, const uint8_t* data, size_t size);
        bool FileExists(const std::string& file);
    }
}