#pragma once
#include "stdafx.h"

namespace Game{
    struct Scene
    {
        struct Archive
        {
            std::string file; // Target file, root of the scene datablocks
            wi::ecs::Entity prefabID; // Does this scene works as a prefab
            
            // File structure
            // |- head  -> scene data header
            // |- main  -> wiscene file with minimum structure
            // |- preview  -> a simplified mesh that describes the whole one scene 
            // |- meshes  -> list of meshes, stored in entity ID
            // |  |- 1
            // |  |- 2
            // |- animation  -> list of animations, stored in entity ID
            // |  |- 3
            // |  |- 4

            // head file
            // List all known streamables here, when loaded the entity ID has already been remapped to the runtime version of the scene
            wi::unordered_map<wi::ecs::Entity, float> radius; // Stream radius, needs to be generated from meshes and sounds
            wi::unordered_map<wi::ecs::Entity, wi::ecs::Entity> meshes; // Meshes and its previews are listed in here
            wi::unordered_map<std::string, wi::ecs::Entity> animations;// Animations are listed in here
            // Store materials and sounds in binary blobs to load later
            wi::unordered_map<wi::ecs::Entity, wi::vector<uint8_t>> materials;
            wi::unordered_map<wi::ecs::Entity, wi::vector<uint8_t>> sounds;

            // Runtime data
            wi::unordered_map<uint64_t, wi::ecs::Entity> remap; // For use with entityseralizer
            // Streaming progresses, check the data from here before doing anything
            enum class LoadState
            {
                UNLOADED,
                LOADING,
                LOADED
            };
            LoadState load_state = LoadState::UNLOADED;

            wi::unordered_set<wi::ecs::Entity> mesh_streaming_inprogress; // Check whether a component is being loaded or not
            wi::unordered_map<wi::ecs::Entity, uint32_t> mesh_dependency_counts; // Check on whether we can remove component from the scene
            wi::unordered_set<wi::ecs::Entity> animation_streaming_inprogress;
            wi::unordered_map<wi::ecs::Entity, uint32_t> animation_dependency_counts;
            wi::unordered_map<wi::ecs::Entity, uint32_t> material_dependency_counts; 

            // Runtime functions, also cover functions for prefabs'
            // Header Actions
            void Head_Load();
            void Head_Save();
            // Base Scene Actions
            void Init(wi::ecs::Entity clone_prefabID = wi::ecs::INVALID_ENTITY); // Load the main file
            void Unload(wi::ecs::Entity clone_prefabID = wi::ecs::INVALID_ENTITY); // Unload all
            // Streaming function, with prefabs in mind too
            void Stream_Mesh(wi::ecs::Entity meshID, bool unload = false, wi::ecs::Entity clone_prefabID = wi::ecs::INVALID_ENTITY); // Materials are also streamed at the same time here!
            void Stream_Animation(std::string animationID, bool unload = false, wi::ecs::Entity clone_prefabID = wi::ecs::INVALID_ENTITY);
            void Stream_Sounds(wi::ecs::Entity soundID, bool unload = false, wi::ecs::Entity clone_prefabID = wi::ecs::INVALID_ENTITY);
        };

        struct Prefab
        {
            std::string file;
            std::string target_entity;
            enum class CopyMode
            {
                SHALLOW_COPY, // For static objects
                DEEP_COPY, // For animated objects such as player characters, Does not have any entities that will refer some data to the original prefab
                LIBRARY // To make some of the entities to be disabled on run
            };
            CopyMode copy_mode = CopyMode::SHALLOW_COPY;
            // Runtime data
            bool loaded = false; // Check if this prefab is loaded or not
            wi::unordered_map<uint64_t, wi::ecs::Entity> remap; // All remaps are handled here
            wi::unordered_set<wi::ecs::Entity> entities; // List all entities
            wi::unordered_set<wi::ecs::Entity> streams; // List entities that are stream related

            // Check prefab clone's dependency count
            wi::unordered_map<wi::ecs::Entity, uint32_t> mesh_dependency_counts;
            wi::unordered_map<wi::ecs::Entity, uint32_t> material_dependency_counts; 
        };
        struct Inactive
        {
            wi::vector<uint8_t> inactive_storage;
        };

        struct Component_Prefab : public Prefab
        {
            void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri){};
        };
        struct Component_Inactive : public Inactive
        {
            void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri){};
        };

        wi::scene::Scene wiscene;
        wi::ecs::ComponentManager<Component_Prefab>& prefabs = wiscene.componentLibrary.Register<Component_Prefab>("Game::Scene::Prefab");
        wi::ecs::ComponentManager<Component_Inactive>& inactives = wiscene.componentLibrary.Register<Component_Inactive>("Game::Scene::Inactive");

        std::string current_scene; // Current filename that is used to point the root scene from scene_db
        wi::unordered_map<std::string, Archive> scene_db; // Lists all scenes referenced in this current game session

        // Scene Operation Function
        void Entity_Disable(wi::ecs::Entity entity);
        void Entity_Enable(wi::ecs::Entity entity);
        void Entity_Clone(wi::ecs::Entity entity, wi::ecs::EntitySerializer* seri, bool deep_copy = false);

        // Saves and loads only root scene file only, saving can only be done in Dev console mode
        void Load(std::string file);
        void Save(std::string file);

        void Update(float dt);
    };
    Scene* GetScene();
}