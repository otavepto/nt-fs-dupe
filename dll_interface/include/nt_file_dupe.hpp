#pragma once


#if defined(NTFSDUPE_EXPORTS)
	#define NTFSDUPE_API extern "C"
#else
	#define NTFSDUPE_API extern "C" __declspec(dllimport)
#endif

#if defined(_WIN32)
	#define NTFSDUPE_DECL __stdcall
#else
	#define NTFSDUPE_DECL __fastcall
#endif


// ntfsdupe::itf works for language >= C++17
namespace ntfsdupe {
namespace itf {
	// mirror of ntfsdupe::cfgs::Mode
	// to avoid including .hpp files from the static lib
	enum class Mode : char {
		redirect,
		hide,
	};
}
}

NTFSDUPE_API bool NTFSDUPE_DECL ntfsdupe_add_entry(
	ntfsdupe::itf::Mode mode,
	const wchar_t *original,
	const wchar_t *target,
	bool must_exist
);

NTFSDUPE_API bool NTFSDUPE_DECL ntfsdupe_load_file(const wchar_t *file);

NTFSDUPE_API void NTFSDUPE_DECL ntfsdupe_add_bypass(const wchar_t *file);

NTFSDUPE_API void NTFSDUPE_DECL ntfsdupe_remove_bypass(const wchar_t *file);

NTFSDUPE_API void NTFSDUPE_DECL ntfsdupe_deinit();
