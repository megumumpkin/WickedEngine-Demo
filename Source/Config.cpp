#include "Config.h"

#define INI_IMPLEMENTATION
#include "ini.h"
#include "Filesystem.h"

wi::vector<std::string> AppSettings_Strings = {
    "VIDEO",
    "width",
    "height",
    "fullscreen"
};

void AppSettings_Save(AppSettings& settings)
{
    ini_t* ini = ini_create(NULL);
    // [VIDEO]
    int section_index = ini_section_add(ini, AppSettings_Strings[0].c_str(), AppSettings_Strings[0].size());
    
    // width
    std::string setting_value = std::to_string(settings.video_width);
    ini_property_add(ini, section_index, AppSettings_Strings[1].c_str(), AppSettings_Strings[1].size(), setting_value.c_str(), setting_value.size());
    
    // height
    setting_value = std::to_string(settings.video_height);
    ini_property_add(ini, section_index, AppSettings_Strings[2].c_str(), AppSettings_Strings[2].size(), setting_value.c_str(), setting_value.size());
    
    // fullscreen
    setting_value = std::to_string(settings.video_fullscreen);
    ini_property_add(ini, section_index, AppSettings_Strings[3].c_str(), AppSettings_Strings[3].size(), setting_value.c_str(), setting_value.size());

    wi::vector<char> data;
    int size = ini_save(ini, NULL, 0);
    data.resize(size);
    size = ini_save(ini, data.data(), size);
    ini_destroy(ini);

    FILE* fp = fopen( "config.ini", "w" );
	fwrite( data.data(), 1, size-1, fp );
	fclose( fp );
}

AppSettings AppSettings_Load()
{
    AppSettings settings;

    if(!wi::helper::FileExists("config.ini")){
        AppSettings defaults = {
            1280,
            720,
            0
        };
        AppSettings_Save(defaults);
    }
    
    //Check if it does not exist then we need to create it first
    FILE* fp = fopen( "config.ini", "r" );
	fseek( fp, 0, SEEK_END );
	int size = ftell( fp );
	fseek( fp, 0, SEEK_SET );
    wi::vector<char> data;
	// char* data = (char*) malloc( size + 1 );
    data.resize(size+1);
	fread( data.data(), 1, size, fp );
	data[size] = '\0';
	fclose( fp );

	ini_t* ini = ini_load(data.data(), NULL);

    // [VIDEO]
    int section_index = ini_find_section(ini, AppSettings_Strings[0].c_str(), AppSettings_Strings[0].size());

    // width
	int setting_index = ini_find_property(ini, section_index, AppSettings_Strings[1].c_str(), AppSettings_Strings[1].size());
    settings.video_width = atoi(ini_property_value(ini, section_index, setting_index));

    // height
    setting_index = ini_find_property(ini, section_index, AppSettings_Strings[2].c_str(), AppSettings_Strings[2].size());
    settings.video_height = atoi(ini_property_value(ini, section_index, setting_index));

    // fullscreen
    setting_index = ini_find_property(ini, section_index, AppSettings_Strings[3].c_str(), AppSettings_Strings[3].size());
    settings.video_fullscreen = atoi(ini_property_value(ini, section_index, setting_index));

    ini_destroy(ini);

    return settings;
}