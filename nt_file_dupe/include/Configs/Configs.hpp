#pragma once

#include <string>
#include <string_view>

namespace ntfsdupe::cfgs {
    // input from file
    enum class Mode : char {
        redirect,
        hide,
    };

    // action per file
    enum class Type : char {
        original, // original file to redirect (file.dll)
        target, // target file after redirection (file.org)
        hide, // hide this file
    };

    struct CfgEntry {
        Type mode;

        std::wstring original;
        const wchar_t *original_filename;

        std::wstring target;
        const wchar_t *target_filename;

        unsigned short filename_bytes;
    };


    bool init();

    void deinit(void);

    const std::wstring& get_proc_dir() noexcept;

    bool add_entry(Mode mode, const std::wstring &original, const std::wstring &target = std::wstring());

    bool load_file(const wchar_t* file);

    const CfgEntry* find_entry(const std::wstring_view &str) noexcept;

    void add_bypass(const std::wstring_view& str) noexcept;
    
    void remove_bypass(const std::wstring_view& str) noexcept;

    bool is_bypassed(const std::wstring_view& str) noexcept;

}

