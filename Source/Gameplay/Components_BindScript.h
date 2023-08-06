#pragma once
#include "Components.h"
#include <wiMath_BindLua.h>

namespace Gameplay::Scripting::Component
{
    class Health_Bind
    {
    private:
        std::unique_ptr<Gameplay::Component::Health> owning;
    public:
        Gameplay::Component::Health* component;

        static const char className[];
        static Luna<Health_Bind>::FunctionType methods[];
        static Luna<Health_Bind>::PropertyType properties[];

        inline void BuildBindings()
        {
            health = wi::lua::FloatProperty(&component->health);
        }

        Health_Bind(Gameplay::Component::Health* component) :component(component) { BuildBindings(); }
        Health_Bind(lua_State* L)
        {
            owning = std::make_unique<Gameplay::Component::Health>();
            this->component = owning.get();
            BuildBindings();
        }
        
        wi::lua::FloatProperty health;
        PropertyFunction(health);
    };

    class HitData_Bind
    {
    private:
        std::unique_ptr<Gameplay::Component::HitData> owning;
    public:
        Gameplay::Component::HitData* component;

        static const char className[];
        static Luna<HitData_Bind>::FunctionType methods[];
        static Luna<HitData_Bind>::PropertyType properties[];

        inline void BuildBindings()
        {
            hit = wi::lua::BoolProperty(&component->hit);
            hitpoint = wi::lua::FloatProperty(&component->hitpoint);
            communicationType = wi::lua::IntProperty((int*)&component->communicationType);
            colliderType = wi::lua::IntProperty((int*)&component->colliderType);
        }

        HitData_Bind(Gameplay::Component::HitData* component) :component(component) { BuildBindings(); }
        HitData_Bind(lua_State* L)
        {
            owning = std::make_unique<Gameplay::Component::HitData>();
            this->component = owning.get();
            BuildBindings();
        }

        wi::lua::BoolProperty hit;
        PropertyFunction(hit);
        wi::lua::FloatProperty hitpoint;
        PropertyFunction(hitpoint);
        wi::lua::IntProperty communicationType;
        PropertyFunction(communicationType);
        wi::lua::IntProperty colliderType;
        PropertyFunction(colliderType);
    };

    class Projectile_Bind
    {
    private:
        std::unique_ptr<Gameplay::Component::Projectile> owning;
    public:
        Gameplay::Component::Projectile* component;

        static const char className[];
        static Luna<Projectile_Bind>::FunctionType methods[];
        static Luna<Projectile_Bind>::PropertyType properties[];

        inline void BuildBindings()
        {
            normal = wi::lua::VectorProperty(&component->normal);
            hitbox = wi::lua::IntProperty((int*)&component->hitbox);
        }

        Projectile_Bind(Gameplay::Component::Projectile* component) :component(component) { BuildBindings(); }
        Projectile_Bind(lua_State* L)
        {
            owning = std::make_unique<Gameplay::Component::Projectile>();
            this->component = owning.get();
            BuildBindings();
        }
        
        wi::lua::VectorProperty normal;
        PropertyFunction(normal);
        wi::lua::IntProperty hitbox;
        PropertyFunction(hitbox);
    };

    void Bind();
}