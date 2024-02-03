#include "Hooks/Hooks.hpp"


namespace ntfsdupe::hooks {
    decltype(NtQueryFullAttributesFile_hook) *NtQueryFullAttributesFile_original = nullptr;
}

NTSTATUS NTAPI ntfsdupe::hooks::NtQueryFullAttributesFile_hook(
    __in  POBJECT_ATTRIBUTES             ObjectAttributes,
    __out PFILE_NETWORK_OPEN_INFORMATION FileInformation
)
{
    auto cfg = find_single_obj_ntpath(ObjectAttributes);
    if (cfg) {
        switch (cfg->mode) {
        case ntfsdupe::cfgs::Type::hide:
        case ntfsdupe::cfgs::Type::target: { // target files are invisible to the process
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }

        case ntfsdupe::cfgs::Type::original: {
            // it would be cheaper to just manipulate the original str, but not sure if that's safe
            auto object_name_new = unique_ptr_stack(wchar_t, ObjectAttributes->ObjectName->Length);
            if (object_name_new) {
                copy_new_target(object_name_new.get(), ObjectAttributes->ObjectName, cfg);

                // backup original buffer
                const auto buffer_backup = ObjectAttributes->ObjectName->Buffer;
                // set new buffer
                ObjectAttributes->ObjectName->Buffer = object_name_new.get();
                const auto result = NtQueryFullAttributesFile_original(
                    ObjectAttributes,
                    FileInformation
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

    return NtQueryFullAttributesFile_original(
        ObjectAttributes,
        FileInformation
    );
}

