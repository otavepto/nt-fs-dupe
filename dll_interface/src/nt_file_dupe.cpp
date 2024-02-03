#include "nt_file_dupe.hpp"

#include "Configs/Configs.hpp"
#include "Helpers/Helpers.hpp"
#include "Hooks/Hooks.hpp"

#include <string>
#include <filesystem>


NTFSDUPE_API const wchar_t* NTFSDUPE_DECL ntfsdupe_get_lib_dir()
{
	// returns a static string
	return ntfsdupe::cfgs::get_lib_dir().c_str();
}

NTFSDUPE_API bool NTFSDUPE_DECL ntfsdupe_add_entry(
	ntfsdupe::itf::Mode mode,
	const wchar_t* original,
	const wchar_t* target
)
{
	if (!original || !original[0]) return false;
	
	std::wstring _target{};
	if (target && target[0]) _target = std::wstring(target);

	return ntfsdupe::cfgs::add_entry((ntfsdupe::cfgs::Mode)mode, original, _target);
}

NTFSDUPE_API bool NTFSDUPE_DECL ntfsdupe_load_file(const wchar_t *file)
{
	return ntfsdupe::cfgs::load_file(file);
}

NTFSDUPE_API void NTFSDUPE_DECL ntfsdupe_add_bypass(const wchar_t *file)
{
	size_t len = wcsnlen_s(file, 4096);
	if (len == 4096) return; // invalid string without null terminator

	std::wstring _file(std::filesystem::absolute(std::wstring(file, len)).wstring());
	ntfsdupe::helpers::upper(&_file[0], (int)_file.size());

	ntfsdupe::cfgs::add_bypass(_file);
}

NTFSDUPE_API void NTFSDUPE_DECL ntfsdupe_remove_bypass(const wchar_t* file)
{
	size_t len = wcsnlen_s(file, 4096);
	if (len == 4096) return; // invalid string without null terminator

	std::wstring _file(std::filesystem::absolute(std::wstring(file, len)).wstring());
	ntfsdupe::helpers::upper(&_file[0], (int)_file.size());

	ntfsdupe::cfgs::remove_bypass(file);
}

NTFSDUPE_API void NTFSDUPE_DECL ntfsdupe_deinit()
{
	ntfsdupe::hooks::deinit();
	ntfsdupe::cfgs::deinit();
}