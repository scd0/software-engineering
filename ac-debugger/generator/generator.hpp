#ifndef A918D9CE_F184_4FCA_B46E_A8934B3B1F30
#define A918D9CE_F184_4FCA_B46E_A8934B3B1F30

#include <Windows.h>
#include <iostream>
#include <vector>

namespace dreamware
{
	class Generator
	{
	public:
		Generator();
		~Generator();

		void set_line_prefix(std::string prefix);

		void write_line(std::string line, bool is_source = true);
		int generate_sdk(const std::string& game);

	private:
		std::vector<std::string> header, source;
		size_t header_index, source_index;
		std::string line_prefix;

		void generate_function(std::string prototype, DWORD64 location, bool is_switch = true);
		void generate_sdk_files(const std::string& game);
		int save_sdk_files();
	};
}

extern dreamware::Generator generator;

#endif // A918D9CE_F184_4FCA_B46E_A8934B3B1F30
