#include "Editor.h"
#include "Resources.h"
#include <LUA/lua.h>
#include <string>
#include <wiBacklog.h>

Editor::Data* Editor::GetData(){
    static Data data;
    return &data;
}

int Editor_GetObjectList(lua_State* L){
    auto& wiscene = Game::Resources::GetScene().wiscene;
    auto& entity_list = wiscene.transforms.GetEntityArray();
    wi::unordered_map<wi::ecs::Entity, wi::unordered_set<wi::ecs::Entity>> reduced_hierarchy_group;
    auto jobCount = (uint32_t)std::ceil(entity_list.size()/128.f);
    
    std::mutex enlistThreadsMutex;
    wi::jobsystem::context enlistThreadGroup;

    wi::jobsystem::Dispatch(enlistThreadGroup, jobCount, 1, [&](wi::jobsystem::JobArgs args){
        std::scoped_lock lock (enlistThreadsMutex);

        wi::unordered_map<wi::ecs::Entity, wi::unordered_set<wi::ecs::Entity>> map_hierarchy_group;

        size_t find_max = std::min(entity_list.size(),(size_t)(args.jobIndex+1)*128);
        for(size_t i = args.jobIndex*128; i < find_max; ++i){
            auto entity = entity_list[i];
            auto has_hierarchy = wiscene.hierarchy.GetComponent(entity);
            if(has_hierarchy != nullptr){
                map_hierarchy_group[has_hierarchy->parentID].insert(entity);
            } else {
                map_hierarchy_group[0].insert(entity);
            }
        }

        for(auto& map_pair : map_hierarchy_group){
            reduced_hierarchy_group[map_pair.first].insert(map_pair.second.begin(), map_pair.second.end());
        }
    });

    wi::jobsystem::Wait(enlistThreadGroup);

    if(reduced_hierarchy_group.size() > 0){
        lua_newtable(L);
        for(auto& map_pair : reduced_hierarchy_group){
            wi::vector<wi::ecs::Entity> entities;
            entities.reserve(map_pair.second.size());
            entities.insert(entities.begin(), map_pair.second.begin(), map_pair.second.end());

            lua_newtable(L);
            for(int i = 0; i < entities.size(); ++i){
                lua_pushnumber(L, entities[i]);
                lua_rawseti(L,-2,i);
            }
            lua_setfield(L, -2, std::to_string(map_pair.first).c_str());
        }
        return 1;
    }
    return 0;
}

void Editor::Init(){
    wi::lua::RunText("EditorAPI = true");
    wi::lua::RegisterFunc("Editor_GetObjectList", Editor_GetObjectList);
}

void Editor::Update(float dt){

}