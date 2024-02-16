#ifndef BF67DD03_6548_4EC5_984D_04EF4C0DF748
#define BF67DD03_6548_4EC5_984D_04EF4C0DF748

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <string>
#include <tuple>
#include <sstream>
#include <Zydis/Zydis.h>

namespace dreamware
{
	class Translator
	{
	public:
		Translator();
		~Translator();

		void reset();

		void translate_peb(DWORD64 rip, DWORD64 end);
		void translate_encrypted(DWORD64 rip, DWORD64 end, ZydisRegister encrypted);
		void translate_range(DWORD64 rip, DWORD64 end);
		void track_register_usage(ZydisRegister targetRegister, int i);
		void fix_translation(ZydisRegister targetRegister);

		std::string translate_register_operand(ZydisDecodedOperand* operand);
		std::string translate_memory_operand(DWORD64& rip, ZydisDecodedInstruction* instruction, ZydisDecodedOperand* operand);
		std::string translate_immediate_operand(ZydisDecodedOperand* operand);

		std::string translate_mov(DWORD64& rip, ZydisDecodedInstruction* instruction, ZydisDecodedOperand* operand);
		std::string translate_lea(DWORD64& rip, ZydisDecodedInstruction* instruction, ZydisDecodedOperand* operand);
		std::string translate_add(DWORD64& rip, ZydisDecodedInstruction* instruction, ZydisDecodedOperand* operand);
		std::string translate_sub(DWORD64& rip, ZydisDecodedInstruction* instruction, ZydisDecodedOperand* operand);
		std::string translate_imul(DWORD64& rip, ZydisDecodedInstruction* instruction, ZydisDecodedOperand* operand);
		std::string translate_and(DWORD64& rip, ZydisDecodedInstruction* instruction, ZydisDecodedOperand* operand);
		std::string translate_shl(DWORD64& rip, ZydisDecodedInstruction* instruction, ZydisDecodedOperand* operand);
		std::string translate_shr(DWORD64& rip, ZydisDecodedInstruction* instruction, ZydisDecodedOperand* operand);
		std::string translate_not(DWORD64& rip, ZydisDecodedInstruction* instruction, ZydisDecodedOperand* operand);
		std::string translate_rol(DWORD64& rip, ZydisDecodedInstruction* instruction, ZydisDecodedOperand* operand);
		std::string translate_ror(DWORD64& rip, ZydisDecodedInstruction* instruction, ZydisDecodedOperand* operand);
		std::string translate_bswap(DWORD64& rip, ZydisDecodedInstruction* instruction, ZydisDecodedOperand* operand);
		std::string translate_xor(DWORD64& rip, ZydisDecodedInstruction* instruction, ZydisDecodedOperand* operand);

		void translate_instructions(bool ignore_fix = false);
		void translate_location(DWORD64 rip, bool is_switch = true);

	private:
		std::vector<std::tuple<DWORD64, ZydisDecodedInstruction, bool>> instructions;
	};
}

extern dreamware::Translator translator;

#endif // BF67DD03_6548_4EC5_984D_04EF4C0DF748
