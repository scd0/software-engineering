#include "translator.hpp"
#include "../debugger.hpp"
#include "../disassembler.hpp"
#include <algorithm>
#include "generator.hpp"
#include <iomanip>

namespace dreamware
{
	Translator::Translator()
	{
	}

	Translator::~Translator()
	{
	}

	void Translator::reset()
	{
		instructions.clear();
		debugger.reset_stack();
	}

	void Translator::translate_peb(DWORD64 rip, DWORD64 end)
	{
		ZydisDecodedInstruction instruction = {};

		while (rip < end)
		{
			instruction = disassembler.decode(rip);

			if (instruction.mnemonic == ZYDIS_MNEMONIC_MOV && instruction.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER && instruction.operands[1].mem.segment == ZYDIS_REGISTER_GS)
			{
				instructions.push_back(std::make_tuple(rip, instruction, false));
				return;
			}

			rip += instruction.length;
		}
	}

	void Translator::translate_encrypted(DWORD64 rip, DWORD64 end, ZydisRegister encrypted)
	{
		ZydisDecodedInstruction instruction = {};

		while (rip < end)
		{
			instruction = disassembler.decode(rip);

			if (instruction.operands[0].reg.value == encrypted)
				instructions.push_back(std::make_tuple(rip, instruction, false));

			rip += instruction.length;
		}
	}

	void Translator::translate_range(DWORD64 rip, DWORD64 end)
	{
		ZydisDecodedInstruction instruction = {};

		debugger.set_register(ZYDIS_REGISTER_RIP, rip);

		while (true)
		{
			rip = debugger.get_context().Rip;
			if (rip >= end)
				return;

			instruction = disassembler.decode(rip);
			if (instruction.mnemonic != ZYDIS_MNEMONIC_NOP && instruction.mnemonic != ZYDIS_MNEMONIC_CMP && instruction.mnemonic != ZYDIS_MNEMONIC_JNZ && instruction.mnemonic != ZYDIS_MNEMONIC_JZ && instruction.mnemonic != ZYDIS_MNEMONIC_TEST && instruction.mnemonic != ZYDIS_MNEMONIC_JMP && instruction.mnemonic != ZYDIS_MNEMONIC_JNBE && instruction.mnemonic != ZYDIS_MNEMONIC_JNL)
				instructions.push_back(std::make_tuple(rip, instruction, false));

			debugger.single_step();
		}
	}

	void Translator::track_register_usage(ZydisRegister target_register, int i)
	{
		int j = NULL;
		ZydisDecodedInstruction* instruction = nullptr;

		for (j = i; j >= 0; --j)
		{
			instruction = &std::get<1>(instructions[j]);

			if (instruction->operands[0].reg.value == target_register)
			{
				std::get<2>(instructions[j]) = true;

				if (instruction->operand_count > 1)
				{
					if (instruction->operands[1].type == 1)
						track_register_usage(instruction->operands[1].reg.value, j - 1);
					else if (instruction->operands[1].type == 2)
					{
						if (instruction->operands[1].mem.base != ZYDIS_REGISTER_RIP)
						{
							if (instruction->operands[1].mem.base != ZYDIS_REGISTER_NONE)
								track_register_usage(instruction->operands[1].mem.base, j - 1);
							if (instruction->operands[1].mem.index != ZYDIS_REGISTER_NONE)
								track_register_usage(instruction->operands[1].mem.index, j - 1);
						}
					}
					else if (instruction->operands[1].type != 4)
						printf("==ERROR2== CANNOT TRACK UNKNOWN OPERAND TYPE %d (%s)\n", instruction->operands[1].type, disassembler.format(instruction).c_str());

					if (instruction->mnemonic == ZYDIS_MNEMONIC_MOV || instruction->mnemonic == ZYDIS_MNEMONIC_LEA || (instruction->mnemonic == ZYDIS_MNEMONIC_AND && instruction->operands[1].imm.value.u == 0xffffffffc0000000))
						return;
				}
			}
		}
	}

	void Translator::fix_translation(ZydisRegister target_register)
	{
		int i = NULL;
		ZydisDecodedInstruction* instruction = nullptr;

		for (i = (int)instructions.size() - 1; i >= 0; --i)
		{
			instruction = &std::get<1>(instructions[i]);

			if (instruction->operands[0].reg.value == target_register)
			{
				std::get<2>(instructions[i]) = true;
				
				if (instruction->operand_count > 1)
				{
					if (instruction->operands[1].type == 1)
						track_register_usage(instruction->operands[1].reg.value, i - 1);
					else if (instruction->operands[1].type == 2)
					{
						if (instruction->operands[1].mem.base != ZYDIS_REGISTER_RIP)
						{
							if (instruction->operands[1].mem.base != ZYDIS_REGISTER_NONE)
								track_register_usage(instruction->operands[1].mem.base, i - 1);
							if (instruction->operands[1].mem.index != ZYDIS_REGISTER_NONE)
								track_register_usage(instruction->operands[1].mem.index, i - 1);
						}
					}
					else if (instruction->operands[1].type == 4)
						continue;
					else
						printf("==ERROR1== CANNOT TRACK UNKNOWN OPERAND TYPE %d (%s)\n", instruction->operands[1].type, disassembler.format(&std::get<1>(instructions[i])).c_str());
				}
			}
		}
	}

	std::string Translator::translate_register_operand(ZydisDecodedOperand* operand)
	{
		return disassembler.get_register_name(operand->reg.value) + ";";
	}

	std::string Translator::translate_memory_operand(DWORD64& rip, ZydisDecodedInstruction* instruction, ZydisDecodedOperand* operand)
	{
		std::stringstream ss;
		DWORD64 displacement = NULL, stack_value = NULL;

		if (instruction->mnemonic == ZYDIS_MNEMONIC_MOV && operand->mem.segment == ZYDIS_REGISTER_GS)
		{
			ss << "driver.get_process_peb();";
			return ss.str();
		}

		if (instruction->mnemonic != ZYDIS_MNEMONIC_LEA)
			ss << "driver.read<DWORD64>(";
		if (operand->mem.base != ZYDIS_REGISTER_NONE)
		{
			if (operand->mem.base == ZYDIS_REGISTER_RIP)
			{
				ss << "driver.get_process_base()";
				displacement = operand->mem.disp.value + rip + instruction->length - debugger.get_process_base();
				if (displacement)
					ss << " + 0x" << std::hex << displacement;
				goto DONE_TRANSLATING;
			}
			else
			{
				if (operand->mem.base == ZYDIS_REGISTER_RBP || operand->mem.base == ZYDIS_REGISTER_RSP)
				{
					ss.str("");
					instruction->mnemonic = ZYDIS_MNEMONIC_LEA;
					operand->mem.disp.has_displacement = false;

					displacement = operand->mem.base == ZYDIS_REGISTER_RBP ? debugger.get_context().Rbp : debugger.get_context().Rsp;
					ReadProcessMemory(debugger.get_process_information()->hProcess, (LPCVOID)(displacement + operand->mem.disp.value), &stack_value, sizeof(stack_value), nullptr);
					if (stack_value > debugger.get_process_base() && stack_value < 0x7FFFFFFFFFFF)
					{
						stack_value -= debugger.get_process_base();
						ss << "driver.get_process_base() + 0x" << std::hex << stack_value;
					}
					else if (stack_value)
						ss << "0x" << std::hex << stack_value;
					else
						ss << "driver.get_process_base()";
				}
				else
					ss << disassembler.get_register_name(operand->mem.base);
			}
		}
		if (operand->mem.index != ZYDIS_REGISTER_NONE)
		{
			if (operand->mem.base != ZYDIS_REGISTER_NONE)
				ss << " + ";
			ss << disassembler.get_register_name(operand->mem.index);
		}
		if (operand->mem.scale)
			ss << " * " << (int)operand->mem.scale;
		if (operand->mem.disp.has_displacement)
			ss << " + 0x" << std::hex << operand->mem.disp.value;

DONE_TRANSLATING:
		if (instruction->mnemonic != ZYDIS_MNEMONIC_LEA)
			ss << ")";
		ss << ";";
		return ss.str();
	}

	std::string Translator::translate_immediate_operand(ZydisDecodedOperand* operand)
	{
		std::stringstream ss;

		ss << "0x" << std::hex << (operand->imm.is_signed ? operand->imm.value.s : operand->imm.value.u) << ";";

		return ss.str();
	}

	void Translator::translate_instructions(bool ignore_fix)
	{
		int i = NULL;
		DWORD64 rip = NULL;
		ZydisDecodedInstruction* instruction = nullptr;
		ZydisDecodedOperand* operand = nullptr;
		std::string line = "";

		for (i = 0; i < (int)instructions.size(); ++i)
		{
			instruction = &std::get<1>(instructions[i]);

			if ((!ignore_fix && !std::get<2>(instructions[i])) || instruction->operands[0].type != ZYDIS_OPERAND_TYPE_REGISTER)
				continue;

			rip = std::get<0>(instructions[i]);
			operand = &instruction->operands[1];
			line = disassembler.get_register_name(instruction->operands[0].reg.value) + " ";

			switch (instruction->mnemonic)
			{
			case ZYDIS_MNEMONIC_MOV:
				line += translate_mov(rip, instruction, operand);
				break;
			case ZYDIS_MNEMONIC_LEA:
				line += translate_lea(rip, instruction, operand);
				break;
			case ZYDIS_MNEMONIC_ADD:
				line += translate_add(rip, instruction, operand);
				break;
			case ZYDIS_MNEMONIC_SUB:
				line += translate_sub(rip, instruction, operand);
				break;
			case ZYDIS_MNEMONIC_IMUL:
				line += translate_imul(rip, instruction, operand);
				break;
			case ZYDIS_MNEMONIC_AND:
				line += translate_and(rip, instruction, operand);
				break;
			case ZYDIS_MNEMONIC_SHL:
				line += translate_shl(rip, instruction, operand);
				break;
			case ZYDIS_MNEMONIC_SHR:
				line += translate_shr(rip, instruction, operand);
				break;
			case ZYDIS_MNEMONIC_NOT:
				line += translate_not(rip, instruction, operand);
				break;
			case ZYDIS_MNEMONIC_ROL:
				line += translate_rol(rip, instruction, operand);
				break;
			case ZYDIS_MNEMONIC_BSWAP:
				line += translate_bswap(rip, instruction, operand);
				break;
			case ZYDIS_MNEMONIC_XOR:
				line += translate_xor(rip, instruction, operand);
				break;
			default:
				generator.write_line(disassembler.format(instruction));
				continue;
			}

			std::stringstream ss;
			ss << line << std::setw((ignore_fix ? 84 : 80) - line.length()) << " // " << disassembler.format(instruction);
			generator.write_line(ss.str());

			if (ignore_fix && !i)
			{
				generator.write_line("if (!" + disassembler.get_register_name(instruction->operands[0].reg.value) + ")");
				generator.write_line("\treturn NULL;");
			}
		}

		reset();
	}

	void Translator::translate_location(DWORD64 rip, bool is_switch)
	{
		// README: should work as long as there is no test above cryptography

		ZydisDecodedInstruction instruction = {};
		ZydisRegister encrypted = ZYDIS_REGISTER_NONE, index = ZYDIS_REGISTER_NONE;
		DWORD64 end = NULL, default_case = NULL, switch_table = NULL;
		DWORD i = NULL, offset = NULL;

		reset();

		end = disassembler.find_mnemonic(rip, ZYDIS_MNEMONIC_TEST);

		translate_peb(rip, end);

		instruction = disassembler.decode(end);
		encrypted = instruction.operands[0].reg.value;
		rip = disassembler.find_register_set(rip, encrypted);

		translate_encrypted(rip, end, encrypted);

		end += instruction.length;
		instruction = disassembler.decode(end);
		end += instruction.length;
		rip = end;
		end += instruction.operands[0].imm.value.u;
		if (instructions.size() != 1)
			std::iter_swap(instructions.begin(), instructions.begin() + 1);

		translate_instructions(true);

		printf("encrypted register -> %d\n", encrypted);
		printf("end of routine -> 0x%llX\n", end - debugger.get_process_base());
		printf("current rip -> 0x%llX\n", rip - debugger.get_process_base());

		if (is_switch)
		{
			translate_range(rip, disassembler.find_mnemonic(rip, ZYDIS_MNEMONIC_CMP));
			translate_instructions(true);

			rip = debugger.get_context().Rip;
			instruction = disassembler.decode(rip);
			index = instruction.operands[0].reg.value;
			default_case = rip + instruction.length;
			instruction = disassembler.decode(default_case);
			default_case += instruction.length;
			switch_table = disassembler.find_mnemonic(default_case, ZYDIS_MNEMONIC_MOV);
			default_case += instruction.operands[0].imm.value.u - debugger.get_process_base();
			switch_table = disassembler.decode(switch_table).operands[1].mem.disp.value;

			printf("index register -> %d\n", index);
			printf("default case -> 0x%llX\n", default_case);
			printf("jump table -> 0x%llX\n\n", switch_table);
		
			generator.write_line("switch (" + disassembler.get_register_name(index) + ")");
			generator.write_line("{");
		
			for (i = 0; i < 16; ++i)
			{
				reset();

				if (i == 15)
				{
					generator.write_line("default:");
					translate_range(debugger.get_process_base() + default_case, end);
				}
				else
				{
					generator.write_line("case " + std::to_string(i) + ":");
					ReadProcessMemory(debugger.get_process_information()->hProcess, (LPCVOID)(debugger.get_process_base() + switch_table + i * sizeof(DWORD)), &offset, sizeof(offset), nullptr);
					translate_range(debugger.get_process_base() + offset, end);
				}

				generator.set_line_prefix("\t\t");

				fix_translation(encrypted);
				translate_instructions();

				generator.write_line("return " + disassembler.get_register_name(encrypted) + ";");
				generator.set_line_prefix("\t");
			}

			generator.write_line("}");
		}
		else
		{
			translate_range(rip, end);
			fix_translation(encrypted);
			translate_instructions();

			generator.write_line("return " + disassembler.get_register_name(encrypted) + ";");
		}
	}
}

dreamware::Translator translator;
