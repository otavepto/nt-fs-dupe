#include "Helpers/Helpers.hpp"
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
    static std::unordered_map<std::wstring, ntfsdupe::cfgs::CfgEntry> config_file{};
    static std::unordered_map<DWORD, std::unordered_set<std::wstring>> bypass_files{};

    struct CppRt {

        bool destroyed = false;

        ~CppRt() {
            ntfsdupe::cfgs::deinit();
        }
    };

    static CppRt cpp_rt{};
}


bool ntfsdupe::cfgs::init()
{
    if (cpp_rt.destroyed) return false;

    InitializeCriticalSection(&bypass_files_cs);

    exe_dir.clear();
    config_file.clear();
    bypass_files.clear();

    HMODULE hModule = GetModuleHandleW(nullptr);
    if (!hModule) return false;

    exe_dir = ntfsdupe::helpers::get_module_fullpath(hModule);
    if (exe_dir.empty()) return false;

    // hide ourself
    //add_entry(Mode::hide, lib_dir);

    exe_dir = exe_dir.substr(0, exe_dir.find_last_of(L'\\') + 1);
    return true;
}

void ntfsdupe::cfgs::deinit(void)
{
    if (cpp_rt.destroyed) return;

    cpp_rt.destroyed = true;
    exe_dir.clear();
    config_file.clear();
    bypass_files.clear();

    DeleteCriticalSection(&bypass_files_cs);
}

const std::wstring& ntfsdupe::cfgs::get_proc_dir() noexcept
{
    return exe_dir;
}

bool ntfsdupe::cfgs::add_entry(Mode mode, const std::wstring& original, const std::wstring& target)
{
    if (cpp_rt.destroyed) return false;

    if (original.empty()) return false;
    if (mode == Mode::redirect && target.empty()) return false;

    try {
        std::wstring _original = ntfsdupe::helpers::to_absolute(original, exe_dir);
        if (!ntfsdupe::helpers::file_exist(_original)) return true; // not a problem
        size_t filename_idx = _original.find_last_of(L'\\') + 1;
        unsigned short filename_bytes = (unsigned short)((_original.size() - filename_idx) * sizeof(_original[0]));

        std::wstring _target = target.empty() ? target : ntfsdupe::helpers::to_absolute(target, exe_dir);

        switch (mode) {
        case ntfsdupe::cfgs::Mode::hide: {
            CfgEntry &entry = config_file[ntfsdupe::helpers::upper(_original)];
            entry.mode = Type::hide;
            entry.original = _original;
            entry.original_filename = &entry.original[0] + filename_idx;
            entry.filename_bytes = filename_bytes;
        }
        return true;

        case ntfsdupe::cfgs::Mode::redirect: {
            if (_target.size() != _original.size()) return false;

            size_t target_filename_idx = _target.find_last_of(L'\\') + 1;
            if (filename_idx != target_filename_idx) return false;
            
            if (_original.compare(_target) == 0) return false; // self redirection
            if (!ntfsdupe::helpers::file_exist(_target)) return true; // not a problem

            {
                CfgEntry &entry = config_file[ntfsdupe::helpers::upper(_original)];
                entry.mode = Type::original;
                entry.original = _original;
                entry.original_filename = &entry.original[0] + filename_idx;
                entry.target = _target;
                entry.target_filename = &entry.target[0] + filename_idx;
                entry.filename_bytes = filename_bytes;
            }

            {
                CfgEntry &entry = config_file[ntfsdupe::helpers::upper(_target)];
                entry.mode = Type::target;
                entry.original = _original;
                entry.original_filename = &entry.original[0] + filename_idx;
                entry.target = _target;
                entry.target_filename = &entry.target[0] + filename_idx;
                entry.filename_bytes = filename_bytes;
            }
        }
        return true;

        default: return false; // unknown type
        }

        return false;
    } catch (const std::exception&) {
        return false;
    }
}

bool ntfsdupe::cfgs::load_file(const wchar_t* file)
{
    if (!file || !file[0]) return false;

    try {
        std::ifstream f(file);
        
        if (!f.is_open()) return false;

        auto j = json::parse(f);
        f.close();

        for (const auto &item: j.items()) {
            try {
                Mode mode;
                std::string mode_str = item.value().value("mode", std::string());
                if (mode_str.compare("redirect") == 0) mode = Mode::redirect;
                else if (mode_str.compare("hide") == 0) mode = Mode::hide;
                else return false;

                std::wstring original(item.key().begin(), item.key().end());
                std::string target = item.value().value("to", std::string());

                if (!add_entry(mode, original, ntfsdupe::helpers::str_to_wstr(target))) return false;
            } catch (const std::exception&) {

            }
        }

        return true;
    } catch (const std::exception&) {
        return false;
    }
}

const ntfsdupe::cfgs::CfgEntry* ntfsdupe::cfgs::find_entry(const std::wstring_view &str) noexcept {
    if (cpp_rt.destroyed) return nullptr;

    if (!ntfsdupe::cfgs::config_file.size() || is_bypassed(str)) return nullptr;

    const auto &res = ntfsdupe::cfgs::config_file.find(std::wstring(str));
    return res == ntfsdupe::cfgs::config_file.end()
        ? nullptr
        : &res->second;
}

void ntfsdupe::cfgs::add_bypass(const std::wstring_view &str) noexcept {
    if (cpp_rt.destroyed) return;

    EnterCriticalSection(&bypass_files_cs);
    bypass_files[GetCurrentThreadId()].insert(std::wstring(str));
    LeaveCriticalSection(&bypass_files_cs);
}

void ntfsdupe::cfgs::remove_bypass(const std::wstring_view &str) noexcept {
    if (cpp_rt.destroyed) return;

    EnterCriticalSection(&bypass_files_cs);
    if (bypass_files.size()) {
        DWORD tid = GetCurrentThreadId();
        auto &itr = bypass_files.find(tid);
        if (bypass_files.end() != itr) {
            if (itr->second.erase(std::wstring(str))) {
                if (itr->second.empty()) {
                    bypass_files.erase(tid);
                }
            }
        }
    }
    LeaveCriticalSection(&bypass_files_cs);
}

bool ntfsdupe::cfgs::is_bypassed(const std::wstring_view &str) noexcept {
    if (cpp_rt.destroyed) return true;

    bool ret = false;

    EnterCriticalSection(&bypass_files_cs);
    ret = bypass_files.size() &&
        bypass_files.find(GetCurrentThreadId()) != bypass_files.end();
    LeaveCriticalSection(&bypass_files_cs);
    
    return ret;
}
