#include "Utils/MemoryMgr.h"
#include "Utils/Patterns.h"

namespace OriginalNames
{
	const char* PIZZA_HUT = "Pizza Hut";
	const char* LEVIS_STORE = "The Original Levi's\x7E store";
	const char* FILA = "FILA";
	const char* KFC = "Kentucky Fried Chicken";
	const char* TOWER_RECORDS = "Tower Records";
}

namespace OriginalMapVoices
{
	static void* JumpBackAddr;
	static uint32_t *Case1Ptr, *Case2Ptr, *Case3Ptr, *Case4Ptr, *Case5Ptr;

	void __declspec(naked) JumpTable_Case1()
	{
		_asm
		{
			mov		eax, [esi+148h]
			and		eax, 7
			mov		edx, [Case1Ptr]
			mov		edx, [edx+eax*4]
			jmp		JumpBackAddr
		}
	}

	void __declspec(naked) JumpTable_Case2()
	{
		_asm
		{
			mov		eax, [esi+148h]
			and		eax, 7
			mov		edx, [Case2Ptr]
			mov		edx, [edx+eax*4]
			jmp		JumpBackAddr
		}
	}

	void __declspec(naked) JumpTable_Case3()
	{
		_asm
		{
			mov		eax, [esi+148h]
			and		eax, 7
			mov		edx, [Case3Ptr]
			mov		edx, [edx+eax*4]
			jmp		JumpBackAddr
		}
	}

	void __declspec(naked) JumpTable_Case4()
	{
		_asm
		{
			mov		eax, [esi+148h]
			and		eax, 7
			mov		edx, [Case4Ptr]
			mov		edx, [edx+eax*4]
			jmp		JumpBackAddr
		}
	}

	void __declspec(naked) JumpTable_Case5()
	{
		_asm
		{
			mov		eax, [esi+148h]
			and		eax, 7
			mov		edx, [Case5Ptr]
			mov		edx, [edx+eax*4]
			jmp		JumpBackAddr
		}
	}
}

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

	// Restore original world textures
	try
	{
		auto addr = get_pattern("C6 46 70 00 8B 83", 4 + 6);
		auto jmp_dest = get_pattern("83 3D ? ? ? ? 00 74 17 8B 0D");
		InjectHook(addr, jmp_dest, PATCH_JUMP);
	}
	TXN_CATCH();

	// Restore original UI thumbnails
	try
	{
		auto addr = get_pattern("A1 ? ? ? ? 83 F8 03 75 6A", 5);
		auto jmp_dest = get_pattern("39 1D ? ? ? ? 74 16");
		InjectHook(addr, jmp_dest, PATCH_JUMP);
	}
	TXN_CATCH();

	// Restore original destination names
	try
	{
		using namespace OriginalNames;

		auto destinationsArr = *get_pattern<const char***>("8B 04 95 ? ? ? ? D9 1C 24", 3);
		auto destinationsArcade = destinationsArr[0];
		auto destinationsOriginal = destinationsArr[1];

		destinationsArcade[8] = destinationsOriginal[1] = PIZZA_HUT;
		destinationsArcade[11] = destinationsOriginal[2] = FILA;
		destinationsArcade[12] = destinationsOriginal[3] = LEVIS_STORE;
		destinationsArcade[13] = destinationsOriginal[4] = TOWER_RECORDS;
		destinationsArcade[14] = destinationsOriginal[5] = KFC;
	}
	TXN_CATCH();

	// Restore voices referring to licensed destinations
	{
		auto jmp = [](uintptr_t& addr, uintptr_t dest)
		{
			const ptrdiff_t offset = dest - (addr+2);
			if (offset >= INT8_MIN && offset <= INT8_MAX)
			{
				Patch(addr, { 0xEB, static_cast<uint8_t>(offset) });
				addr += 2;
			}
			else
			{
				InjectHook(addr, dest, PATCH_JUMP);
				addr += 5;
			}
		};

		try
		{
			auto jmp_src = reinterpret_cast<uintptr_t>(get_pattern("75 37 8B 86", 2 + 6));
			auto jmp_dest = reinterpret_cast<uintptr_t>(get_pattern("8B 96 ? ? ? ? 83 E2 07 8D 04 80"));

			jmp(jmp_src, jmp_dest);
		}
		TXN_CATCH();

		try
		{
			auto jmp_src = reinterpret_cast<uintptr_t>(get_pattern("85 FF 75 15 8B 86"));
			auto jmp_dest = reinterpret_cast<uintptr_t>(get_pattern("B8 02 00 00 00 E8 ? ? ? ? 83 3D ? ? ? ? ? 75 2C 33 C0"));

			jmp(jmp_src, jmp_dest);
		}
		TXN_CATCH();

		try
		{
			using namespace OriginalMapVoices;

			auto jump_table_pattern = pattern("83 C0 F3 83 F8 0E").get_one();
			auto jump_back_addr = get_pattern("89 15 ? ? ? ? 89 0D ? ? ? ? 8B 0D ? ? ? ? B8");

			auto voices_table = *get_pattern<uint32_t*>("8B 14 95 ? ? ? ? E9", 3);

			JumpBackAddr = jump_back_addr;

			Case1Ptr = voices_table+40;
			Case2Ptr = voices_table+55;
			Case3Ptr = voices_table+60;
			Case4Ptr = voices_table+65;
			Case5Ptr = voices_table+70;

			// Replace with a new jump table
			auto jump_table_ptr = jump_table_pattern.get<void**>(0x13 + 3);

			static const uint8_t indirect_table[] { 0, 1, 2, 3, 4, 12, 12, 12, 12, 12, 12, 12, 5, 12, 6, 7, 8, 9, 12, 12, 12, 10, 11, 11, 11, 11, 11 };
			static void* jump_table[5 + 8] { &JumpTable_Case1, &JumpTable_Case2, &JumpTable_Case3, &JumpTable_Case4, &JumpTable_Case5 };	
			std::copy_n(*jump_table_ptr, 8, jump_table+5);

			Patch<int8_t>(jump_table_pattern.get<void>(2), -1);
			Patch<int8_t>(jump_table_pattern.get<void>(3 + 2), 26);
			Patch(jump_table_pattern.get<void>(0xC + 3), &indirect_table);
			Patch(jump_table_ptr, &jump_table);
		}
		TXN_CATCH();
	}
}