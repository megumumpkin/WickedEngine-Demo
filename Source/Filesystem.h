#pragma once
#include "stdafx.h"

namespace Game
{
    namespace Filesystem
    {
        // Register a filesystem directory before working on loading and saving a data
        void Register_FS(std::string virtualpath, std::string actualpath, bool file);
        // Use overlay for situations such as patches, DLC, and the like
        void Register_FSOverlay(std::string virtualpath, std::string actualpath, bool file);

        // Use Game::Filesystem::fstream to work with files!
        class fstream
        {
        private:
            std::fstream streamhandle;
            size_t head;
        public:
            bool Open(std::string dir);
            void Close();
            inline fstream& operator>> (char& data);
            inline fstream& operator<< (char data);
        };
    }
}