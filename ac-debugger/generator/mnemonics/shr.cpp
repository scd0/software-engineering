#include "../translator.hpp"


std::string dreamware::Translator::translate_shr(DWORD64& rip, ZydisDecodedInstruction* instruction, ZydisDecodedOperand* operand)
{
	std::stringstream ss;

	ss << ">>= ";

	switch (operand->type)
	{
	case ZYDIS_OPERAND_TYPE_IMMEDIATE:
		ss << translate_immediate_operand(operand);
		break;
	default:
		ss << __FUNCTION__ << ": operand type " << operand->type << " unhandled";
		break;
	}

	return ss.str();
}
