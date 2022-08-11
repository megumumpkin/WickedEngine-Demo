#include "LiveUpdate.h"

#include "BindLua.h"
#include "Resources.h"

#include <mutex>
#include <efsw/efsw.hpp>
#include <WickedEngine.h>
#include <wiBacklog.h>
#include <wiHelper.h>

struct FSEvent{
    enum TYPE{
        ADD,
        MODIFY,
        DELETE
    };
    TYPE type;
    std::string filetype;
    std::string filepath;
};

std::vector<FSEvent> fsevents;
std::mutex event_sync;

class FSUpdateListener : public efsw::FileWatchListener{
    void handleFileAction(efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action, std::string oldFilename) override
    {
        bool store = false;
        FSEvent::TYPE action_type;
        switch(action){
            case efsw::Actions::Add:
            {
                store = true;
                action_type = FSEvent::ADD;
                break;
            }
            case efsw::Actions::Modified:
            {
                store = true;
                action_type = FSEvent::MODIFY;
                break;
            }
            case efsw::Actions::Delete:
            {
                store = true;
                action_type = FSEvent::DELETE;
                break;
            }
            default:
            break;
        }
        std::scoped_lock lock (event_sync);
        if(store) fsevents.push_back({
            action_type,
            wi::helper::toUpper(wi::helper::GetExtensionFromFileName(filename)),
            dir+filename,
        });
    }
};

std::shared_ptr<efsw::FileWatcher> filewatcher;
std::shared_ptr<FSUpdateListener> fwlistener;
efsw::WatchID watch_front;
efsw::WatchID watch_asset;

void Game::LiveUpdate::Init(){
    filewatcher = std::make_shared<efsw::FileWatcher>();
    fwlistener = std::make_shared<FSUpdateListener>();
    watch_front = filewatcher->addWatch(wi::helper::GetCurrentPath(), fwlistener.get(), true);
    watch_asset = filewatcher->addWatch(wi::helper::GetCurrentPath()+"/Data/Asset", fwlistener.get(), true);
    filewatcher->watch();
}

void Game::LiveUpdate::Update(float dt){
    std::scoped_lock lock (event_sync);
    
    for(auto& event : fsevents){
        if(event.filetype == "LUA"){
            Game::ScriptBindings::LiveUpdate::PushEvent({
                (event.type == FSEvent::DELETE) ? Game::ScriptBindings::LiveUpdate::ScriptReloadEvent::UNLOAD : Game::ScriptBindings::LiveUpdate::ScriptReloadEvent::RELOAD,
                Game::ScriptBindings::LiveUpdate::ScriptReloadEvent::LUA,
                event.filepath
            });
        }
    }
    fsevents.clear();

    Resources::LiveUpdate::Update(dt);
    ScriptBindings::LiveUpdate::Update(dt);
}