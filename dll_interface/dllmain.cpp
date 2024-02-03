#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "lib_main/lib_main.hpp"
#include "Helpers/Helpers.hpp"
#include "Configs/Configs.hpp"

#include <filesystem>
#include <string>

std::vector<std::wstring> initial_files = {
    // the current name of the dll is added here as a first entry
    L"nt_file_dupe.json",
    L"nt_file_dupe_config.json",
    L"nt_fs_dupe.json",
    L"nt_fs_dupe_config.json",
    L"nt_dupe.json",
    L"nt_dupe_config.json",
};

BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        if (!ntfsdupe::init()) return FALSE;

        std::wstring my_path_str(ntfsdupe::helpers::get_module_fullpath(hModule));
        if (my_path_str.empty()) return FALSE;

        // hide ourself
        if (!ntfsdupe::cfgs::add_entry(ntfsdupe::cfgs::Mode::hide, my_path_str)) return FALSE;

        // add <dll name>.json to the list
        auto my_path = std::filesystem::path(my_path_str);
        initial_files.insert(initial_files.begin(), my_path.stem().wstring() + L".json");

        // try to load some files by default
        auto my_dir = my_path.parent_path();
        for (const auto &file : initial_files) {
            auto cfg_file = (my_dir / file).wstring();
            if (ntfsdupe::cfgs::load_file(cfg_file.c_str())) {
                // hiding this file isn't really critical, right?
                ntfsdupe::cfgs::add_entry(ntfsdupe::cfgs::Mode::hide, cfg_file);
                break;
            }
        }

    }
    break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    break;

    case DLL_PROCESS_DETACH:
        ntfsdupe::deinit();
    break;
    }

    return TRUE;
}

