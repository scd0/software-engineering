#include "../translator.hpp"
#include "../../disassembler.hpp"

std::string dreamware::Translator::translate_not(DWORD64& rip, ZydisDecodedInstruction* instruction, ZydisDecodedOperand* operand)
{
	std::stringstream ss;

	ss << "= ~" << disassembler.get_register_name(instruction->operands[0].reg.value) << ";";

	return ss.str();
}
