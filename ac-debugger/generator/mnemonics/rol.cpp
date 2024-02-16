#include "../translator.hpp"
#include "../../disassembler.hpp"

std::string dreamware::Translator::translate_rol(DWORD64& rip, ZydisDecodedInstruction* instruction, ZydisDecodedOperand* operand)
{
	std::stringstream ss;
	std::string return_value = "";

	ss << "= _rotl64(" << disassembler.get_register_name(instruction->operands[0].reg.value) << ", ";

	switch (operand->type)
	{
	case ZYDIS_OPERAND_TYPE_IMMEDIATE:
		ss << translate_immediate_operand(operand);
		break;
	default:
		ss << __FUNCTION__ << ": operand type " << operand->type << " unhandled";
		break;
	}

	return_value = ss.str();
	return_value.pop_back();
	return return_value + ");";
}
