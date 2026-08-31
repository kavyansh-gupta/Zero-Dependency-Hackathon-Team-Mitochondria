#include "Input.h"
#include <conio.h>
#include <cctype>

Command Input::GetCommand() {
    if (_kbhit()) {
        char ch = std::tolower(_getch());
        switch (ch) {
            case 'w': return Command::UP;
            case 's': return Command::DOWN;
            case 'a': return Command::LEFT;
            case 'd': return Command::RIGHT;
            case 'q': return Command::QUIT;
            case 'f': return Command::ATTACK;
            case '1': return Command::ABILITY_1;
            case '2': return Command::ABILITY_2;
            case '3': return Command::ABILITY_3;
            case '4': return Command::ABILITY_4;
            case '5': return Command::SIGSTOP_CMD;
            case '6': return Command::SIGTERM_CMD;
            case '7': return Command::SIGKILL_CMD;
            case '8': return Command::FORK_CMD;
            case 'r': return Command::CLEAR_RAM;
            case 'p': return Command::SAVE_CMD;
            default:  return Command::NONE;
        }
    }
    return Command::NONE;
}