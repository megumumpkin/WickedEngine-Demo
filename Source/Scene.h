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

                wi::ecs::Entity block_id;
                XMINT3 block_position;
                wi::vector<wi::ecs::Entity> entities;
                wi::ecs::Entity preview_id; // We can have a proxy mesh that can preview the block's contents, to hide the fact that it is not loaded

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
            void Save_Block(wi::ecs::Entity block_id);
            // There's no manual load for blocks, instead the data is loaded through streaming
            void Stream_Block(wi::ecs::Entity block_id);
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