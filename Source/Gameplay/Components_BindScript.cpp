#include "Components_BindScript.h"
#include <wiScene_BindLua.h>

namespace Gameplay::Scripting::Component
{
    // HEALTH SECTION START
    const char Health_Bind::className[] = "Gameplay_Health";
    Luna<Health_Bind>::FunctionType Health_Bind::methods[] = {
        {NULL, NULL}
    };
    Luna<Health_Bind>::PropertyType Health_Bind::properties[] = {
        lunaproperty(Health_Bind, health),
        {NULL, NULL}
    };
    // HEALTH SECTION END

    // HITDATA SECTION START
    const char HitData_Bind::className[] = "Gameplay_HitData";
    Luna<HitData_Bind>::FunctionType HitData_Bind::methods[] = {
        {NULL, NULL}
    };
    Luna<HitData_Bind>::PropertyType HitData_Bind::properties[] = {
        lunaproperty(HitData_Bind, hit),
        lunaproperty(HitData_Bind, hitpoint),
        lunaproperty(HitData_Bind, communicationType),
        lunaproperty(HitData_Bind, colliderType),
        {NULL, NULL}
    };
    // HITDATA SECTION END

    // PROJECTILE SECTION START
    const char Projectile_Bind::className[] = "Gameplay_Projectile";
    Luna<Projectile_Bind>::FunctionType Projectile_Bind::methods[] = {
        {NULL, NULL}
    };
    Luna<Projectile_Bind>::PropertyType Projectile_Bind::properties[] = {
        lunaproperty(Projectile_Bind, normal),
        lunaproperty(Projectile_Bind, hitbox),
        {NULL, NULL}
    };
    // PROJECTILE SECTION END

    int _internal_Create_Health(lua_State* L)
    {
        int args = wi::lua::SGetArgCount(L);
        if(args == 2)
        {
            Gameplay::Component::Health& health = Gameplay::Component::healths->Create(wi::ecs::Entity(wi::lua::SGetLongLong(L, 1)));
            health.health = wi::lua::SGetFloat(L, 2);
        }
        else
        {
            wi::lua::SError(L, "Gameplay_Create_Component_Health(entity, health) not enough arguments");
        }
        return 0;
    }

    int _internal_Create_HitData(lua_State* L)
    {
        int args = wi::lua::SGetArgCount(L);
        if(args == 4)
        {
            wi::ecs::Entity entity = wi::ecs::Entity(wi::lua::SGetLongLong(L, 1));

            Gameplay::Component::HitData& hitdata = Gameplay::Component::hitdatas->Create(entity);
            hitdata.hitpoint = wi::lua::SGetFloat(L, 2);
            hitdata.communicationType = Gameplay::Component::HitData::CommunicationType(wi::lua::SGetInt(L, 3));
            hitdata.colliderType = Gameplay::Component::HitData::ColliderType(wi::lua::SGetInt(L, 4));
        }
        else
        {
            wi::lua::SError(L, "Gameplay_Create_Component_HitData(entity, hitpoint, communicationtype, collidertype) not enough arguments");
        }
        return 0;
    }

    int _internal_Find_Projectile(lua_State* L)
    {
        int args = wi::lua::SGetArgCount(L);
        if(args > 0)
        {
            wi::ecs::Entity entity = wi::ecs::Entity(wi::lua::SGetLongLong(L, 1));
            Gameplay::Component::Projectile* component = Gameplay::Component::projectiles->GetComponent(entity);
            if(component != nullptr)
            {
                Luna<Projectile_Bind>::push(L, component);
                return 1;
            }
        }
        else
        {
            wi::lua::SError(L, "Gameplay_Component_Find_Projectile(entity) not enough arguments");
        }
        return 0;
    }

    void Bind()
    {
        static bool initialized = false;
        if(!initialized)
        {
            lua_State* L = wi::lua::GetLuaState();

            Luna<Health_Bind>::Register(L);
            Luna<HitData_Bind>::Register(L);
            Luna<Projectile_Bind>::Register(L);
        }

        wi::lua::RegisterFunc("Gameplay_Create_Component_Health",_internal_Create_Health);
        wi::lua::RegisterFunc("Gameplay_Create_Component_HitData",_internal_Create_HitData);
        wi::lua::RegisterFunc("Gameplay_Component_Find_Projectile",_internal_Find_Projectile);
    }
}