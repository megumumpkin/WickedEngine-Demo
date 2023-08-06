#include "GBus.h"
#include "Projectile.h"
#include "Components.h"
#include "../Scene.h"

#include <WickedEngine.h>
#include <mutex>

wi::jobsystem::context pj_ctx;
std::mutex data_sync;

wi::unordered_map<wi::ecs::Entity, wi::ecs::Entity> projectiles;
wi::unordered_set<wi::ecs::Entity> projectiles_dead;

struct ProjectileProgramData
{
    std::string prefab;

    bool shoot = false;
    wi::primitive::Ray spawn;
    float rate = 0.f;
    XMFLOAT3 velocity = XMFLOAT3();
    wi::vector<wi::scene::TransformComponent> spawners;

    float rate_timer = 0.f;
};
wi::unordered_map<wi::ecs::Entity, ProjectileProgramData> projectile_programs;

void Gameplay::Projectile::Hook_Init()
{
    wi::lua::RunText(Projectile_Scripting_Globals);
}

struct ProjectileDispatchData
{
    float dt;
    wi::vector<wi::ecs::Entity> relive_requests;
    wi::vector<wi::ecs::Entity> delete_requests;
};
ProjectileDispatchData projectile_dispatch_data;
void Gameplay::Projectile::Hook_PreUpdate(float dt)
{
    // Run projectile sim
    projectile_dispatch_data.dt = dt;
    wi::jobsystem::Dispatch(pj_ctx, Gameplay::Component::projectiles->GetCount(), 255, [](wi::jobsystem::JobArgs jobArgs){
        uint32_t projectile_idx = jobArgs.jobIndex;
        wi::ecs::Entity projectileID = Gameplay::Component::projectiles->GetEntity(projectile_idx);

        if(projectiles_dead.find(projectileID) == projectiles_dead.end())
        {
            wi::scene::TransformComponent* transform = Game::GetScene().wiscene.transforms.GetComponent(projectileID);
            transform->Translate(XMLoadFloat3(&(*Gameplay::Component::projectiles)[projectile_idx].normal)*projectile_dispatch_data.dt*15.f);
        
            Game::Scene::Prefab* prefab = Game::GetScene().prefabs.GetComponent(projectileID);
            if(prefab->loaded)
            {
                Gameplay::Component::HitData* hitdata = Gameplay::Component::hitdatas->GetComponent((*Gameplay::Component::projectiles)[projectile_idx].hitbox);
                if(hitdata != nullptr)
                {
                    if(hitdata->hit)
                    {
                        std::scoped_lock sync (data_sync);
                        projectiles_dead.insert(projectileID);
                        hitdata->communicationType = Gameplay::Component::HitData::CommunicationType::DISABLED;
                        prefab->Disable();
                    }
                }
            }
        }
    });
    wi::jobsystem::Wait(pj_ctx);
}

void Gameplay::Projectile::Hook_Update(float dt)
{
    // Compute projectile data
    for(auto& projectile_program : projectile_programs)
    {
        wi::ecs::Entity programID = projectile_program.first;
        ProjectileProgramData* program = &projectile_program.second;

        Game::Scene::Component_Script* script = Game::GetScene().scripts.GetComponent(programID);
        if(!script->done_init)
            continue;
        
        lua_State* L = wi::lua::GetLuaState();
        lua_getglobal(L, "Gameplay_Projectile_GetPrefabFile");
        lua_pushnumber(L, programID);
        lua_call(L, 1, 1);
        program->prefab = wi::lua::SGetString(L, -1);
        lua_pop(L, 1);

        lua_getglobal(L, "Gameplay_Projectile_GetSpawnRate");
        lua_pushinteger(L, programID);
        lua_call(L, 1, 1);
        program->rate = float(wi::lua::SGetFloat(L, -1));
        lua_pop(L, 1);

        lua_getglobal(L, "Gameplay_Projectile_GetVelocity");
        lua_pushinteger(L, programID);
        lua_call(L, 1, 1);
        lua_pushnumber(L, 1);
        lua_gettable(L, -2);
        program->velocity.x = float(lua_tonumber(L, -1));
        lua_pop(L, 1);
        lua_pushnumber(L, 2);
        lua_gettable(L, -2);
        program->velocity.y = float(lua_tonumber(L, -1));
        lua_pop(L, 1);
        lua_pushnumber(L, 3);
        lua_gettable(L, -2);
        program->velocity.z = float(lua_tonumber(L, -1));
        lua_pop(L, 1);

        lua_getglobal(L, "Gameplay_Projectile_GetSpawners");
        lua_pushinteger(L, programID);
        lua_call(L, 1, 1);
        size_t table_size = lua_rawlen(L, -1);

        int counter = 0;
        program->spawners.clear();
        for(size_t i = 0; i < table_size; ++i)
        {
            if(counter == 0)
            {
                program->spawners.emplace_back();
            }

            lua_pushnumber(L, i+1);
            lua_gettable(L, -2);
            XMFLOAT3 rotation_euler;
            switch(counter)
            {
                case 0: program->spawners.back() = wi::scene::TransformComponent(); program->spawners.back().translation_local.x = float(lua_tonumber(L, -1)); break;
                case 1: program->spawners.back().translation_local.y = float(lua_tonumber(L, -1)); break;
                case 2: program->spawners.back().translation_local.z = float(lua_tonumber(L, -1)); break;
                case 3: rotation_euler.x = float(lua_tonumber(L, -1)); break;
                case 4: rotation_euler.y = float(lua_tonumber(L, -1)); break;
                case 5: rotation_euler.z = float(lua_tonumber(L, -1)); program->spawners.back().RotateRollPitchYaw(rotation_euler); program->spawners.back().UpdateTransform(); counter = 0; break;
            }
            lua_pop(L, 1);
            counter++;
            
        }
        
        if(program->shoot && (program->rate_timer == 0.f))
        {
            XMFLOAT3 Up = XMFLOAT3(0.f,1.f,0.f);
            XMFLOAT3 Forward = XMFLOAT3(0.f,0.f,1.f);

            size_t new_count = program->spawners.size();

            if(Gameplay::Component::projectiles->GetCount() > 0)
            {
                wi::jobsystem::Dispatch(pj_ctx, Gameplay::Component::projectiles->GetCount(), 256, [programID,&program](wi::jobsystem::JobArgs jobArgs){
                    uint32_t projectile_idx = jobArgs.jobIndex;
                    wi::ecs::Entity projectileID = Gameplay::Component::projectiles->GetEntity(projectile_idx);

                    if( (*Gameplay::Component::projectiles)[projectile_idx].program == programID
                        &&(projectiles_dead.find(projectileID) != projectiles_dead.end()))
                    {
                        std::scoped_lock sync (data_sync);
                        projectile_dispatch_data.relive_requests.push_back(projectileID);
                    }
                });
                wi::jobsystem::Wait(pj_ctx);
            }

            size_t new_count_target = std::max(int(new_count - projectile_dispatch_data.relive_requests.size()), 0);
            for(size_t i = 0; i < new_count_target; ++i)
            {
                wi::ecs::Entity projectileID = wi::ecs::CreateEntity();
                wi::scene::TransformComponent& transform = Game::GetScene().wiscene.transforms.Create(projectileID);

                XMVECTOR angle_hori_normal = XMVector3Normalize(XMLoadFloat3(&program->spawn.direction)*XMVectorSet(1.f, 0.f, 1.f, 1.f));
                XMVECTOR angle_vert_normal = XMVector3Normalize(XMLoadFloat3(&program->spawn.direction));
                XMFLOAT3 angle_vert_normal_vec3;
                XMStoreFloat3(&angle_vert_normal_vec3, angle_vert_normal);
                angle_vert_normal_vec3.x = 0.f;
                angle_vert_normal_vec3.z = 1.f - angle_vert_normal_vec3.y;
                angle_vert_normal = XMLoadFloat3(&angle_vert_normal_vec3);

                float horizontal_angle = wi::math::GetAngle(XMLoadFloat3(&Forward)*-1.f, angle_hori_normal, XMLoadFloat3(&Up));
                float vertical_angle = wi::math::GetAngle(XMLoadFloat3(&Up)*1.f, angle_vert_normal, XMLoadFloat3(&Forward));

                transform.MatrixTransform(XMMatrixRotationRollPitchYaw(-vertical_angle+XM_PIDIV2, horizontal_angle, 0.f));
                transform.Translate(program->spawn.origin);

                Gameplay::Component::Projectile& projectile = Gameplay::Component::projectiles->Create(projectileID);
                projectile.normal = program->spawn.direction;
                projectile.program = programID;

                Game::Scene::Prefab& prefab = Game::GetScene().prefabs.Create(projectileID);
                prefab.file = program->prefab;
                prefab.stream_mode = Game::Scene::Prefab::StreamMode::DIRECT;
                prefab.fade_factor = 0.f;
            }
            for(size_t i = 0; i < std::min(projectile_dispatch_data.relive_requests.size(), new_count); ++i)
            {
                wi::ecs::Entity projectileID = projectile_dispatch_data.relive_requests[i];
                // wi::backlog::post("XMX2: "+std::to_string(projectileID));

                Game::Scene::Prefab* prefab = Game::GetScene().prefabs.GetComponent(projectileID);
                if(prefab->disabled)
                    prefab->Enable();

                XMVECTOR angle_hori_normal = XMVector3Normalize(XMLoadFloat3(&program->spawn.direction)*XMVectorSet(1.f, 0.f, 1.f, 1.f));
                XMVECTOR angle_vert_normal = XMVector3Normalize(XMLoadFloat3(&program->spawn.direction));
                XMFLOAT3 angle_vert_normal_vec3;
                XMStoreFloat3(&angle_vert_normal_vec3, angle_vert_normal);
                angle_vert_normal_vec3.x = 0.f;
                angle_vert_normal_vec3.z = 1.f - angle_vert_normal_vec3.y;
                angle_vert_normal = XMLoadFloat3(&angle_vert_normal_vec3);

                float horizontal_angle = wi::math::GetAngle(XMLoadFloat3(&Forward)*-1.f, angle_hori_normal, XMLoadFloat3(&Up));
                float vertical_angle = wi::math::GetAngle(XMLoadFloat3(&Up)*1.f, angle_vert_normal, XMLoadFloat3(&Forward));

                wi::scene::TransformComponent* transform = Game::GetScene().wiscene.transforms.GetComponent(projectileID);
                transform->ClearTransform();
                transform->MatrixTransform(XMMatrixRotationRollPitchYaw(-vertical_angle+XM_PIDIV2, horizontal_angle, 0.f));
                transform->Translate(program->spawn.origin);

                Gameplay::Component::Projectile* projectile = Gameplay::Component::projectiles->GetComponent(projectileID);
                projectile->normal = program->spawn.direction;

                Gameplay::Component::HitData* hitbox = Gameplay::Component::hitdatas->GetComponent(projectile->hitbox);
                hitbox->hit = false;
                hitbox->communicationType = Gameplay::Component::HitData::CommunicationType::SENDER;

                projectiles_dead.erase(projectileID);
            }

            projectile_dispatch_data.relive_requests.clear();
        }
        if(program->shoot)
            program->rate_timer += dt;
        if((program->rate_timer > program->rate))
            program->rate_timer = 0.f;
    }
}

void Gameplay::Projectile::Hook_FixedUpdate()
{

}

void Gameplay::Projectile::Hook_Migrate(bool store, wi::vector<uint8_t>& storage)
{

}

void Gameplay::GBus::Projectile::CreateProgram(wi::primitive::Ray spawn, wi::ecs::Entity programID, std::string programfile)
{
    auto find_program = projectile_programs.find(programID);
    if(find_program != projectile_programs.end())
    {
        projectile_programs[programID].spawn = spawn;

        lua_State* L = wi::lua::GetLuaState();
        lua_getglobal(L, "Gameplay_Projectile_RestoreResetData");
        wi::lua::SSetString(L, std::to_string(programID).c_str());
        lua_call(L, 1, 0);
    }
    else
    {
        projectile_programs[programID] = {};

        projectile_programs[programID].spawn = spawn;
        Game::Scene::Component_Script& script = Game::GetScene().scripts.Create(programID);
        script.file = programfile;
        script.params += "_ENV.GAMEPLAY_BULLET_PROGRAM["+std::to_string(programID)+"] = {}"
            "local Projectile = _ENV.GAMEPLAY_BULLET_PROGRAM["+std::to_string(programID)+"];";
    }
}
void Gameplay::GBus::Projectile::SetSpawn(wi::primitive::Ray spawn, wi::ecs::Entity programID)
{
    projectile_programs[programID].spawn = spawn;
}
void Gameplay::GBus::Projectile::SetShoot(bool value, wi::ecs::Entity programID)
{
    projectile_programs[programID].shoot = value;
}
