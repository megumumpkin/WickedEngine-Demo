#include "Editor.h"
#include <wiLua.h>

Editor::Data* Editor::GetData(){
    static Data data;
    return &data;
}

void Editor::Init(){
    wi::lua::RunText("EditorAPI = true");
}

void Editor::Update(float dt){

}