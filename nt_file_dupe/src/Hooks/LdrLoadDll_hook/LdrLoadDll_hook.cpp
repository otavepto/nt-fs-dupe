#include "Hooks/Hooks.hpp"


namespace ntfsdupe::hooks {
    decltype(LdrLoadDll_hook) *LdrLoadDll_original = nullptr;
}


NTSTATUS NTAPI ntfsdupe::hooks::LdrLoadDll_hook(
    __in_opt  LPVOID            DllCharacteristics,
    __in_opt  LPDWORD           Unknown,
    __in      PUNICODE_STRING   DllName,
    _Out_     HMODULE*          BaseAddress
)
{
    if (DllName && DllName->Length && DllName->Buffer && DllName->Buffer[0]) {
        // try redirection/hiding first
        auto cfg = ntfsdupe::hooks::find_module_obj_filename(DllName);
        if (cfg) switch (cfg->mode)
        {
        // in the case 'hide_handle' we just want to hide it in memory, not prevent loading

        case ntfsdupe::cfgs::ModuleType::prevent_load:
        case ntfsdupe::cfgs::ModuleType::target: { // target files are invisible to the process
            NTFSDUPE_DBG(
                L"ntfsdupe::hooks::LdrLoadDll_hook prevent_load/target '%s'",
                std::wstring(DllName->Buffer, DllName->Length / sizeof(wchar_t)).c_str()
            );
            // this is what the original API returns when you pass a dll name that doesn't exist
            return STATUS_DLL_NOT_FOUND;
        }

        case ntfsdupe::cfgs::ModuleType::original: {
            NTFSDUPE_DBG(
                L"ntfsdupe::hooks::LdrLoadDll_hook original '%s'",
                std::wstring(DllName->Buffer, DllName->Length / sizeof(wchar_t)).c_str()
            );
            // it would be cheaper to just manipulate the original str, but not sure if that's safe
            auto dllname_new = unique_ptr_stack(wchar_t, DllName->Length + sizeof(wchar_t));
            if (dllname_new) {
                ntfsdupe::hooks::copy_new_module_target(dllname_new.get(), DllName, cfg);
                dllname_new.get()[DllName->Length / sizeof(wchar_t)] = L'\0';

                USHORT fullDosPathBytes = 0;
                ntfsdupe::ntapis::GetFullDosPath(NULL, &fullDosPathBytes, dllname_new.get());
                if (fullDosPathBytes) {
                    auto fullDosPath = unique_ptr_stack(wchar_t, fullDosPathBytes);
                    if (fullDosPath) {
                        // convert dos path to full
                        bool conversion = ntfsdupe::ntapis::GetFullDosPath(fullDosPath.get(), &fullDosPathBytes, dllname_new.get());
                        if (conversion) {
                            ntfsdupe::helpers::upper(fullDosPath.get(), fullDosPathBytes / sizeof(wchar_t));
                            std::wstring_view wsv(fullDosPath.get(), fullDosPathBytes / sizeof(wchar_t));

                            ntfsdupe::cfgs::add_bypass(wsv);
                            // backup original buffer
                            const auto buffer_backup = DllName->Buffer;
                            // set new buffer
                            DllName->Buffer = dllname_new.get();
                            // call original API
                            const auto result = LdrLoadDll_original(
                                DllCharacteristics,
                                Unknown,
                                DllName,
                                BaseAddress
                            );
                            // restore original buffer
                            DllName->Buffer = buffer_backup;
                            ntfsdupe::cfgs::remove_bypass(wsv);

                            return result;
                        }
                    }
                }
            }
        }
        break;

        default: break;
        }

        // if redirection/hiding didn't do anything, then bypass this dll to avoid later redirection by NtOpenFile or NtCreateFile
        auto terminated_name = unique_ptr_stack(wchar_t, DllName->Length + sizeof(wchar_t));
        if (terminated_name) {
            memcpy(terminated_name.get(), DllName->Buffer, DllName->Length);
            terminated_name.get()[DllName->Length / sizeof(wchar_t)] = L'\0';

            USHORT fullDosPathBytes = 0;
            ntfsdupe::ntapis::GetFullDosPath(NULL, &fullDosPathBytes, terminated_name.get());
            if (fullDosPathBytes) {
                auto fullDosPath = unique_ptr_stack(wchar_t, fullDosPathBytes);
                if (fullDosPath) {
                    // convert dos path to full
                    bool conversion = ntfsdupe::ntapis::GetFullDosPath(fullDosPath.get(), &fullDosPathBytes, terminated_name.get());
                    if (conversion) {
                        ntfsdupe::helpers::upper(fullDosPath.get(), fullDosPathBytes / sizeof(wchar_t));
                        std::wstring_view wsv(fullDosPath.get(), fullDosPathBytes / sizeof(wchar_t));
                        ntfsdupe::cfgs::add_bypass(wsv);
                        const auto result = LdrLoadDll_original(
                            DllCharacteristics,
                            Unknown,
                            DllName,
                            BaseAddress
                        );
                        ntfsdupe::cfgs::remove_bypass(wsv);
                        return result;
                    }
                }
            }
        }
    }

    // if everything above failed
    return LdrLoadDll_original(
        DllCharacteristics,
        Unknown,
        DllName,
        BaseAddress
    );
}
