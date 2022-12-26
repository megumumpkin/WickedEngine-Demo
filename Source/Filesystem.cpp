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
            size_t dirtree_size;
            struct directorydata
            {
                std::string path;
                size_t offset;
                size_t size;
            };
            wi::vector<directorydata> directories;

            // And then the rest are the datablocks, nothing else
        };
        struct _internal_fsdata
        {
            std::string actualpath;
            wi::ecs::Entity vfs_id; // If it is a vfs then can we have the id of the vfs?

            void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri){};
        };

        wi::ecs::ComponentManager<_internal_vfs_header> vfsdb;
        wi::ecs::ComponentManager<_internal_fsdata> fsdb;
        
        std::unordered_map<std::string, wi::ecs::Entity> fslookup;
        std::unordered_map<std::string, wi::vector<wi::ecs::Entity>> fsoverlaylookup;
        
        std::atomic<wi::ecs::Entity> vfsid_gen;
        std::atomic<wi::ecs::Entity> fsid_gen;
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
                auto vfsdata = vfsdb.Create(vfsid);
                _internal_register_VFS(actualpath, vfsdata);
            }

            auto fsid = fsid_gen.fetch_add(1);
            auto fsdata = fsdb.Create(fsid);
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
                auto vfsdata = vfsdb.Create(vfsid);
                _internal_register_VFS(actualpath, vfsdata);
            }


            auto fsid = fsid_gen.fetch_add(1);
            auto fsdata = fsdb.Create(fsid);
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
        bool fstream::Open(std::string dir)
        {
            return false;
        }
        void fstream::Close()
        {

        }
        inline fstream& fstream::operator>> (char& data)
        {
            return *this;
        }
        inline fstream& fstream::operator<< (char data)
        {
            return *this;
        }
    }
}