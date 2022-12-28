#pragma once
#include "stdafx.h"

namespace Game{
    struct Scene
    {
        // Scene Internal Data
        struct Prefab
        {
            std::string file;
            enum class LoadMode
            {
                SHALLOW_COPY,
                DEEP_COPY,
                FULL,
                LIBRARY
            };
            LoadMode loadMode = LoadMode::SHALLOW_COPY;
            std::string targetEntity; // Entity to copy, we're using strings to avoid cases when the scene's structure changes for a reason
        
            // Runtime Variables
            wi::vector<wi::ecs::Entity> entity_list; // Precompiled entity list for ease of work
        };
        struct Inactive
        {
            wi::Archive inactive_storage;
        };

        struct Component_Prefab : public Prefab
        {
            void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
        };
        struct Component_Inactive : public Inactive
        {
            void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri){};
        };

        // To track loaded scenes from disks and their datablocks
        struct Archive
        {
            std::string fileName;

            // Archive directory structure:
            // myworld.scene/  -> a folder, the scene file that is a folder with the file extension .scene
            // |- headfile  -> a file that lists all the blocks and entities, but no actual data is stored there
            // |- blocks/  -> a folder that contains the scene file, separated in blocks, with the file name based on the block_id of the file
            // |  |- 1
            // |  |- 2
            // |- blockpreview/  -> a folder that contains the preview of certain blocks, if it exists here than the engine has to load the preview first before anything
            // |  |- 2  -> same block_id from ./blocks/
            // |- entitypreview/  -> a folder that contains the preview of certain object with certain meshes, contains a proxy mesh that substitutes a transform with the same entity_id
            // |  |- 192
            // |- animation/  -> a folder that contains an animation that runs in the scene, stores the filename instead of entity_id, contains both animation_component and lots of animation_datas in one file
            //    |- player_run
            //    |- player_stop

            // Header data, for checking what the scene's content in an overview
            // This is necessary for scene streaming to work, for all cases
            //   KEEP entity's current ID to NOT break the overall structure of the scene blocks
            //   We want to be able to edit only a block of the scene
            struct Block_Table
            {
                // Block table data structure
                // Stores the block id (the filename) and then the chunk position of said block table
                // A chunk is about 512*512*1024 of size stored in block_position
                // There can be different block overlaying the data
                // Preview can be seen by checking out if the block id is available at the "./blockpreview/" subfolder

                wi::ecs::Entity block_id;
                XMINT3 block_position;
                bool no_position = false; // We ignore position completely, making it serve as an on-demand data block, referenced by the other blocks
                wi::vector<wi::ecs::Entity> entities;

                void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);

                // Runtime data
                enum class LoadState
                {
                    UNLOADED,
                    LOADING,
                    LOADED
                };
                LoadState load_state = LoadState::UNLOADED;
            };
            wi::ecs::ComponentManager<Block_Table> block_table;

            struct Entity_Table
            {
                wi::ecs::Entity entity;
                wi::ecs::Entity block_id;
                XMFLOAT4 load_radius;
                wi::vector<wi::ecs::Entity> prerequisites;

                void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
            };
            wi::ecs::ComponentManager<Entity_Table> entity_table;

            // To note: there'll be no more saving and loading full blocks of file, instead we're handling this per block case
            wi::ecs::EntitySerializer seri;
            // Helper functions to update the header data
            void Save_Header();
            void Load_Header();
            // Saving is done without streaming
            // There's no manual load for blocks, instead the data is loaded through streaming
            void Save_Block(wi::ecs::Entity block_id);
            void Stream_Block(wi::ecs::Entity block_id);

            // Scan entities for changes with dependencies and load radius to entity_table, WON'T delete any entity data
            // entity_table's changes on deletion needs to be done together with the actual entity deletion
            void Update_EntityTable();
        };

        wi::scene::Scene wiscene;
        wi::ecs::ComponentManager<Component_Prefab>& prefabs = wiscene.componentLibrary.Register<Component_Prefab>("Game::Scene::Prefab");
        wi::ecs::ComponentManager<Component_Inactive>& inactive = wiscene.componentLibrary.Register<Component_Inactive>("Game::Scene::Inactive");

        // Scene Operation Function
        void Entity_Disable(wi::ecs::Entity entity);
        void Entity_Enable(wi::ecs::Entity entity);

        void Update(float dt);
    };
    Scene* GetScene();
}