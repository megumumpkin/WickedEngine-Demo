#include "Filesystem.h"

namespace Game
{
    namespace Filesystem
    {
        // Virtual file system file, with the extension .DATA
        // Uses 4kb as the base block size for offset (with maximum of 2TB per file)
        // https://pclt.sites.yale.edu/disk-block-size for the block size reference
        struct _internal_vfs_header
        {
            uint32_t HEAD = 0x52564653; //RFVS
            uint32_t MAJOR = 1;
            uint32_t REVISION = 0;

            // File path structure
            struct directorydata
            {
                std::string path;
                size_t offset;
                size_t size;
            };
            wi::vector<directorydata> directories;

            // And then the rest are the datablocks, nothing else

            void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri){};
        };

        struct _internal_fsdata
        {
            std::string actualpath;
            wi::ecs::Entity vfs_id = wi::ecs::INVALID_ENTITY; // If it is a vfs then can we have the id of the vfs?
            uint32_t priority = 0; // For file overlay, 0 is lowest

            void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri){};
        };

        wi::ecs::ComponentManager<_internal_vfs_header> vfsdb;
        wi::ecs::ComponentManager<_internal_fsdata> fsdb;
        
        std::unordered_map<std::string, wi::ecs::Entity> fslookup;
        std::unordered_map<std::string, wi::vector<wi::ecs::Entity>> fsoverlaylookup;
        
        std::atomic<wi::ecs::Entity> vfsid_gen { wi::ecs::INVALID_ENTITY + 1 };
        std::atomic<wi::ecs::Entity> fsid_gen { wi::ecs::INVALID_ENTITY + 1 };

        // Register vfs structure to memory for fast lookup
        // No datablocks are loaded here
        void _internal_register_VFS(std::string file, _internal_vfs_header& vfs_header)
        {
            
        }
        void Register_FS(std::string virtualpath, std::string actualpath, bool file)
        {
            wi::ecs::Entity vfsid;
            if(file)
            {
                vfsid = vfsid_gen.fetch_add(1);
                auto& vfsdata = vfsdb.Create(vfsid);
                _internal_register_VFS(actualpath, vfsdata);
            }

            auto fsid = fsid_gen.fetch_add(1);
            auto& fsdata = fsdb.Create(fsid);
            fsdata.actualpath = actualpath;
            fsdata.vfs_id = vfsid;
            fslookup[virtualpath] = fsid;
        }
        void Register_FSOverlay(std::string virtualpath, std::string actualpath, bool file)
        {
            wi::ecs::Entity vfsid;
            if(file)
            {
                vfsid = vfsid_gen.fetch_add(1);
                auto& vfsdata = vfsdb.Create(vfsid);
                _internal_register_VFS(actualpath, vfsdata);
            }


            auto fsid = fsid_gen.fetch_add(1);
            auto& fsdata = fsdb.Create(fsid);
            fsdata.actualpath = actualpath;
            fsdata.vfs_id = vfsid;
            
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