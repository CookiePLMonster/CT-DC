#include "Utils/MemoryMgr.h"
#include "Utils/Patterns.h"

void OnInitializeHook()
{
	auto Protect = ScopedUnprotect::UnprotectSectionOrFullModule( GetModuleHandle( nullptr ), ".text" );

	using namespace Memory;
	using namespace hook::txn;

	auto nopComparison = [](std::string_view str, ptrdiff_t offset)
	{
		try
		{
			auto addr = get_pattern(str, offset);
			Patch<uint32_t>(addr, 0xFFFFFFFFu);
		}
		TXN_CATCH();
	};

	// Skip Pizza Hut texture replacements
	{
		nopComparison("81 F9 60 DF BF 0C", 2);

		try
		{
			pattern("81 F9 00 80 E9 0C").count(2).for_each_result([](auto match) {
				Patch<uint32_t>(match.get<void>(2), 0xFFFFFFFFu);
			});
		}
		TXN_CATCH();

		nopComparison("81 F9 A0 B3 C0 0C", 2);
	}

}