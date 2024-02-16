#include "generator.hpp"
#include <fstream>
#include <boost/algorithm/string/join.hpp>
#include "translator.hpp"
#include "../debugger.hpp"
#include "finder.hpp"

// mw2019 cg_entities
/*prototype = "DWORD64 decrypt_cg()";
write_line(prototype + ";", false);
write_line(prototype);
write_line("{");
set_line_prefix("\t");
write_line("CONTEXT c = {};\n");
translator.translate_switch(debugger.get_process_base() + 0x64ED204);
set_line_prefix("");
write_line("}\n");*/

// cgArray
/*prototype = "DWORD64 decrypt_cg()";
write_line(prototype + ";", false);
write_line(prototype);
write_line("{");
set_line_prefix("\t");
write_line("CONTEXT c = {};\n");
translator.translate_switch(debugger.get_process_base() + 0x8D93582);
set_line_prefix("");
write_line("}\n");*/

// cg_entitiesArray
/*prototype = "DWORD64 decrypt_cg_entities()";
write_line(prototype + ";", false);
write_line(prototype);
write_line("{");
set_line_prefix("\t");
write_line("CONTEXT c = {};\n");
translator.translate_switch(debugger.get_process_base() + 0x8D9820B);
set_line_prefix("");
write_line("}\n");*/

namespace dreamware
{
	Generator::Generator()
	{
		header_index = 0;
		source_index = 0;
		line_prefix = "";
	}

	Generator::~Generator()
	{
	}

	void Generator::set_line_prefix(std::string prefix)
	{
		line_prefix = prefix;
	}

	void Generator::generate_function(std::string prototype, DWORD64 location, bool is_switch)
	{
		write_line(prototype + ";", false);
		write_line(prototype);
		write_line("{");
		set_line_prefix("\t");
		write_line("CONTEXT c = {};\n");

		translator.translate_location(location, is_switch);

		set_line_prefix("");
		write_line("}\n");
	}

	void Generator::generate_sdk_files(const std::string& game)
	{
		std::string timestamp = "INSERT_TIMESTAMP_HERE";
		std::string version = "INSERT_GAME_VERSION_HERE";

		header.push_back("#ifndef D7EDEDCE_AAC0_4805_9E86_D861A7CED6D6\n");
		header.push_back("#define D7EDEDCE_AAC0_4805_9E86_D861A7CED6D6\n");
		header.push_back("\n");
		header.push_back("//\t\xE2\x96\x88\xE2\x96\x80\xE2\x96\x84\xE2\x80\x83\xE2\x96\x88\xE2\x96\x80\xE2\x96\x88\xE2\x80\x83\xE2\x96\x88\xE2\x96\x80\xE2\x96\x80\xE2\x80\x83\xE2\x96\x84\xE2\x96\x80\xE2\x96\x88\xE2\x80\x83\xE2\x96\x88\xE2\x96\x80\xE2\x96\x84\xE2\x96\x80\xE2\x96\x88\xE2\x80\x83\xE2\x96\x88\xE2\x96\x91\xE2\x96\x88\xE2\x96\x91\xE2\x96\x88\xE2\x80\x83\xE2\x96\x84\xE2\x96\x80\xE2\x96\x88\xE2\x80\x83\xE2\x96\x88\xE2\x96\x80\xE2\x96\x88\xE2\x80\x83\xE2\x96\x88\xE2\x96\x80\xE2\x96\x80\n");
		header.push_back("//\t\xE2\x96\x88\xE2\x96\x84\xE2\x96\x80\xE2\x80\x83\xE2\x96\x88\xE2\x96\x80\xE2\x96\x84\xE2\x80\x83\xE2\x96\x88\xE2\x96\x88\xE2\x96\x84\xE2\x80\x83\xE2\x96\x88\xE2\x96\x80\xE2\x96\x88\xE2\x80\x83\xE2\x96\x88\xE2\x96\x91\xE2\x96\x80\xE2\x96\x91\xE2\x96\x88\xE2\x80\x83\xE2\x96\x80\xE2\x96\x84\xE2\x96\x80\xE2\x96\x84\xE2\x96\x80\xE2\x80\x83\xE2\x96\x88\xE2\x96\x80\xE2\x96\x88\xE2\x80\x83\xE2\x96\x88\xE2\x96\x80\xE2\x96\x84\xE2\x80\x83\xE2\x96\x88\xE2\x96\x88\xE2\x96\x84\n");
		header.push_back("\n");
		header.push_back("//\t" + timestamp + "\n");
		header.push_back("#define GAME_NAME \"" + game + "\"\n");
		header.push_back("//\t" + version + "\n");
		header.push_back("\n");
		header.push_back("#include <Windows.h>\n");
		header.push_back("\n");
		header.push_back("namespace dreamware\n");
		header.push_back("{\n");
		header_index = header.size();
		header.push_back("}\n");
		header.push_back("\n");
		header.push_back("#endif // D7EDEDCE_AAC0_4805_9E86_D861A7CED6D6\n");

		source.push_back("#include \"sdk.hpp\"\n");
		source.push_back("\n");
		source.push_back("namespace dreamware\n");
		source.push_back("{\n");
		source_index = source.size();
		source.push_back("}\n");
	}

	int Generator::save_sdk_files()
	{
		std::ofstream hpp("sdk.hpp", std::ios::trunc);
		std::ofstream cpp("sdk.cpp", std::ios::trunc);
		std::string content = "";

		if (!hpp.is_open())
		{
COULD_NOT_SAVE_SDK_FILES:
			std::cerr << "could not save sdk files" << std::endl;
			return 4;
		}

		if (!cpp.is_open())
		{
			hpp.close();
			goto COULD_NOT_SAVE_SDK_FILES;
		}

		content = boost::algorithm::join(header, "");
		hpp.write(content.c_str(), content.size());
		content = boost::algorithm::join(source, "");
		cpp.write(content.c_str(), content.size());
		hpp.close();
		cpp.close();

		return 0;
	}

	void Generator::write_line(std::string line, bool is_source)
	{
		line = "\t" + line_prefix + line + "\n";

		if (is_source)
		{
			source.insert(source.begin() + source_index, line);
			++source_index;
		}
		else
		{
			header.insert(header.begin() + header_index, line);
			++header_index;
		}
	}

	int Generator::generate_sdk(const std::string& game)
	{
		dreamware::INFINITY_WARD_INFO iw_info = {};

		generate_sdk_files(game);

		if (game.find("ModernWarfare") != std::string::npos)
		{
			finder.find_mw1_info(&iw_info);

			generate_function("DWORD64 decrypt_cg()", iw_info.cg, false);
			generate_function("DWORD64 decrypt_characters(DWORD64 cg)", iw_info.characters);
		}

		return save_sdk_files();
	}
}

dreamware::Generator generator;
