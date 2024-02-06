# Nt Filesystem Dupe  
A library for file redirection and hiding by hooking various Nt APIs.  
This project is inspired by `CODEX` Steam emu and based on reversing it. Credits to them.

---

## Solution structure
The solution is divided into 3 projects:
1. `nt_file_dupe`: The actual library, this is a static library (`.lib`)
2. `dll_interface`: A thin wrapper `.dll` project around the static library, exporting the necessary functions
3. `tests`: A simple console app to test the library

## JSON file format:
```json
{
  "myfile.txt": {
    "mode": "redirect",
    "to": "myfile.org",
    "must_exist": true
  },
  "path/myfile_22.txt": {
    "mode": "redirect",
    "to": "path/myfile_22.org",
  },
  "../../folder/some_file.txt": {
    "mode": "redirect",
    "to": "../../folder/some_file.org",
  },
  "hideme.txt": {
    "mode": "hide"
  },
  "../hideme_22.txt": {
    "mode": "hide"
  }
}
```
Each JSON key is considered the original file, its path could be absolute or relative.  

The value/object for that key defines the action for the original file:
* `mode`: could be `redirect` or `hide`
* `to`: only useful when `mode` = `redirect`, this defines which target file to redirect the original file to
* `must_exist` (default = `false`): when set to `true`, the JSON entry will be skipped if the original file doesn't exist, or the target file doesn't exist in `redirect` mode

Check the example [sample.json](./example/sample.json)

## Behavior
Relative paths will be relative to the **location of the current `.exe`**, not the current directory.  

Both the original and target files must:  
- Have the same filename length
- Exist in the same dir  

Target files are hidden by default, for example, in the JSON file defined above, `myfile.org` will be hidden.

If `must_exist` = `true`, then JSON entries with a missing original file will be ignored without an error.  

The dll will try to load one of the following files upon loading with that order:
* A JSON file with the same name as the `.dll`
* `nt_file_dupe.json`
* `nt_file_dupe_config.json`
* `nt_fs_dupe.json`
* `nt_fs_dupe_config.json`
* `nt_dupe.json`
* `nt_dupe_config.json`  

Any of these files, if needed to be loaded, must exist **beside** the `.dll`, not the current running `.exe`

## How to link and use as a static lib (.lib):
1. Open the Visual Studio solution file `nt_file_dupe.sln` and build the project `nt_file_dupe`.  
   Make sure to select the right architecture (`x64` or `x86`) and build type (`release` or `debug`)
2. Assuming for example you've selected `Debug | x64`, the library will be built in  
   ```batch
   bin\x64\nt_file_dupe\nt_file_dupe_static.lib
   ```
3. In your own Visual Studio project, you must use C++ language version >= `C++17`,  
   add the static `.lib` file as an input to the linker: [.lib files as linker input](https://learn.microsoft.com/en-us/cpp/build/reference/dot-lib-files-as-linker-input#to-add-lib-files-as-linker-input-in-the-development-environment)
4. Finally, add the folder `nt_file_dupe\include` as an extra include directory: [Additional include directories](https://learn.microsoft.com/en-us/cpp/build/reference/i-additional-include-directories#to-set-this-compiler-option-in-the-visual-studio-development-environment)
5. Everything will be under the namespace `ntfsdupe::`
6. Check how the `.dll` wrapper project is importing the required `.hpp` files, and using the library in its [DllMain](./dll_interface/dllmain.cpp)

## How to link and use as a dynamic lib (.dll):
1. Open the Visual Studio solution file `nt_file_dupe.sln` and build the project `dll_interface`.  
   Make sure to select the right architecture (`x64` or `x86`) and build type (`release` or `debug`)
2. Assuming for example you've selected `Debug | x64`, the library will be built as 2 parts in  
   ```batch
   bin\Debug\x64\dll_interface\nt_file_dupe.dll
   bin\Debug\x64\dll_interface\nt_file_dupe.lib
   ```
   Notice that the `.dll` file must be accompanied by a small-sized `.lib` file which we'll use next
3. In your own Visual Studio project, you must use C++ language version >= `C++17`,  
   add the static `.lib` file as an input to the linker: [.lib files as linker input](https://learn.microsoft.com/en-us/cpp/build/reference/dot-lib-files-as-linker-input#to-add-lib-files-as-linker-input-in-the-development-environment)
4. Finally, add the folder `dll_interface\include` as an extra include directory: [Additional include directories](https://learn.microsoft.com/en-us/cpp/build/reference/i-additional-include-directories#to-set-this-compiler-option-in-the-visual-studio-development-environment)
5. All available exports will have this prefix `ntfsdupe_`
   ```c++
   #include "nt_file_dupe.hpp"

   int main() {
       ntfsdupe_load_file(L"myfile.json");
       ntfsdupe_add_entry(ntfsdupe::itf::Mode::hide, L"some_file.dll", nullptr);
       return 0;
   }
   ```
6. The file `nt_file_dupe.dll` will be added to your imports table, so make sure to copy this `.dll` file beside your project's build output
