#include "Hooks/Hooks.hpp"


namespace ntfsdupe::hooks {
    decltype(LdrLoadDll_hook) *LdrLoadDll_original = nullptr;
}


NTSTATUS NTAPI ntfsdupe::hooks::LdrLoadDll_hook(
    __in_opt  LPWSTR            LibSearchPath,
    __in_opt  PULONG            DllCharacteristics,
    __in      PUNICODE_STRING   DllName,
    _Out_     HMODULE*          BaseAddress
)
{
    bool should_skip_api = false;
    NTSTATUS ret = STATUS_DLL_NOT_FOUND;

    if (DllName && DllName->Length && DllName->Buffer && DllName->Buffer[0]) {
        USHORT terminated_name_bytes = (USHORT)(DllName->Length + sizeof(wchar_t));
        auto terminated_name = unique_ptr_stack(wchar_t, terminated_name_bytes);
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
                        should_skip_api = true;
                        ntfsdupe::helpers::upper(fullDosPath.get(), fullDosPathBytes / sizeof(wchar_t));
                        std::wstring_view wsv(fullDosPath.get(), fullDosPathBytes / sizeof(wchar_t));
                        ntfsdupe::cfgs::add_bypass(wsv);
                        ret = LdrLoadDll_original(
                            LibSearchPath,
                            DllCharacteristics,
                            DllName,
                            BaseAddress
                        );
                        ntfsdupe::cfgs::remove_bypass(wsv);
                    }
                }
            }
        }
    }

    if (!should_skip_api) {
        ret = LdrLoadDll_original(
            LibSearchPath,
            DllCharacteristics,
            DllName,
            BaseAddress
        );
    }
    return ret;
}
