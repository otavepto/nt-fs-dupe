
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winternl.h>

#include "lib_main/lib_main.hpp"
#include "Configs/Configs.hpp"
#include "Hooks/Hooks.hpp"


void CreateFileA_tests()
{
    auto hidden = CreateFileA(
        "example/hideme.txt",
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    CloseHandle(hidden);
    std::cout << "hidden file handle " << (hidden == INVALID_HANDLE_VALUE) << std::endl;

    auto org = CreateFileA(
        "example/myfile.org",
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    CloseHandle(org);
    std::cout << "original file handle " << (org == INVALID_HANDLE_VALUE) << std::endl;

    auto redired = CreateFileA(
        "example/myfile.txt",
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    CloseHandle(redired);
    std::cout << "redirected file handle " << (redired != INVALID_HANDLE_VALUE) << std::endl;

}

void GetFileAttributesA_tests()
{
    auto hidden = GetFileAttributesW(L"example/hideme.txt");
    std::cout << "hidden file = " << (hidden == INVALID_FILE_ATTRIBUTES) << std::endl;

    auto org = GetFileAttributesW(L"example/myfile.org");
    std::cout << "original file = " << (org == INVALID_FILE_ATTRIBUTES) << std::endl;

    auto redired = GetFileAttributesW(L"example/myfile.txt");
    std::cout << "redirected file = " << (redired != INVALID_FILE_ATTRIBUTES) << std::endl;

}

void GetFileSize_tests()
{
    auto org = CreateFileW(
        L"example/myfile.txt",
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    auto org_sz = GetFileSize(org, nullptr);
    CloseHandle(org);
    std::cout << "original file = [" << org_sz << "] " << (org_sz == 8) << std::endl;

}

void FindFirstFileExW_tests()
{
    WIN32_FIND_DATA fdata{};

    auto hidden = FindFirstFileExW(
        L"example/hideme.txt",
        FindExInfoBasic,
        &fdata,
        FindExSearchLimitToDirectories,
        NULL,
        0
    );
    FindClose(hidden);
    std::cout << "hidden file = " << (hidden == INVALID_HANDLE_VALUE) << std::endl;
    hidden = FindFirstFileExW(
        L"example/hideme*",
        FindExInfoBasic,
        &fdata,
        FindExSearchLimitToDirectories,
        NULL,
        0
    );
    FindClose(hidden);
    std::cout << "hidden file = " << (hidden == INVALID_HANDLE_VALUE) << std::endl;

    auto org = FindFirstFileExW(
        L"example/myfile.org",
        FindExInfoBasic,
        &fdata,
        FindExSearchLimitToDirectories,
        NULL,
        0
    );
    FindClose(org);
    std::cout << "original file = " << (org == INVALID_HANDLE_VALUE) << std::endl;
    org = FindFirstFileExW(
        L"example/myfile.o*",
        FindExInfoBasic,
        &fdata,
        FindExSearchLimitToDirectories,
        NULL,
        0
    );
    FindClose(org);
    std::cout << "original file = " << (org == INVALID_HANDLE_VALUE) << std::endl;

    auto redired = FindFirstFileExW(
        L"example/myfile.txt",
        FindExInfoBasic,
        &fdata,
        FindExSearchLimitToDirectories,
        NULL,
        0
    );
    FindClose(redired);
    std::cout << "redirected file = " << (redired != INVALID_HANDLE_VALUE) << std::endl;
    redired = FindFirstFileExW(
        L"example/myfile.tx*",
        FindExInfoBasic,
        &fdata,
        FindExSearchLimitToDirectories,
        NULL,
        0
    );
    FindClose(redired);
    std::cout << "redirected file = " << (redired != INVALID_HANDLE_VALUE) << std::endl;

    auto all = FindFirstFileExW(
        L"example/*",
        FindExInfoBasic,
        &fdata,
        FindExSearchNameMatch,
        NULL,
        0
    );

    BOOL x = FALSE;
    do {
        std::wcout
            << L"file [" << fdata.nFileSizeHigh << L", " << fdata.nFileSizeLow << L"]"
            << L" = " << fdata.cFileName << L" = " << fdata.cAlternateFileName
            << std::endl;
        x = FindNextFileW(all, &fdata);
    } while (x);
    FindClose(all);

}

void std_ifstream_tests()
{
    std::ifstream ss("example/myfile.txt");

    std::string line{};
    std::getline(ss, line);
    ss.close();
    std::cout << "read line: '" << line << "' " << ("original" == line) << std::endl;
}



// TODO
void NtQueryDirectoryObject_tests()
{
    HANDLE dirHandle{};

    //wchar_t dirStr[] = L"\\Device\\HarddiskVolume8";
    //wchar_t dirStr[] = L"\\Device\\HarddiskVolume8\\test\\";
    wchar_t dirStr[] = L"\\Device";
    //wchar_t dirStr[] = L"\\"; // only this syntax works WHY ???

    UNICODE_STRING dirUs;
    dirUs.Length = sizeof(dirStr) - 2;
    dirUs.MaximumLength = sizeof(dirStr);
    dirUs.Buffer = dirStr;

    OBJECT_ATTRIBUTES dirAttr{};
    InitializeObjectAttributes(&dirAttr, &dirUs, 0, NULL, NULL);

    auto opndir = ntfsdupe::hooks::NtOpenDirectoryObject_original(&dirHandle, 1, &dirAttr);

    typedef struct _OBJECT_DIRECTORY_INFORMATION {
        UNICODE_STRING Name;
        UNICODE_STRING TypeName;
    } OBJECT_DIRECTORY_INFORMATION, * POBJECT_DIRECTORY_INFORMATION;

    typedef NTSTATUS(NTAPI* NtQueryDirectoryObject_t)(
        _In_      HANDLE  DirectoryHandle,
        _Out_opt_ PVOID   Buffer,
        _In_      ULONG   Length,
        _In_      BOOLEAN ReturnSingleEntry,
        _In_      BOOLEAN RestartScan,
        _Inout_   PULONG  Context,
        _Out_opt_ PULONG  ReturnLength
        );

    OBJECT_DIRECTORY_INFORMATION bufDir[505]{};

    auto QueryDirectoryObject =
        (NtQueryDirectoryObject_t)GetProcAddress(
            LoadLibraryW(L"ntdll.dll"), "NtQueryDirectoryObject");

    ULONG object_index = 0;
    ULONG data_written = 0;
    auto qdstatus = QueryDirectoryObject(dirHandle, bufDir, sizeof(bufDir), FALSE, FALSE, &object_index, &data_written);


}


// TODO
void NtQueryInformationFile_tests()
{
    auto hndle = CreateFileA(
        //"test\\neutral.txt",
        //"\\\\?\\.\\.\\.\\test\\original.txt",
        //"test\\neutral.txt",
        "test\\original.txt",
        //"test\\redired.txt",
        //"\\test\\original.txt",
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    std::cout << "Error = " << GetLastError() << std::endl;


    // =========================================================================
    // NtQueryInformationFile
    typedef NTSTATUS(__stdcall* NtQueryInformationFile_tt)(
        HANDLE                 FileHandle,
        PIO_STATUS_BLOCK       IoStatusBlock,
        PVOID                  FileInformation,
        ULONG                  Length,
        FILE_INFORMATION_CLASS FileInformationClass
        );

    typedef struct {
        ULONG FileNameLength;
        WCHAR FileName[1];
    } FNI;

    auto ntd = LoadLibraryW(L"ntdll.dll");
    auto qex = (NtQueryInformationFile_tt)GetProcAddress(ntd, "NtQueryInformationFile");

    std::cout << "offset = " << UFIELD_OFFSET(FNI, FileName) << std::endl;

    IO_STATUS_BLOCK sb;
    char aa[sizeof(FNI) + 164 - 4];
    auto res = qex(hndle, &sb, (void*)&aa, sizeof(aa), (FILE_INFORMATION_CLASS)9);

    std::cout << "Error = " << GetLastError() << std::endl;
    // =========================================================================


    CloseHandle(hndle);

}

void loadlib_prevent_tests()
{
    // absolute
    auto mod = std::filesystem::current_path() / "bin" / "Debug" /
#if defined _WIN64
        "x64"
#else
        "Win32"
#endif
        / "test_dll_prevent_load" / "test_dll_prevent_load.dll";
    std::cout << "loading absolute: " << mod << std::endl;

    auto addr = LoadLibraryA(mod.string().c_str());
    std::cout << "LoadLibraryA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryW(mod.wstring().c_str());
    std::cout << "LoadLibraryW " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);
    
    addr = LoadLibraryExA(mod.string().c_str(), NULL, 0);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);
    
    addr = LoadLibraryExA(mod.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_APPLICATION_DIR);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);
    
    addr = LoadLibraryExA(mod.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);
    
    addr = LoadLibraryExA(mod.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);
    
    addr = LoadLibraryExA(mod.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);
    
    addr = LoadLibraryExA(mod.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_USER_DIRS);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);
    
    addr = LoadLibraryExA(mod.string().c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);
    
    addr = LoadLibraryExA(mod.string().c_str(), NULL, LOAD_LIBRARY_SAFE_CURRENT_DIRS);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);
    
    addr = LoadLibraryExW(mod.wstring().c_str(), NULL, 0);
    std::cout << "LoadLibraryExW " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);


    // relative
    auto mod_rel = mod.filename();
    std::cout << "loading relative: " << mod_rel << std::endl;

    addr = LoadLibraryA(mod_rel.string().c_str());
    std::cout << "LoadLibraryA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryW(mod_rel.wstring().c_str());
    std::cout << "LoadLibraryW " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod_rel.string().c_str(), NULL, 0);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod_rel.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_APPLICATION_DIR);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod_rel.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod_rel.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod_rel.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod_rel.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_USER_DIRS);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod_rel.string().c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod_rel.string().c_str(), NULL, LOAD_LIBRARY_SAFE_CURRENT_DIRS);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExW(mod_rel.wstring().c_str(), NULL, 0);
    std::cout << "LoadLibraryExW " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);


}

void loadlib_redirect_tests()
{
    // absolute
    auto mod = std::filesystem::current_path() / "bin" / "Debug" /
#if defined _WIN64
        "x64"
#else
        "Win32"
#endif
        / "test_dll_redirect" / "test_dll_redirect_org.dll";
    std::cout << "loading absolute: " << mod << std::endl;

    auto addr = LoadLibraryA(mod.string().c_str());
    std::cout << "LoadLibraryA " << (int)(addr != nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryW(mod.wstring().c_str());
    std::cout << "LoadLibraryW " << (int)(addr != nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod.string().c_str(), NULL, 0);
    std::cout << "LoadLibraryExA " << (int)(addr != nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_APPLICATION_DIR);
    std::cout << "LoadLibraryExA " << (int)(addr != nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    std::cout << "LoadLibraryExA " << (int)(addr != nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);
    std::cout << "LoadLibraryExA " << (int)(addr != nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    std::cout << "LoadLibraryExA " << (int)(addr != nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_USER_DIRS);
    std::cout << "LoadLibraryExA " << (int)(addr != nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod.string().c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    std::cout << "LoadLibraryExA " << (int)(addr != nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod.string().c_str(), NULL, LOAD_LIBRARY_SAFE_CURRENT_DIRS);
    std::cout << "LoadLibraryExA " << (int)(addr != nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExW(mod.wstring().c_str(), NULL, 0);
    std::cout << "LoadLibraryExW " << (int)(addr != nullptr) << std::endl;
    //FreeLibrary(addr);
    // don't free here because the relative will fail


    // relative
    auto mod_rel = mod.filename();
    std::cout << "loading relative: " << mod_rel << std::endl;

    addr = LoadLibraryA(mod_rel.string().c_str());
    std::cout << "LoadLibraryA " << (int)(addr != nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryW(mod_rel.wstring().c_str());
    std::cout << "LoadLibraryW " << (int)(addr != nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod_rel.string().c_str(), NULL, 0);
    std::cout << "LoadLibraryExA " << (int)(addr != nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod_rel.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_APPLICATION_DIR);
    std::cout << "LoadLibraryExA " << (int)(addr != nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod_rel.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    std::cout << "LoadLibraryExA " << (int)(addr != nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod_rel.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);
    std::cout << "LoadLibraryExA " << (int)(addr != nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod_rel.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    std::cout << "LoadLibraryExA " << (int)(addr != nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod_rel.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_USER_DIRS);
    std::cout << "LoadLibraryExA " << (int)(addr != nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod_rel.string().c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    std::cout << "LoadLibraryExA " << (int)(addr != nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod_rel.string().c_str(), NULL, LOAD_LIBRARY_SAFE_CURRENT_DIRS);
    std::cout << "LoadLibraryExA " << (int)(addr != nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExW(mod_rel.wstring().c_str(), NULL, 0);
    std::cout << "LoadLibraryExW " << (int)(addr != nullptr) << std::endl;
    FreeLibrary(addr);

    // the last absolute one
    FreeLibrary(addr);

}

// target modules should be invisible to the process
void loadlib_redirect_target_tests()
{
    // absolute
    auto mod = std::filesystem::current_path() / "bin" / "Debug" /
#if defined _WIN64
        "x64"
#else
        "Win32"
#endif
        / "test_dll_redirect" / "test_dll_redirect_mod.dll";
    std::cout << "loading absolute: " << mod << std::endl;

    auto addr = LoadLibraryA(mod.string().c_str());
    std::cout << "LoadLibraryA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryW(mod.wstring().c_str());
    std::cout << "LoadLibraryW " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod.string().c_str(), NULL, 0);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_APPLICATION_DIR);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_USER_DIRS);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod.string().c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod.string().c_str(), NULL, LOAD_LIBRARY_SAFE_CURRENT_DIRS);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExW(mod.wstring().c_str(), NULL, 0);
    std::cout << "LoadLibraryExW " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);


    // relative
    auto mod_rel = mod.filename();
    std::cout << "loading relative: " << mod_rel << std::endl;

    addr = LoadLibraryA(mod_rel.string().c_str());
    std::cout << "LoadLibraryA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryW(mod_rel.wstring().c_str());
    std::cout << "LoadLibraryW " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod_rel.string().c_str(), NULL, 0);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod_rel.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_APPLICATION_DIR);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod_rel.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod_rel.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod_rel.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod_rel.string().c_str(), NULL, LOAD_LIBRARY_SEARCH_USER_DIRS);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod_rel.string().c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExA(mod_rel.string().c_str(), NULL, LOAD_LIBRARY_SAFE_CURRENT_DIRS);
    std::cout << "LoadLibraryExA " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

    addr = LoadLibraryExW(mod_rel.wstring().c_str(), NULL, 0);
    std::cout << "LoadLibraryExW " << (int)(addr == nullptr) << std::endl;
    FreeLibrary(addr);

}

void loadlib_hide_handle_tests()
{
    // absolute
    auto mod = std::filesystem::current_path() / "bin" / "Debug" /
#if defined _WIN64
        "x64"
#else
        "Win32"
#endif
        / "test_dll_redirect" / "test_dll_redirect_org.dll";
    std::cout << "loading absolute: " << mod << std::endl;

    auto addr = LoadLibraryA(mod.string().c_str());
    std::cout << "LoadLibraryA " << (int)(addr != nullptr) << std::endl;

    auto mod_addr = GetModuleHandleA(mod.string().c_str());
    std::cout << "GetModuleHandleA " << (int)(addr != nullptr) << std::endl;

    FreeLibrary(addr);

    GetModuleHandleA("kernel32");
    GetModuleHandleA("kernel32.dll");

}



int main(int argc, char** argv)
{
    if (!ntfsdupe::init()) return 1;

    std::filesystem::current_path(ntfsdupe::cfgs::get_exe_dir() + L"../../../../");
    std::cout << "Current dir: '" << std::filesystem::current_path().u8string() << "'" << std::endl;
    if (!ntfsdupe::cfgs::load_file(L"example/sample.json")) return 1;

    std::cout << "\n================== CreateFileA_tests" << std::endl;
    CreateFileA_tests();

    std::cout << "\n================== GetFileAttributesA_tests" << std::endl;
    GetFileAttributesA_tests();

    std::cout << "\n================== GetFileSize_tests" << std::endl;
    GetFileSize_tests();

    std::cout << "\n================== FindFirstFileExW_tests" << std::endl;
    FindFirstFileExW_tests();

    std::cout << "\n================== std_ifstream_tests" << std::endl;
    std_ifstream_tests();
    
    std::cout << "\n================== loadlib_prevent_tests" << std::endl;
    loadlib_prevent_tests();
    
    std::cout << "\n================== loadlib_redirect_tests" << std::endl;
    loadlib_redirect_tests();
    
    std::cout << "\n================== loadlib_redirect_target_tests" << std::endl;
    loadlib_redirect_target_tests();
    
    std::cout << "\n================== loadlib_hide_handle_tests" << std::endl;
    loadlib_hide_handle_tests();

    //GetModuleHandleA(NULL);
    //GetModuleHandleA("example/hideme.txt");
    //LoadLibraryA("example/hideme.txt");
    ntfsdupe::deinit();

    return 0;
}

