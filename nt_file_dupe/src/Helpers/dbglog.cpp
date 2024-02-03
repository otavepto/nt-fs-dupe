#if !defined(NT_FS_DUPE_RELEASE)

#include "Helpers/dbglog.hpp"
#include "Helpers/Helpers.hpp"

#include <stdio.h>
#include <sstream>
#include <cstdarg>
#include <chrono>
#include <mutex>

static const auto start_time = std::chrono::system_clock::now();
static std::recursive_mutex f_mtx{};
static FILE *out_file = nullptr;

bool ntfsdupe::helpers::dbglog::init()
{
    std::lock_guard lk(f_mtx);
    if (!out_file) {
        auto path = ntfsdupe::helpers::get_module_fullpath(nullptr) + L".NT_FS_DUPE.log";
        auto err = _wfopen_s(&out_file, path.c_str(), L"a, ccs=UTF-8");
        if (err == 0) {
            return true;
        } else {
            out_file = nullptr;
        }
    }

    return false;
}

void ntfsdupe::helpers::dbglog::write(const wchar_t *fmt, ...)
{
    std::lock_guard lk(f_mtx);
    if (out_file) {
        auto elapsed = std::chrono::system_clock::now() - start_time;
        std::wstringstream ss{};
        ss << "[" << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() << " ms] [tid: " << std::this_thread::get_id() << "] ";
        auto ss_str = ss.str();
        std::fwprintf(out_file, ss_str.c_str());

        std::va_list args{};
        va_start(args, fmt);
        std::vfwprintf(out_file, fmt, args);
        va_end(args);

        std::fwprintf(out_file, L"\n");
        std::fflush(out_file);
    }
}

void ntfsdupe::helpers::dbglog::write(const std::wstring &str)
{
    std::lock_guard lk(f_mtx);
    if (out_file) {
        write(str.c_str());
    }
}

void ntfsdupe::helpers::dbglog::write(const std::string &str)
{
    std::lock_guard lk(f_mtx);
    if (out_file) {
        std::wstring wstr(ntfsdupe::helpers::str_to_wstr(str));
        ntfsdupe::helpers::dbglog::write(wstr.c_str());
    }
}

void ntfsdupe::helpers::dbglog::close()
{
    std::lock_guard lk(f_mtx);
    if (out_file) {
        std::fwprintf(out_file, L"\nLog file closed\n\n");
        std::fclose(out_file);
        out_file = nullptr;
    }
}

#endif // NT_FS_DUPE_RELEASE
