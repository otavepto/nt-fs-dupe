#include "Hooks/Hooks.hpp"


namespace ntfsdupe::hooks {
    decltype(NtQueryInformationByName_hook) *NtQueryInformationByName_original = nullptr;
}


NTSTATUS NTAPI ntfsdupe::hooks::NtQueryInformationByName_hook(
    __in  POBJECT_ATTRIBUTES     ObjectAttributes,
    __out PIO_STATUS_BLOCK       IoStatusBlock,
    __out PVOID                  FileInformation,
    __in  ULONG                  Length,
    __in  FILE_INFORMATION_CLASS FileInformationClass
)
{
    auto cfg = find_single_obj_ntpath(ObjectAttributes);
    if (cfg) {
        switch (cfg->mode) {
        case ntfsdupe::cfgs::Type::hide:
        case ntfsdupe::cfgs::Type::target: { // target files are invisible to the process
            NTFSDUPE_DBG(
                L"ntfsdupe::hooks::NtQueryInformationByName_hook hide/target '%s'",
                std::wstring(ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Length / sizeof(wchar_t)).c_str()
            );
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }

        case ntfsdupe::cfgs::Type::original: {
            NTFSDUPE_DBG(L"ntfsdupe::hooks::NtQueryInformationByName_hook original '%s'", cfg->original.c_str());
            // it would be cheaper to just manipulate the original str, but not sure if that's safe
            auto object_name_new = unique_ptr_stack(wchar_t, ObjectAttributes->ObjectName->Length);
            if (object_name_new) {
                copy_new_target(object_name_new.get(), ObjectAttributes->ObjectName, cfg);

                // backup original buffer
                const auto buffer_backup = ObjectAttributes->ObjectName->Buffer;
                // set new buffer
                ObjectAttributes->ObjectName->Buffer = object_name_new.get();
                const auto result = NtQueryInformationByName_original(
                    ObjectAttributes,
                    IoStatusBlock,
                    FileInformation,
                    Length,
                    FileInformationClass
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

    return NtQueryInformationByName_original(
        ObjectAttributes,
        IoStatusBlock,
        FileInformation,
        Length,
        FileInformationClass
    );
}
