#include "Hooks/Hooks.hpp"


namespace ntfsdupe::hooks {
    decltype(NtCreateDirectoryObjectEx_hook) *NtCreateDirectoryObjectEx_original = nullptr;
}


// // https://googleprojectzero.blogspot.com/2016/08/
NTSTATUS NTAPI ntfsdupe::hooks::NtCreateDirectoryObjectEx_hook(
    __out     PHANDLE DirectoryHandle,
    __in      ACCESS_MASK DesiredAccess,
    __in      POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt  HANDLE ShadowDirectoryHandle,
    __in      ULONG Flags
)
{
    auto cfg = find_single_file_obj_ntpath(ObjectAttributes);
    if (cfg) {
        switch (cfg->mode) {
        case ntfsdupe::cfgs::FileType::hide:
        case ntfsdupe::cfgs::FileType::target: { // target files are invisible to the process
            NTFSDUPE_DBG(
                L"ntfsdupe::hooks::NtCreateDirectoryObjectEx_hook hide/target '%s'",
                std::wstring(ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Length / sizeof(wchar_t)).c_str()
            );
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }

        case ntfsdupe::cfgs::FileType::original: {
            NTFSDUPE_DBG(L"ntfsdupe::hooks::NtCreateDirectoryObjectEx_hook original '%s'", cfg->original.c_str());
            // it would be cheaper to just manipulate the original str, but not sure if that's safe
            auto object_name_new = unique_ptr_stack(wchar_t, ObjectAttributes->ObjectName->Length);
            if (object_name_new) {
                copy_new_file_target(object_name_new.get(), ObjectAttributes->ObjectName, cfg);

                // backup original buffer
                const auto buffer_backup = ObjectAttributes->ObjectName->Buffer;
                // set new buffer
                ObjectAttributes->ObjectName->Buffer = object_name_new.get();
                const auto result = NtCreateDirectoryObjectEx_original(
                    DirectoryHandle,
                    DesiredAccess,
                    ObjectAttributes,
                    ShadowDirectoryHandle,
                    Flags
                );
                // restore original buffer
                ObjectAttributes->ObjectName->Buffer = buffer_backup;
                return result;
            }
        }
        break;

        default: break;
        }

    }

    return NtCreateDirectoryObjectEx_original(
        DirectoryHandle,
        DesiredAccess,
        ObjectAttributes,
        ShadowDirectoryHandle,
        Flags
    );
}
