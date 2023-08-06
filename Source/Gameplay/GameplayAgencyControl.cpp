#include "GameplayAgencyControl.h"

bool _internal_agency_value = true;
bool Gameplay::GetGameplayAgencyControl()
{
    return _internal_agency_value;
}

void Gameplay::SetGameplayAgencyControl(bool value)
{
    _internal_agency_value = value;
}