#include "debugger.hpp"
#include "disassembler.hpp"
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

namespace dreamware
{
	Debugger::Debugger()
	{
		process_information = {};
		process_base = NULL;
		exception = STILL_ACTIVE;
		stack = NULL;
		text = nullptr;
		rdata = nullptr;
		text_size = NULL;
		rdata_size = NULL;
		rdata_address = NULL;
		text_address = NULL;
	}

	Debugger::~Debugger()
	{
	}

	CONTEXT Debugger::get_context() const
	{
		CONTEXT context = {};

		context.ContextFlags = CONTEXT_ALL;
		GetThreadContext(process_information.hThread, &context);
		return context;
	}

	void Debugger::set_context(LPCONTEXT context)
	{
		SetThreadContext(process_information.hThread, context);
	}

	void Debugger::set_trap_flag()
	{
		CONTEXT context = get_context();

		context.EFlags |= 0x100;
		set_context(&context);
	}

	LPPROCESS_INFORMATION Debugger::get_process_information()
	{
		return &process_information;
	}

	void Debugger::set_register(ZydisRegister target_register, DWORD64 value)
	{
		CONTEXT context = get_context();
		int offset = target_register - ZYDIS_REGISTER_RAX;

		if (target_register == ZYDIS_REGISTER_RIP)
		{
			if (value < process_base)
				value += process_base;
			context.Rip = value;
		}
		else
			*(&context.Rax + offset) = value;
		set_context(&context);
	}

	DWORD64 Debugger::get_process_base() const
	{
		return process_base;
	}

	void Debugger::reset_stack()
	{
		BYTE buffer[4096] = {};

		ZeroMemory(buffer, sizeof(buffer));
		WriteProcessMemory(process_information.hProcess, (LPVOID)stack, buffer, sizeof(buffer), nullptr);
	}

	bool Debugger::attach(LPCSTR file)
	{
		STARTUPINFOA startup_info = {};

		startup_info.cb = sizeof(startup_info);
		if (!CreateProcessA(file, nullptr, nullptr, nullptr, FALSE, DEBUG_ONLY_THIS_PROCESS, nullptr, nullptr, &startup_info, &process_information))
			return false;

		wait_debug_event();

		stack = (DWORD64)VirtualAllocEx(process_information.hProcess, nullptr, 4096, MEM_COMMIT, PAGE_READWRITE);
		set_register(ZYDIS_REGISTER_RBP, stack + 2048);
		set_register(ZYDIS_REGISTER_RSP, stack + 2048);

		return map_sections();
	}

	void Debugger::detach()
	{
		if (stack)
			VirtualFreeEx(process_information.hProcess, (LPVOID)stack, NULL, MEM_RELEASE);
		if (text)
			free(text);
		if (rdata)
			free(rdata);

		if (stack)
		{
			TerminateProcess(process_information.hProcess, 0);
			wait_debug_event();

			CloseHandle(process_information.hProcess);
			CloseHandle(process_information.hThread);
		}
	}

	bool Debugger::dispatch_debug_event(LPDEBUG_EVENT debug_event)
	{
		char buffer[MAX_PATH] = {};

		switch (debug_event->dwDebugEventCode)
		{
		case CREATE_PROCESS_DEBUG_EVENT:
			printf("CREATE_PROCESS_DEBUG_EVENT");
			process_base = (DWORD64)debug_event->u.CreateProcessInfo.lpBaseOfImage;
			printf("\t%llXh\n", process_base);
			break;
		case EXCEPTION_DEBUG_EVENT:
			exception = debug_event->u.Exception.ExceptionRecord.ExceptionCode;
			if (exception != EXCEPTION_SINGLE_STEP)
				printf("EXCEPTION_DEBUG_EVENT\t\t%Xh\n", exception);
			return false;
		case EXIT_PROCESS_DEBUG_EVENT:
			ContinueDebugEvent(process_information.dwProcessId, process_information.dwThreadId, DBG_CONTINUE);
			return false;
		default:
			break;
		}

		return true;
	}

	void Debugger::wait_debug_event()
	{
		DEBUG_EVENT debug_event = {};

		if (exception != STILL_ACTIVE)
		{
			ContinueDebugEvent(process_information.dwProcessId, process_information.dwThreadId, DBG_CONTINUE);
			exception = STILL_ACTIVE;
		}

		while (WaitForDebugEvent(&debug_event, INFINITE))
		{
			process_information.dwProcessId = debug_event.dwProcessId;
			process_information.dwThreadId = debug_event.dwThreadId;
			
			if (dispatch_debug_event(&debug_event))
				ContinueDebugEvent(process_information.dwProcessId, process_information.dwThreadId, DBG_CONTINUE);
			else
				break;
		}
	}

	void Debugger::single_step()
	{
		DWORD64 rip = NULL;
		ZydisDecodedInstruction instruction = {};

		set_trap_flag();
		wait_debug_event();

		if (exception == EXCEPTION_ACCESS_VIOLATION)
		{
			rip = get_context().Rip;
			instruction = disassembler.decode(rip);
			set_register(ZYDIS_REGISTER_RIP, rip + instruction.length);
		}
	}

	bool Debugger::map_sections()
	{
		BYTE header[0x1000] = {};
		PIMAGE_SECTION_HEADER section_header = {};
		DWORD section_size = sizeof(IMAGE_SECTION_HEADER);
		PIMAGE_DOS_HEADER dos_header = nullptr;
		PIMAGE_NT_HEADERS nt_headers = nullptr;
		DWORD64 section_location = NULL;
		int i = NULL;

		ReadProcessMemory(process_information.hProcess, (LPCVOID)process_base, header, sizeof(header), nullptr);

		dos_header = (PIMAGE_DOS_HEADER)header;
		nt_headers = (PIMAGE_NT_HEADERS)((DWORD64)header + dos_header->e_lfanew);
		section_location = (DWORD64)nt_headers + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + nt_headers->FileHeader.SizeOfOptionalHeader;

		for (i = 0; i < nt_headers->FileHeader.NumberOfSections; i++)
		{
			section_header = (PIMAGE_SECTION_HEADER)section_location;

			if (!text_address && !strcmp((const char*)section_header->Name, ".text"))
			{
				/*printf("\t%s\n", section_header->Name);
				printf("\t\t0x%x\t\tVirtual Size\n", section_header->Misc.VirtualSize);
				printf("\t\t0x%x\t\tVirtual Address\n", section_header->VirtualAddress);*/

				text_address = section_header->VirtualAddress;
				text_size = section_header->Misc.VirtualSize;
			}
			else if (!rdata_address && !strcmp((const char*)section_header->Name, ".rdata"))
			{
				/*printf("\t%s\n", section_header->Name);
				printf("\t\t0x%x\t\tVirtual Size\n", section_header->Misc.VirtualSize);
				printf("\t\t0x%x\t\tVirtual Address\n", section_header->VirtualAddress);*/

				rdata_address = section_header->VirtualAddress;
				rdata_size = section_header->Misc.VirtualSize;
			}

			section_location += section_size;
		}

		text = (BYTE*)malloc(text_size);
		rdata = (BYTE*)malloc(rdata_size);

		if (!text || !rdata)
			return false;

		ReadProcessMemory(process_information.hProcess, (LPCVOID)(process_base + text_address), text, text_size, nullptr);
		ReadProcessMemory(process_information.hProcess, (LPCVOID)(process_base + rdata_address), rdata, rdata_size, nullptr);

		printf(".text mapped at\t%p\n", text);
		printf(".rdata mapped at\t%p\n", rdata);

		return true;
	}
}

dreamware::Debugger debugger;
