#pragma once

#include "stdafx.h"

namespace Game::LiveUpdate{
    void Init();
    void Update(float dt);

    void IgnoreFile(std::string& file);
    void IgnoreFileRemove(std::string& file);
}