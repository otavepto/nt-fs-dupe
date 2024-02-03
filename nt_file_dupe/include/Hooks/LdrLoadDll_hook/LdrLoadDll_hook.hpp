#pragma once

namespace ntfsdupe::hooks {
    // https://doxygen.reactos.org/d7/d55/ldrapi_8c.html#a8838a6bd5ee2987045215ee7129c3a2c
    NTSTATUS NTAPI LdrLoadDll_hook(
        __in_opt  LPWSTR            LibSearchPath,
        __in_opt  PULONG            DllCharacteristics,
        __in      PUNICODE_STRING   DllName,
        _Out_     HMODULE*          BaseAddress
    );

    extern decltype(LdrLoadDll_hook) *LdrLoadDll_original;

}
