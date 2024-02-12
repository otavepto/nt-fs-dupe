#include "Helpers/Helpers.hpp"
#include "Helpers/dbglog.hpp"
#include "Json/nlohmann/json.hpp"
#include "NtApis/NtApis.hpp"
#include "Configs/Configs.hpp"

#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <filesystem>


namespace ntfsdupe::cfgs {
    using json = nlohmann::json;

    static CRITICAL_SECTION bypass_files_cs{};
    static std::wstring exe_dir{};
    // this storage holds all strings used (actual memory allocation), which are later referenced by the other containers
    // it is way cheaper to use string_view for other containers since it doesn't copy strings, and Ntxxx APIs use
    // strings with length, so just wrapping them is enough
    static std::unordered_set<std::wstring> global_entries_storage{};
    static std::unordered_map<std::wstring_view, ntfsdupe::cfgs::CfgEntry> config_entries{};
    static std::unordered_map<DWORD, std::unordered_set<std::wstring_view>> bypass_entries{};
}


bool ntfsdupe::cfgs::init()
{
    NTFSDUPE_DBG(L"ntfsdupe::cfgs::init()");

    InitializeCriticalSection(&bypass_files_cs);

    exe_dir.clear();
    config_entries.clear();
    bypass_entries.clear();
    
    HMODULE hModule = GetModuleHandleW(nullptr);
    if (!hModule) {
        NTFSDUPE_DBG(L"  couldn't get module handle");
        return false;
    }

    exe_dir = ntfsdupe::helpers::get_module_fullpath(hModule);
    if (exe_dir.empty()) {
        NTFSDUPE_DBG(L"  couldn't get exe dir");
        return false;
    }

    // hide ourself
    //add_entry(Mode::hide, lib_dir);

    exe_dir = exe_dir.substr(0, exe_dir.find_last_of(L'\\') + 1);
    return true;
}

void ntfsdupe::cfgs::deinit(void)
{
    NTFSDUPE_DBG(L"ntfsdupe::cfgs::deinit()");
    
    DeleteCriticalSection(&bypass_files_cs);
    memset(&bypass_files_cs, 0, sizeof(bypass_files_cs));
}

const std::wstring& ntfsdupe::cfgs::get_proc_dir() noexcept
{
    return exe_dir;
}

bool ntfsdupe::cfgs::add_entry(Mode mode, const std::wstring &original, const std::wstring &target, bool must_exist)
{
    NTFSDUPE_DBG(L"ntfsdupe::cfgs::add_entry() %i '%s' '%s'", (int)mode, original.c_str(), target.c_str());

    if (original.empty()) {
        NTFSDUPE_DBG(L"  original str is empty");
        return false;
    }

    if (mode == Mode::redirect && target.empty()) {
        NTFSDUPE_DBG(L"  mode is redirect but target str is empty");
        return false;
    }

    try {
        std::wstring _original = ntfsdupe::helpers::to_absolute(original, exe_dir);
        NTFSDUPE_DBG(L"  absolute original file: '%s'", _original.c_str());
        if (must_exist && !ntfsdupe::helpers::file_exist(_original)) {  // not a problem
            NTFSDUPE_DBG(L"  original file not found");
            return true;
        }

        size_t filename_idx = _original.find_last_of(L'\\') + 1;
        unsigned short filename_bytes = (unsigned short)((_original.size() - filename_idx) * sizeof(_original[0]));
        NTFSDUPE_DBG(L"  filename_idx = %zu, filename_bytes = %u", filename_idx, filename_bytes); // TODO

        std::wstring _target = target.empty() ? target : ntfsdupe::helpers::to_absolute(target, exe_dir);
        NTFSDUPE_DBG(L"  absolute target file: '%s'", _target.c_str());

        switch (mode) {
        case ntfsdupe::cfgs::Mode::hide: {
            const auto &_original_upper_ref = *global_entries_storage.insert(ntfsdupe::helpers::upper(_original)).first;
            CfgEntry &entry = config_entries[_original_upper_ref];
            entry.mode = Type::hide;
            entry.original = _original;
            entry.original_filename = &entry.original[0] + filename_idx;
            entry.filename_bytes = filename_bytes;
        }
        return true;

        case ntfsdupe::cfgs::Mode::redirect: {
            if (_target.size() != _original.size()) {
                NTFSDUPE_DBG(L"  original and target size mismatch");
                return false;
            }
            if (filename_idx != (_target.find_last_of(L'\\') + 1)) {
                NTFSDUPE_DBG(L"  original and target filename_idx mismatch");
                return false;
            }

            std::wstring _original_upper(ntfsdupe::helpers::upper(_original));
            std::wstring _target_upper(ntfsdupe::helpers::upper(_target));
            if (_original_upper.compare(0, filename_idx, _target_upper, 0, filename_idx) != 0) { // path mismatch
                NTFSDUPE_DBG(L"  path mismatch");
                return false;
            }
            if (_original_upper.compare(_target_upper) == 0) { // self redirection
                NTFSDUPE_DBG(L"  self redirection");
                return false;
            }
            if (must_exist && !ntfsdupe::helpers::file_exist(_target)) { // not a problem
                NTFSDUPE_DBG(L"  mode is redirect but target file not found");
                return true;
            }

            {
                const auto& _original_upper_ref = *global_entries_storage.insert(_original_upper).first;
                CfgEntry &entry = config_entries[_original_upper_ref];
                entry.mode = Type::original;
                entry.original = _original;
                entry.original_filename = &entry.original[0] + filename_idx;
                entry.target = _target;
                entry.target_filename = &entry.target[0] + filename_idx;
                entry.filename_bytes = filename_bytes;
            }

            {
                const auto& _target_upper_ref = *global_entries_storage.insert(_target_upper).first;
                CfgEntry &entry = config_entries[_target_upper_ref];
                entry.mode = Type::target;
                entry.original = _original;
                entry.original_filename = &entry.original[0] + filename_idx;
                entry.target = _target;
                entry.target_filename = &entry.target[0] + filename_idx;
                entry.filename_bytes = filename_bytes;
            }
        }
        return true;

        default: NTFSDUPE_DBG(L"  unknown entry mode %i", (int)mode); return false; // unknown type
        }

        return false;
    } catch (const std::exception &e) {
        NTFSDUPE_DBG(e.what());
        return false;
    }
}

bool ntfsdupe::cfgs::load_file(const wchar_t *file)
{
    NTFSDUPE_DBG(L"ntfsdupe::cfgs::load_file() '%s'", file);
    if (!file || !file[0]) {
        NTFSDUPE_DBG(L"  empty file");
        return false;
    }

    try {
        std::ifstream f(file);
        
        if (!f.is_open()) {
            NTFSDUPE_DBG(L" failed to open file");
            return false;
        }

        auto j = json::parse(f);
        f.close();

        for (const auto &item: j.items()) {
            try {
                NTFSDUPE_DBG(L"  parsing new entry");
                std::wstring original(ntfsdupe::helpers::str_to_wstr(item.key()));
                NTFSDUPE_DBG(L"  original '%s'", original.c_str());

                Mode mode;
                std::string mode_str = item.value().value("mode", std::string());
                NTFSDUPE_DBG("  mode = " + mode_str);
                if (mode_str.compare("redirect") == 0) mode = Mode::redirect;
                else if (mode_str.compare("hide") == 0) mode = Mode::hide;
                else return false;

                std::wstring target(
                    ntfsdupe::helpers::str_to_wstr(
                        item.value().value("to", std::string())
                    )
                );
                NTFSDUPE_DBG(L"  target '%s'", target.c_str());

                bool must_exist = item.value().value("must_exist", false);
                NTFSDUPE_DBG(L"  must_exist '%i'", (int)must_exist);

                if (!add_entry(mode, original, target, must_exist)) return false;
            } catch (const std::exception &e) {
                NTFSDUPE_DBG(e.what());
            }
        }

        return true;
    } catch (const std::exception &e) {
        NTFSDUPE_DBG(e.what());
        return false;
    }
}

const ntfsdupe::cfgs::CfgEntry* ntfsdupe::cfgs::find_entry(const std::wstring_view &str) noexcept
{
    if (ntfsdupe::cfgs::config_entries.empty() || is_bypassed(str)) return nullptr;

    const auto &res = ntfsdupe::cfgs::config_entries.find(str);
    return res == ntfsdupe::cfgs::config_entries.end()
        ? nullptr
        : &res->second;
}

void ntfsdupe::cfgs::add_bypass(const std::wstring_view &str) noexcept
{
    EnterCriticalSection(&bypass_files_cs);
    //NTFSDUPE_DBG(L"ntfsdupe::cfgs::add_bypass '%.*s'", (int)str.size(), str.data());
    bypass_entries[GetCurrentThreadId()].insert(str);
    LeaveCriticalSection(&bypass_files_cs);
}

void ntfsdupe::cfgs::remove_bypass(const std::wstring_view &str) noexcept
{
    EnterCriticalSection(&bypass_files_cs);
    //NTFSDUPE_DBG(L"ntfsdupe::cfgs::remove_bypass '%.*s'", (int)str.size(), str.data());
    if (bypass_entries.size()) {
        DWORD tid = GetCurrentThreadId();
        auto &itr = bypass_entries.find(tid);
        if (bypass_entries.end() != itr) {
            if (itr->second.erase(str)) {
                if (itr->second.empty()) {
                    bypass_entries.erase(tid);
                }
            }
        }
    }
    LeaveCriticalSection(&bypass_files_cs);
}

bool ntfsdupe::cfgs::is_bypassed(const std::wstring_view &str) noexcept
{
    bool ret = false;

    EnterCriticalSection(&bypass_files_cs);
    //NTFSDUPE_DBG(L"ntfsdupe::cfgs::is_bypassed '%.*s'", (int)str.size(), str.data());
    if (bypass_entries.size()) {
        auto tid_it = bypass_entries.find(GetCurrentThreadId());
        if (tid_it != bypass_entries.end()) {
            ret = tid_it->second.find(str) != tid_it->second.end();
        }
    }
    LeaveCriticalSection(&bypass_files_cs);
    
    return ret;
}
