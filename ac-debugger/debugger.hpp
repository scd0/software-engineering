#ifndef C66B939A_32B9_4F2F_838C_F8E6D829E878
#define C66B939A_32B9_4F2F_838C_F8E6D829E878

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <Zydis/Zydis.h>

namespace dreamware
{
	class Debugger
	{
	public:
		Debugger();
		~Debugger();

		CONTEXT get_context() const;
		void set_context(LPCONTEXT context);
		void set_trap_flag();
		LPPROCESS_INFORMATION get_process_information();
		void set_register(ZydisRegister target_register, DWORD64 value);
		DWORD64 get_process_base() const;
		void reset_stack();

		bool attach(LPCSTR file);
		void detach();
		bool dispatch_debug_event(LPDEBUG_EVENT debug_event);
		void wait_debug_event();
		void single_step();

		BYTE* text;
		BYTE* rdata;
		DWORD text_size, rdata_size;
		DWORD text_address, rdata_address;

	private:
		PROCESS_INFORMATION process_information;
		DWORD64 process_base, stack;
		DWORD exception;


		bool map_sections();
	};
}

extern dreamware::Debugger debugger;

#endif // C66B939A_32B9_4F2F_838C_F8E6D829E878
