#pragma once
#include "stdafx.h"

// Development ecosystem for integration with Blender
namespace Dev
{
    struct CommandData
    {
        enum class CommandType
        {
            SCENE_IMPORT,
            SCENE_PREVIEW,
            SCENE_EXTRACT,
        }; 
        CommandType type; // -t
        std::string input; // -i
        std::string output; // -o
    };

    struct ProcessData
    {
        // SCENE_IMPORT PROCESSES DATA
        XMFLOAT3 scene_offset;
        wi::unordered_map<std::string, wi::scene::TransformComponent> composite_offset;
    };

    // Development CLI
    CommandData* GetCommandData();
    ProcessData* GetProcessData();
    bool ReadCMD(int argc, char *argv[]);
    bool ReadCMD(const wchar_t* win_args);
    
    void Execute(float dt); // Execute stored commands
    void UpdateHook(); // Development Interconnect (with Embark Studios' Skyhook perhaps?)
    void UpdateUI(); // Development UI

    namespace IO
    {
        extern wi::vector<std::string> importdata_lodtier;
        void Import_GLTF(const std::string& fileName, wi::scene::Scene& scene);
        void Export_GLTF(const std::string& filename, wi::scene::Scene& scene);
    }

    namespace LiveUpdate
    {
        void Init();
        void Update();
    }
};