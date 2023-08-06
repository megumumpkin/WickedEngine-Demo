#include "Dev.h"
#include "Filesystem.h"
#include "Gameplay.h"

#include <filesystem>

#include <efsw/efsw.hpp>
#include <reproc++/run.hpp>

struct LiveUpdateData
{
    enum class Type
    {
        BINARY,
        WISCENE,
        SCRIPT,
        GAMEPLAYSRC,
        GAMEPLAYDLL
    };
    std::string file;
    efsw::Action action;
    Type type = Type::BINARY;
};
wi::vector<LiveUpdateData> file_changes;
wi::unordered_map<std::string, std::filesystem::file_time_type> file_diff;

class DevFileUpdateListener : public efsw::FileWatchListener
{
public:
    void handleFileAction( efsw::WatchID watchid, const std::string& dir,
                           const std::string& filename, efsw::Action action,
                           std::string oldFilename ) override 
    {
        bool is_new = false;

        std::string filepath = dir+filename;
        std::filesystem::file_time_type latest_diff;
        if(wi::helper::FileExists(filepath)) 
            latest_diff = std::filesystem::last_write_time(dir+"/"+filename);
        auto find_diff = file_diff.find(filename);
        if(find_diff != file_diff.end())
        {
            if(find_diff->second < latest_diff)
                is_new = true;
        }
        else
            is_new = true;

        if(is_new)
        {
            LiveUpdateData fileinfo;
            fileinfo.file = filepath;
            fileinfo.action = action;

            static const wi::unordered_map<std::string, LiveUpdateData::Type> extension_to_type_remap = {
                {"CPP", LiveUpdateData::Type::GAMEPLAYSRC},
                {"C", LiveUpdateData::Type::GAMEPLAYSRC},
                {"HPP", LiveUpdateData::Type::GAMEPLAYSRC},
                {"H", LiveUpdateData::Type::GAMEPLAYSRC},
                {"LUA", LiveUpdateData::Type::SCRIPT},
                {"WISCENE", LiveUpdateData::Type::WISCENE},
            };
            
            auto extension = wi::helper::toUpper(wi::helper::GetExtensionFromFileName(filename));
            auto find_type = extension_to_type_remap.find(extension);
            if(find_type != extension_to_type_remap.end())
                fileinfo.type = find_type->second;
#ifdef _WIN32
            if(filename == "Gameplay_DEV.dll")
#else
            if(filename == "libGameplay_DEV.so")
#endif
                fileinfo.type = LiveUpdateData::Type::GAMEPLAYDLL;

            file_changes.push_back(fileinfo);
            file_diff[filename] = latest_diff;
        }
    }
};

reproc::options shexec_options;

void Dev::LiveUpdate::Init()
{
    auto fileWatcher = new efsw::FileWatcher();
    auto listener = new DevFileUpdateListener();

    auto watch_content = fileWatcher->addWatch(Game::Filesystem::GetActualPath("content/"), listener, true);
    auto watch_gameplaycode = fileWatcher->addWatch("../Source/Gameplay", listener, true);
    auto watch_gameplaydll = fileWatcher->addWatch(wi::helper::GetCurrentPath(), listener,false);

    fileWatcher->watch();

    // shexec_options.redirect.parent = true;
    // shexec_options.nonblocking = true;
    shexec_options.deadline = reproc::infinite;
    reproc::stop_actions shexec_options_stop = {
        { reproc::stop::terminate, reproc::milliseconds(5000) },
        { reproc::stop::kill, reproc::milliseconds(2000) },
        {}
    };
    shexec_options.stop = shexec_options_stop;
}

uint32_t gameplaysrc_update_queue_count = 0;
struct gameplaylib_liveupdate_t
{
    reproc::process build_process;
    wi::jobsystem::context build_process_query;
    bool query_reload = false;
    std::mutex build_mutex;
};
gameplaylib_liveupdate_t gameplaylib_liveupdate;

wi::unordered_map<std::string, wi::vector<uint8_t>> gameplayhook_migration_datas;

void Dev::LiveUpdate::Update()
{
    for(auto& fileinfo : file_changes)
    {
        std::string action_type = 
            (fileinfo.action == efsw::Action::Add) ? "> added!" :
            (fileinfo.action == efsw::Action::Delete) ? "> deleted!" : 
            (fileinfo.action == efsw::Action::Modified) ? "> modified!" : 
            (fileinfo.action == efsw::Action::Moved) ? "> moved!" : "";

        auto& filename = fileinfo.file;
        wi::backlog::post("[Dev][LiveUpdate] <"+filename+action_type);

        if((fileinfo.action == efsw::Action::Modified) && (fileinfo.type == LiveUpdateData::Type::GAMEPLAYSRC))
            gameplaysrc_update_queue_count++;
    }
    file_changes.clear();

    // For gamecode changes we need to queue a build process
    int gameplaysrc_build_process_status = 0;
    std::error_code gameplaysrc_build_process_ec;

    if((!wi::jobsystem::IsBusy(gameplaylib_liveupdate.build_process_query)) && (gameplaysrc_update_queue_count > 0))
    {
        gameplaysrc_update_queue_count = 0;
        wi::vector<std::string> args = {"cmake","--build",".","--config","Debug","--target","Gameplay_DEV"};
        gameplaylib_liveupdate.build_process = reproc::process();
        gameplaylib_liveupdate.build_process.start(args, shexec_options);
        wi::jobsystem::Execute(gameplaylib_liveupdate.build_process_query, [](wi::jobsystem::JobArgs JobArgs){
            reproc::drain(gameplaylib_liveupdate.build_process, reproc::sink::null, reproc::sink::null);
            gameplaylib_liveupdate.build_process.stop(shexec_options.stop);
            std::scoped_lock gameplaylib_update_mutex (gameplaylib_liveupdate.build_mutex);
            gameplaylib_liveupdate.query_reload = true;
        });
    }

    std::scoped_lock gameplaylib_update_mutex (gameplaylib_liveupdate.build_mutex);
    if(gameplaylib_liveupdate.query_reload)
    {
        // Store migration data
        for(auto& gameplay_hook_pair : Gameplay::gameplay_hooks)
        {
            wi::vector<uint8_t> migration_buffer;
            gameplay_hook_pair.second->Hook_Migrate(true, migration_buffer);
            gameplayhook_migration_datas[gameplay_hook_pair.first] = std::move(migration_buffer);
        }
        // Unload hooks
        Gameplay::Deinit();
        // Re-hook
        Gameplay::Init();
        // Restore migration data
        for(auto& gameplay_hook_migration_pair : gameplayhook_migration_datas)
        {
            // wi::vector<uint8_t> migration_buffer;
            // gameplay_hook_pair.second->Hook_Migrate(true, migration_buffer);
            // gameplayhook_migration_datas[gameplay_hook_pair.first] = std::move(migration_buffer);
            Gameplay::gameplay_hooks[gameplay_hook_migration_pair.first]->Hook_Migrate(false, gameplay_hook_migration_pair.second);
        }

        gameplaylib_liveupdate.query_reload = false;
    }
}