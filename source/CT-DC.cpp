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

	
	// Restore Pizza Hut
	{
		nopComparison("81 FB 60 DF BF 0C", 2); // Original

		try
		{
			auto addr = get_pattern("0F 8F ? ? ? ? 74 6B", 6); // Arcade
			Nop(addr, 2);
		}
		TXN_CATCH();
	}

	// Restore FILA
	{
		// Original
		try
		{
			auto addr = get_pattern("81 FB 60 84 BF 0C", 6 + 6);
			Nop(addr, 6);
		}
		TXN_CATCH();

		nopComparison("81 FB A0 58 C0 0C", 2); // Arcade
	}

	// Restore clothes inside FILA
	{
		// Original
		try
		{
			auto original_addr = get_pattern("81 FB C0 91 BF 0C", 6 + 2);
			auto original_jmp_target = get_pattern("81 FB 00 F0 BF 0C", -4 - 5);
			WriteOffsetValue(original_addr, original_jmp_target);
		}
		TXN_CATCH();

		// Arcade
		try
		{
			auto arcade_addr = pattern("8B 3D ? ? ? ? 89 4D 94").get_one();
			auto arcade_jmp_target = get_pattern("81 BD 5C FF FF FF 40 7F 62 0C");

			// mov byte ptr [ebp+var_59], 0
			// mov edi, 0F0h
			// jmp 40857Ch
			Patch(arcade_addr.get<void>(-6), {0xC6, 0x45, 0xA7, 0x00});
			Patch(arcade_addr.get<void>(-6 + 4), {0xBF, 0xF0, 0x00, 0x00, 0x00});
			InjectHook(arcade_addr.get<void>(-6 + 4 + 5), arcade_jmp_target, PATCH_JUMP);
		}
		TXN_CATCH();
	}

	// Restore FILA as destination
	try
	{
		auto addr = get_pattern("3B 10 75 2C", 2);
		Patch<uint8_t>(addr, 0xEB); // jne -> jmp
	}
	TXN_CATCH();
}