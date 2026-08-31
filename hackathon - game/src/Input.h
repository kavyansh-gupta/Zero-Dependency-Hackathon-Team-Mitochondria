#pragma once

enum class Command { 
    NONE, UP, DOWN, LEFT, RIGHT, QUIT, ATTACK,
    ABILITY_1, ABILITY_2, ABILITY_3, ABILITY_4,
    SIGSTOP_CMD, SIGTERM_CMD, SIGKILL_CMD, FORK_CMD, CLEAR_RAM,
    SAVE_CMD
};

class Input {
public:
    Command GetCommand();
};