#include "disassembler.hpp"
#include "debugger.hpp"
#include <sstream>

// dbg 
#include "generator/translator.hpp"

namespace dreamware
{
	Disassembler::Disassembler()
	{
		ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_ADDRESS_WIDTH_64);
		ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);

		// cmp     byte ptr [rax+0F1h], 0
		BYTE buf[] = { 0x80, 0xB8, 0xF1, 00, 00, 00, 00 };

		// cmp     dword ptr [rax+5FB0h], 0
		// BYTE buf[] = { 0x83, 0xB8, 0xB0, 0x5F, 00, 00, 00 };

		ZydisDecodedInstruction ins = {};
		ZydisDecoderDecodeBuffer(&decoder, buf, sizeof(buf), &ins);
			
		printf("OPTYPE %d\n", ins.operands[0].element_size / 8);
		
	}

	Disassembler::~Disassembler()
	{
	}

	std::string Disassembler::get_register_name(ZydisRegister target_register)
	{
		std::string name = "";

		if (ZydisRegisterGetWidth(ZYDIS_MACHINE_MODE_LONG_64, target_register) == 32)
			target_register = ZydisRegisterEncode(ZYDIS_REGCLASS_GPR64, ZydisRegisterGetId(target_register));
		name = std::string(ZydisRegisterGetString(target_register));
		name[0] = toupper(name[0]);
		name.insert(0, "c.");
		return name;
	}

	ZydisDecodedInstruction Disassembler::decode(DWORD64 rip)
	{
		BYTE buffer[15] = {};
		ZydisDecodedInstruction instruction = {};

		ReadProcessMemory(debugger.get_process_information()->hProcess, (LPCVOID)rip, buffer, sizeof(buffer), nullptr);
		ZydisDecoderDecodeBuffer(&decoder, buffer, sizeof(buffer), &instruction);
		return instruction;
	}

	std::string Disassembler::format(const ZydisDecodedInstruction* instruction)
	{
		char buffer[MAX_PATH] = {};

		ZydisFormatterFormatInstruction(&formatter, instruction, buffer, sizeof(buffer), NULL);
		return std::string(buffer);
	}

	DWORD64 Disassembler::find_mnemonic(DWORD64 rip, ZydisMnemonic mnemonic, bool skip)
	{
		ZydisDecodedInstruction instruction = {};

		while (true)
		{
			instruction = disassembler.decode(rip);

			if (instruction.mnemonic == mnemonic)
				break;

			rip += instruction.length;
		}

		return skip ? rip + instruction.length : rip;
	}

	DWORD64 Disassembler::find_extended_mov(DWORD64 rip)
	{
		ZydisDecodedInstruction instruction = {};

		while (true)
		{
			instruction = disassembler.decode(rip);

			if (instruction.mnemonic == ZYDIS_MNEMONIC_MOVZX || instruction.mnemonic == ZYDIS_MNEMONIC_MOVSXD || instruction.mnemonic == ZYDIS_MNEMONIC_MOVSX)
				return rip;

			rip += instruction.length;
		}

		return NULL;
	}

	DWORD64 Disassembler::find_register_set(DWORD64 rip, ZydisRegister target_register)
	{
		ZydisDecodedInstruction instruction = {};

		while (true)
		{
			instruction = disassembler.decode(rip);

			if (instruction.mnemonic == ZYDIS_MNEMONIC_MOV && instruction.operands[0].reg.value == target_register)
				return rip;

			rip += instruction.length;
		}
	}
}

dreamware::Disassembler disassembler;
