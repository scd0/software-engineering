#ifndef EAB27974_62CC_4E08_AC0C_DA3845387178
#define EAB27974_62CC_4E08_AC0C_DA3845387178

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <Zydis/Zydis.h>

namespace dreamware
{
	class Disassembler
	{
	public:
		Disassembler();
		~Disassembler();

		std::string get_register_name(ZydisRegister target_register);

		ZydisDecodedInstruction decode(DWORD64 rip);
		std::string format(const ZydisDecodedInstruction* instruction);
		DWORD64 find_mnemonic(DWORD64 rip, ZydisMnemonic mnemonic, bool skip = false);
		DWORD64 find_extended_mov(DWORD64 rip);
		DWORD64 find_register_set(DWORD64 rip, ZydisRegister target_register);

	private:
		ZydisDecoder decoder;
		ZydisFormatter formatter;
	};
}

extern dreamware::Disassembler disassembler;

#endif // EAB27974_62CC_4E08_AC0C_DA3845387178
