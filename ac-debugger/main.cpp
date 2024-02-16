#include <Windows.h>
#include <iostream>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include "debugger.hpp"
#include "generator/generator.hpp"

// USAGE: unpacked_game_path 
int main(int argc, char *argv[])
{
    std::string game = "";
    int exit_code = 0;

    SetConsoleTitle(L"dreamware®: debugger");

    if (argc != 2)
    {
NO_GAME_SPECIFIED:
        std::cerr << "no game specified" << std::endl;
        return 1;
    }

    game = std::string(PathFindFileNameA(argv[1]));
    if (game.find(".exe") == std::string::npos)
        goto NO_GAME_SPECIFIED;

    if (game.find("ModernWarfare") == std::string::npos && game.find("cod") == std::string::npos &&
        game.find("BlackOpsColdWar") == std::string::npos)
    {
        std::cerr << "game is not implemented yet" << std::endl;
        return 2;
    }

    if (!debugger.attach(argv[1]))
    {
        debugger.detach();
        std::cerr << "could not attach debugger to game" << std::endl;
        return 3;
    }

    exit_code = generator.generate_sdk(game);
    debugger.detach();

    return exit_code;
}
