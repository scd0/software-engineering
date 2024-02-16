#ifndef E1142739_7ACE_48C2_913B_26E4232FA00A
#define E1142739_7ACE_48C2_913B_26E4232FA00A

#include <Windows.h>
#include <vector>

namespace dreamware
{
	typedef struct _INFINITY_WARD_INFO
	{
		DWORD64 cg, characters;

		DWORD64 character_size, character_valid, character_team;
		DWORD valid_size, team_size;
	} INFINITY_WARD_INFO, * PINFINITY_WARD_INFO;

	class Finder
	{
	public:
		Finder();
		~Finder();

		DWORD64 find_pattern(bool in_text_section, std::vector<BYTE> pattern);

		void find_mw1_info(PINFINITY_WARD_INFO info);

	private:
		std::vector<BYTE> make_pattern(DWORD64 value);
	};
}

extern dreamware::Finder finder;

#endif // E1142739_7ACE_48C2_913B_26E4232FA00A
