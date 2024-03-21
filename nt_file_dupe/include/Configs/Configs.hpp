#pragma once

#include <string>
#include <string_view>

namespace ntfsdupe::cfgs {
    // input from file
    enum class Mode : char {
        file_redirect,
        file_hide,

        module_prevent_load,
        module_redirect,
        module_hide_handle,
    };

    // file operation/action type
    enum class FileType : char {
        original, // current file to redirect (file.dll)
        target, // target file after redirection (file.org)

        hide, // hide this file
    };

    struct FileCfgEntry {
        FileType mode{};

        // strong ref holder
        std::wstring original{};
        const wchar_t *original_filename{};

        // strong ref holder
        std::wstring target{};
        const wchar_t *target_filename{};

        unsigned short filename_bytes{};
    };

    // module operation/action type
    enum class ModuleType : char {
        prevent_load, // prevent dynamic load via LoadLibrary()

        // --- both used when mode = redirect
        original, // current file to redirect (file.dll)
        target, // target file after redirection (file.org)
        // --- 

        hide_handle, // prevent GetModuleHandle(), but allow LoadLibrary()
    };

    struct ModuleCfgEntry {
        ModuleType mode{};

        std::wstring original_filename{};
        std::wstring target_filename{};

        unsigned short filename_bytes{};
    };


    bool init();

    void deinit();

    const std::wstring& get_exe_dir() noexcept;

    bool add_entry(Mode mode, const std::wstring &original, const std::wstring &target = std::wstring(), bool file_must_exist = false);

    bool load_file(const wchar_t *file);

    const FileCfgEntry* find_file_entry(const std::wstring_view &str) noexcept;
    
    const ModuleCfgEntry* find_module_entry(const std::wstring_view &str) noexcept;

    void add_bypass(const std::wstring_view &str) noexcept;
    
    void remove_bypass(const std::wstring_view &str) noexcept;

    bool is_bypassed(const std::wstring_view &str) noexcept;

}

