#include "finder.hpp"
#include "../debugger.hpp"
#include <iostream>
#include "../disassembler.hpp"

namespace dreamware
{
	Finder::Finder()
	{
	}

	Finder::~Finder()
	{
	}

	DWORD64 Finder::find_pattern(bool in_text_section, std::vector<BYTE> pattern)
	{
		DWORD i = NULL;
		size_t j = NULL;
		BYTE* section = nullptr;
		DWORD section_size = NULL;
		DWORD section_address = NULL;

		if (in_text_section)
		{
			section = debugger.text;
			section_size = debugger.text_size;
			section_address = debugger.text_address;
		}
		else
		{
			section = debugger.rdata;
			section_size = debugger.rdata_size;
			section_address = debugger.rdata_address;
		}

		for (i = 0; i < section_size; i++)
		{
			for (j = 0; j < pattern.size(); j++)
			{
				if (pattern[j] != -1 && pattern[j] != section[i + j])
					break;

				if (j + 1 == pattern.size())
					return debugger.get_process_base() + section_address + i;
			}
		}

		return NULL;
	}

	void Finder::find_mw1_info(PINFINITY_WARD_INFO info)
	{
		ZydisDecodedInstruction instruction = {};
		DWORD64 rip = NULL;

		rip = find_pattern(false, { 0x44, 0x48, 0x45, 0x44, 0x45, 0x49, 0x48, 0x47, 0x46 });
		rip = find_pattern(false, make_pattern(rip));
		rip += 8;
		ReadProcessMemory(debugger.get_process_information()->hProcess, (LPCVOID)rip, &rip, sizeof(rip), nullptr);

		rip = disassembler.find_mnemonic(rip, ZYDIS_MNEMONIC_CALL);
		instruction = disassembler.decode(rip);
		rip += instruction.length + (instruction.operands[0].imm.is_signed ? instruction.operands[0].imm.value.s : instruction.operands[0].imm.value.u);
		
		rip = disassembler.find_mnemonic(rip, ZYDIS_MNEMONIC_TEST, true);
		rip = disassembler.find_mnemonic(rip, ZYDIS_MNEMONIC_TEST, true);
		info->cg = rip;
		std::cout << "info->cg " << std::hex << info->cg - debugger.get_process_base() << std::endl;

		rip = disassembler.find_mnemonic(rip, ZYDIS_MNEMONIC_TEST, true);
		rip = disassembler.find_mnemonic(rip, ZYDIS_MNEMONIC_TEST, true);
		info->characters = rip;
		std::cout << "info->characters " << std::hex << info->characters - debugger.get_process_base() << std::endl;

		rip = disassembler.find_mnemonic(rip, ZYDIS_MNEMONIC_TEST, true);
		instruction = disassembler.decode(rip);
		rip += instruction.length + (instruction.operands[0].imm.is_signed ? instruction.operands[0].imm.value.s : instruction.operands[0].imm.value.u);
		rip = disassembler.find_mnemonic(rip, ZYDIS_MNEMONIC_IMUL);
		instruction = disassembler.decode(rip);
		info->character_size = (instruction.operands[2].imm.is_signed ? instruction.operands[2].imm.value.s : instruction.operands[2].imm.value.u);
		std::cout << "info->character_size " << std::hex << info->character_size << std::endl;

		rip = disassembler.find_mnemonic(rip, ZYDIS_MNEMONIC_CMP);
		instruction = disassembler.decode(rip);
		info->character_valid = instruction.operands[0].mem.disp.value;
		info->valid_size = instruction.operands[0].element_size / 8;
		std::cout << "info->character_valid " << std::hex << info->character_valid << std::endl;
		std::cout << "info->valid_size (bytes) " << info->valid_size << std::endl;

		rip = disassembler.find_extended_mov(rip);
		instruction = disassembler.decode(rip);
		info->character_team = instruction.operands[1].mem.disp.value;
		info->team_size = instruction.operands[1].element_size / 8;
		std::cout << "info->character_team " << std::hex << info->character_team << std::endl;
		std::cout << "info->team_size (bytes) " << info->team_size << std::endl;
		
		system("pause");
		// std::cout << "----------->" << std::hex << instruction.operands[0].element_type << std::endl;
	}

	std::vector<BYTE> Finder::make_pattern(DWORD64 value)
	{
		std::vector<BYTE> pattern;
		int i = NULL;
		BYTE j = NULL;

		for (i = 0; i < 8; i++)
		{
			j = (value >> (i * 8));
			if (j)
				pattern.push_back(j);
		}

		return pattern;
	}
}

dreamware::Finder finder;
