#include "../translator.hpp"


std::string dreamware::Translator::translate_and(DWORD64& rip, ZydisDecodedInstruction* instruction, ZydisDecodedOperand* operand)
{
	std::stringstream ss;

	ss << "&= ";

	switch (operand->type)
	{
	case ZYDIS_OPERAND_TYPE_REGISTER:
		ss << translate_register_operand(operand);
		break;
	case ZYDIS_OPERAND_TYPE_MEMORY:
		ss << translate_memory_operand(rip, instruction, operand);
		break;
	case ZYDIS_OPERAND_TYPE_IMMEDIATE:
		if (operand->imm.is_signed && operand->imm.value.s == 0xffffffffc0000000)
			ss.str("= NULL;");
		else
			ss << translate_immediate_operand(operand);
		break;
	default:
		ss << __FUNCTION__ << ": operand type " << operand->type << " unhandled";
		break;
	}

	return ss.str();
}
