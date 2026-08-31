#ifndef GAME_H
#define GAME_H

#include <string>
#include <vector>
#include <fstream>

#include "GameState.h"
#include "Input.h"
#include "Renderer.h"

class Game {
public:
    Game(unsigned int seed,
         const std::string& loadFile,
         const std::string& saveFile,
         bool isReplay,
         const std::vector<Command>& replayCommands);

    void Run();

private:
    GameState state;
    Input input;
    Renderer renderer;

    std::string saveFile;

    // Phase 10: Replay
    bool isReplayMode;
    std::vector<Command> replayCommands;
    size_t replayIndex;

    std::ofstream recordFile;

    // Phase 11: Screens
    bool ShowTitleScreen();
    bool LoadFromTitle();

    void ShowDeathScreen();
    void ShowVictoryScreen();

    std::string CommandToString(Command cmd) const;
};

#endif