#include "Filesystem.h"

namespace Game
{
    namespace Filesystem
    {
        struct _internal_fsdata
        {
            std::string actualpath;
            uint32_t priority = 0; // For file overlay, 0 is lowest

            void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri){};
        };

        wi::ecs::ComponentManager<_internal_fsdata> fsdb;
        
        wi::unordered_map<std::string, wi::ecs::Entity> fslookup;
        wi::unordered_map<std::string, wi::vector<wi::ecs::Entity>> fsoverlaylookup;
        
        std::atomic<wi::ecs::Entity> vfsid_gen { wi::ecs::INVALID_ENTITY + 1 };
        std::atomic<wi::ecs::Entity> fsid_gen { wi::ecs::INVALID_ENTITY + 1 };

        void Register_FS(std::string virtualpath, std::string actualpath, bool file)
        {
            auto fsid = fsid_gen.fetch_add(1);
            auto& fsdata = fsdb.Create(fsid);
            fsdata.actualpath = actualpath;
            fslookup[virtualpath] = fsid;
        }
        void Register_FSOverlay(std::string virtualpath, std::string actualpath, bool file)
        {
            auto fsid = fsid_gen.fetch_add(1);
            auto& fsdata = fsdb.Create(fsid);
            fsdata.actualpath = actualpath;
            
            auto fsoverlayget = fsoverlaylookup.find(virtualpath);
            if(fsoverlayget != fsoverlaylookup.end())
            {
                fsoverlaylookup[virtualpath].push_back(fsid);
            }
            else
            {
                fsoverlaylookup[virtualpath] = { fsid };
            }
        }
        std::string GetActualPath(const std::string& file)
        {
            std::string actual_file = "";
            uint32_t max_priority = 0;
            // Search overlays first and pick priority
            for(auto& fsobj : fsoverlaylookup)
            {
                // Just do the stupid string search technique, DON'T YOU DARE WASTE CPU CYCLES JUST FOR FILE REDIRECTION
                auto fso_str = fsobj.first;
                for(auto& fso_data_id : fsobj.second)
                {
                    auto fso_data = fsdb.GetComponent(fso_data_id);
                    if((std::string(file.substr(0,fso_str.size())).compare(fso_str) == 0) && (fso_data->priority >= max_priority))
                    {
                        // Concat the path and set the maxxest priority
                        actual_file = fso_data->actualpath + file.substr(fso_str.size(), file.size() - fso_str.size());
                        max_priority = fso_data->priority;
                    }
                }
            }

            // If it does not exist on overlay then we can seek from base FS redirect
            if(actual_file.empty())
            {
                for(auto& fsobj : fslookup)
                {
                    // Just do the stupid string search technique, DON'T YOU DARE WASTE CPU CYCLES JUST FOR FILE REDIRECTION
                    auto fso_str = fsobj.first;
                    auto fso_data = fsdb.GetComponent(fsobj.second);
                    if(std::string(file.substr(0,fso_str.size())).compare(fso_str) == 0)
                    {
                        actual_file = fso_data->actualpath + file.substr(fso_str.size(), file.size() - fso_str.size());
                    }
                }
            }

            return actual_file;
        }
        bool FileRead(const std::string &file, wi::vector<uint8_t> &data)
        {
            wi::helper::FileRead(GetActualPath(file), data);
            return false;
        }
        bool FileWrite(const std::string &file, const uint8_t *data, size_t size)
        {
            wi::helper::FileWrite(GetActualPath(file), data, size);
            return false;
        }
        bool FileExists(const std::string &file)
        {
            wi::helper::FileExists(GetActualPath(file));
            return false;
        }
    }
}