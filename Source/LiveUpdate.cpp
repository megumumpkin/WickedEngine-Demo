#include "LiveUpdate.h"

#include <efsw/efsw.hpp>

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

wi::unordered_map<std::string, FSEvent> fsevents;
std::mutex event_sync;

class FSUpdateListener : public efsw::FileWatchListener
{
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
        fsevents[filename] = {
            action_type,
            wi::helper::toUpper(wi::helper::GetExtensionFromFileName(filename)),
            dir+filename,
        };
    }
};

std::shared_ptr<efsw::FileWatcher> filewatcher;
std::shared_ptr<FSUpdateListener> fwlistener;
efsw::WatchID watch_all;

void Game::LiveUpdate::Init()
{
    filewatcher = std::make_shared<efsw::FileWatcher>();
    fwlistener = std::make_shared<FSUpdateListener>();
    filewatcher->followSymlinks(true);
    filewatcher->allowOutOfScopeLinks(true);
    watch_all = filewatcher->addWatch(wi::helper::GetCurrentPath(), fwlistener.get(), true);
    filewatcher->watch();
}

void Game::LiveUpdate::Update(float dt)
{
    std::scoped_lock lock (event_sync);
    
    for(auto& pair_event : fsevents){
        auto& event = pair_event.second;
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