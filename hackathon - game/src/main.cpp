#include "Game.h"

#include <iostream>
#include <string>
#include <fstream>
#include <vector>

int main(int argc, char** argv) {

    unsigned int seed = 12345;

    std::string loadFile = "";
    std::string saveFile = "save.dat";

    std::string replayFile = "";
    bool isReplay = false;

    // --------------------------------------------------------
    // Parse command-line arguments
    // --------------------------------------------------------

    for (int i = 1; i < argc; ++i) {

        std::string arg = argv[i];

        if (arg == "--load" && i + 1 < argc) {
            loadFile = argv[++i];
        }

        else if (arg == "--save" && i + 1 < argc) {
            saveFile = argv[++i];
        }

        else if (arg == "--seed" && i + 1 < argc) {
            try {
                seed = static_cast<unsigned int>(
                    std::stoul(argv[++i])
                );
            }
            catch (...) {
                std::cerr << "[ERROR] Invalid seed.\n";
                return 1;
            }
        }

        else if (arg == "--replay" && i + 1 < argc) {
            replayFile = argv[++i];
            isReplay = true;
        }
    }


    // --------------------------------------------------------
    // Phase 10: Load replay file
    // --------------------------------------------------------

    std::vector<Command> replayCommands;

    if (isReplay) {

        std::ifstream in(replayFile);

        if (!in.is_open()) {
            std::cerr
                << "[ERROR] Could not open replay file: "
                << replayFile
                << "\n";

            return 1;
        }

        std::string token;

        // Header
        if (!(in >> token) ||
            token != "LAST_PROCESS_REPLAY") {

            std::cerr
                << "[ERROR] Invalid replay format.\n";

            return 1;
        }

        // Version
        int version = 0;

        if (!(in >> token >> version) ||
            token != "VERSION" ||
            version != 1) {

            std::cerr
                << "[ERROR] Unsupported replay version.\n";

            return 1;
        }

        // Seed
        if (!(in >> token >> seed) ||
            token != "SEED") {

            std::cerr
                << "[ERROR] Missing replay seed.\n";

            return 1;
        }

        // Commands
        while (in >> token) {

            Command cmd = Command::NONE;

            if (token == "UP")
                cmd = Command::UP;

            else if (token == "DOWN")
                cmd = Command::DOWN;

            else if (token == "LEFT")
                cmd = Command::LEFT;

            else if (token == "RIGHT")
                cmd = Command::RIGHT;

            else if (token == "QUIT")
                cmd = Command::QUIT;

            else if (token == "ATTACK")
                cmd = Command::ATTACK;

            else if (token == "ABILITY_1")
                cmd = Command::ABILITY_1;

            else if (token == "ABILITY_2")
                cmd = Command::ABILITY_2;

            else if (token == "ABILITY_3")
                cmd = Command::ABILITY_3;

            else if (token == "ABILITY_4")
                cmd = Command::ABILITY_4;

            else if (token == "SIGSTOP_CMD")
                cmd = Command::SIGSTOP_CMD;

            else if (token == "SIGTERM_CMD")
                cmd = Command::SIGTERM_CMD;

            else if (token == "SIGKILL_CMD")
                cmd = Command::SIGKILL_CMD;

            else if (token == "FORK_CMD")
                cmd = Command::FORK_CMD;

            else if (token == "CLEAR_RAM")
                cmd = Command::CLEAR_RAM;

            else if (token == "SAVE_CMD")
                cmd = Command::SAVE_CMD;

            if (cmd != Command::NONE) {
                replayCommands.push_back(cmd);
            }
            else {
                std::cerr
                    << "[WARNING] Ignoring unknown replay command: "
                    << token
                    << "\n";
            }
        }
    }


    // --------------------------------------------------------
    // Start game
    // --------------------------------------------------------

    Game game(
        seed,
        loadFile,
        saveFile,
        isReplay,
        replayCommands
    );

    game.Run();

    return 0;
}