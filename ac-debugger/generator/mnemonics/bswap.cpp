#include "../translator.hpp"
#include "../../disassembler.hpp"

std::string dreamware::Translator::translate_bswap(DWORD64& rip, ZydisDecodedInstruction* instruction, ZydisDecodedOperand* operand)
{
	std::stringstream ss;

	ss << "= _byteswap_uint64(" << disassembler.get_register_name(instruction->operands[0].reg.value) << ");";

	return ss.str();
}
