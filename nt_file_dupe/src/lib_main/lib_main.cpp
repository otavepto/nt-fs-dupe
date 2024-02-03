#include "lib_main/lib_main.hpp"
#include "Helpers/dbglog.hpp"
#include "Configs/Configs.hpp"
#include "NtApis/NtApis.hpp"
#include "Hooks/Hooks.hpp"

namespace ntfsdupe {
	struct CppRt {
		~CppRt() {
			NTFSDUPE_DBG(L"ntfsdupe::CppRt::~CppRt()");
			ntfsdupe::deinit();
		}
	};

	static CppRt cpp_rt{};
}

bool ntfsdupe::init()
{
	NTFSDUPE_DBG_INIT();

	if (!ntfsdupe::cfgs::init()) return false;
	if (!ntfsdupe::ntapis::init()) return false;
	if (!ntfsdupe::hooks::init()) return false;

	return true;
}

void ntfsdupe::deinit()
{
	ntfsdupe::hooks::deinit();
	ntfsdupe::cfgs::deinit();

	NTFSDUPE_DBG_CLOSE();
}
