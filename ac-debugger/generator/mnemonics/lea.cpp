#include "../translator.hpp"


std::string dreamware::Translator::translate_lea(DWORD64& rip, ZydisDecodedInstruction* instruction, ZydisDecodedOperand* operand)
{
	std::stringstream ss;

	ss << "= ";

	switch (operand->type)
	{
	case ZYDIS_OPERAND_TYPE_MEMORY:
		ss << translate_memory_operand(rip, instruction, operand);
		break;
	default:
		ss << __FUNCTION__ << ": operand type " << operand->type << " unhandled";
		break;
	}

	return ss.str();
}
