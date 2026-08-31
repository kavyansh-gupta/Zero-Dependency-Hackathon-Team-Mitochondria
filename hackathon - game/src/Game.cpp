#include "Game.h"

#include <iostream>
#include <conio.h>
#include <cstdlib>
#include <windows.h>

Game::Game(unsigned int seed,
           const std::string& loadFile,
           const std::string& saveFile,
           bool isReplay,
           const std::vector<Command>& replayCommands)
    : state(seed),
      saveFile(saveFile),
      isReplayMode(isReplay),
      replayCommands(replayCommands),
      replayIndex(0) {

    state.SetSaveFilename(saveFile);

    // Normal command-line loading.
    // Replay mode must always start from the replay seed.
    if (!loadFile.empty() && !isReplayMode) {
        if (!state.LoadFromFile(loadFile)) {
            std::cerr << "[SYSTEM] Failed to load save file: "
                      << loadFile << "\n";
            std::cerr << "[SYSTEM] Starting a new game instead.\n";
            system("pause");
        }
    }

    // Phase 10: Record normal gameplay.
    if (!isReplayMode) {
        recordFile.open("run.rep");

        if (recordFile.is_open()) {
            recordFile << "LAST_PROCESS_REPLAY\n";
            recordFile << "VERSION 1\n";
            recordFile << "SEED " << seed << "\n";
            recordFile.flush();
        }
    }
}

std::string Game::CommandToString(Command cmd) const {
    switch (cmd) {
        case Command::UP:          return "UP";
        case Command::DOWN:        return "DOWN";
        case Command::LEFT:        return "LEFT";
        case Command::RIGHT:       return "RIGHT";
        case Command::QUIT:        return "QUIT";
        case Command::ATTACK:      return "ATTACK";
        case Command::ABILITY_1:   return "ABILITY_1";
        case Command::ABILITY_2:   return "ABILITY_2";
        case Command::ABILITY_3:   return "ABILITY_3";
        case Command::ABILITY_4:   return "ABILITY_4";
        case Command::SIGSTOP_CMD: return "SIGSTOP_CMD";
        case Command::SIGTERM_CMD: return "SIGTERM_CMD";
        case Command::SIGKILL_CMD: return "SIGKILL_CMD";
        case Command::FORK_CMD:    return "FORK_CMD";
        case Command::CLEAR_RAM:   return "CLEAR_RAM";
        case Command::SAVE_CMD:    return "SAVE_CMD";
        default:                   return "NONE";
    }
}


// ============================================================
// PHASE 11 — TITLE SCREEN
// ============================================================

bool Game::ShowTitleScreen() {
    while (true) {
        system("cls");

        std::cout << "\n";
        std::cout << "====================================================\n";
        std::cout << "\n";
        std::cout << "                 T H E   L A S T\n";
        std::cout << "                    P R O C E S S\n";
        std::cout << "\n";
        std::cout << "====================================================\n";
        std::cout << "\n";
        std::cout << "                  [ ENTER ] START\n";
        std::cout << "                  [ L ] LOAD\n";
        std::cout << "                  [ Q ] QUIT\n";
        std::cout << "\n";
        std::cout << "====================================================\n";
        std::cout << "\n";

        char key = _getch();

        if (key == 13) {
            // ENTER
            return true;
        }

        if (key == 'l' || key == 'L') {
            if (LoadFromTitle()) {
                return true;
            }
        }

        if (key == 'q' || key == 'Q') {
            return false;
        }
    }
}


// ============================================================
// PHASE 11 — LOAD FROM TITLE SCREEN
// ============================================================

bool Game::LoadFromTitle() {
    system("cls");

    std::cout << "\n";
    std::cout << "====================================================\n";
    std::cout << "                    LOAD GAME\n";
    std::cout << "====================================================\n";
    std::cout << "\n";

    std::cout << "Loading: " << saveFile << "\n\n";

    if (state.LoadFromFile(saveFile)) {
        std::cout << "[SYSTEM] SAVE LOADED SUCCESSFULLY.\n";
        std::cout << "\nStarting game...\n";

        Sleep(1000);
        return true;
    }

    std::cout << "[SYSTEM] NO VALID SAVE FILE FOUND.\n";
    std::cout << "\nPress any key to return to the title screen...";

    _getch();
    return false;
}


// ============================================================
// PHASE 11 — DEATH SCREEN
// ============================================================

void Game::ShowDeathScreen() {
    system("cls");

    std::cout << "\n";
    std::cout << "====================================================\n";
    std::cout << "\n";
    std::cout << "              P R O C E S S   T E R M I N A T E D\n";
    std::cout << "\n";
    std::cout << "====================================================\n";
    std::cout << "\n";

    std::cout << "PID:    4201\n";

    std::cout << "Floor:  "
              << state.GetFloorName()
              << " ("
              << state.GetCurrentFloor()
              << "/"
              << state.GetTotalFloors()
              << ")\n";

    std::cout << "\n";

    if (state.HasSegfaulted()) {
        std::cout << "Cause:  OUT_OF_MEMORY\n";
    } else {
        std::cout << "Cause:  PROCESS_TERMINATED\n";
    }

    std::cout << "\n";
    std::cout << "Run statistics...\n";
    std::cout << "\n";

    std::cout << "HP:     "
              << state.GetPlayerHP()
              << "\n";

    std::cout << "RAM:    "
              << state.GetPlayerUsedRAM()
              << " / "
              << state.GetPlayerMaxRAM()
              << " MB\n";

    std::cout << "\n";
    std::cout << "====================================================\n";
    std::cout << "\n";
    std::cout << "              [ ENTER ] EXIT\n";
    std::cout << "\n";

    while (true) {
        char key = _getch();

        if (key == 13 || key == 'q' || key == 'Q') {
            break;
        }
    }
}


// ============================================================
// PHASE 11 — VICTORY SCREEN
// ============================================================

void Game::ShowVictoryScreen() {
    system("cls");

    std::cout << "\n";
    std::cout << "====================================================\n";
    std::cout << "\n";
    std::cout << "                P I D   1   T E R M I N A T E D\n";
    std::cout << "\n";
    std::cout << "====================================================\n";
    std::cout << "\n";
    std::cout << "                  SYSTEM RESTORED\n";
    std::cout << "\n";
    std::cout << "                    You survived.\n";
    std::cout << "\n";

    std::cout << "Seed: "
              << state.GetSeed()
              << "\n";

    std::cout << "\n";
    std::cout << "====================================================\n";
    std::cout << "\n";
    std::cout << "                  [ ENTER ] EXIT\n";
    std::cout << "\n";

    while (true) {
        char key = _getch();

        if (key == 13 || key == 'q' || key == 'Q') {
            break;
        }
    }
}


// ============================================================
// GAME LOOP
// ============================================================

void Game::Run() {

    // --------------------------------------------------------
    // Replay mode skips title screen.
    // --------------------------------------------------------
    if (!isReplayMode) {
        if (!ShowTitleScreen()) {
            system("cls");

            std::cout << "\n";
            std::cout << "THE LAST PROCESS shutting down...\n";
            Sleep(500);

            return;
        }
    }

    // --------------------------------------------------------
    // Initial render
    // --------------------------------------------------------
    renderer.Render(state);

    // --------------------------------------------------------
    // Main gameplay / replay loop
    // --------------------------------------------------------
    while (state.IsRunning()) {

        Command cmd = Command::NONE;

        // ----------------------------------------------------
        // Phase 10: Replay
        // ----------------------------------------------------
        if (isReplayMode) {

            if (replayIndex < replayCommands.size()) {
                cmd = replayCommands[replayIndex++];
            } else {
                std::cout << "\n";
                std::cout << "[SYSTEM] REPLAY COMPLETE.\n";

                Sleep(800);
                break;
            }

        }
        // ----------------------------------------------------
        // Normal gameplay
        // ----------------------------------------------------
        else {
            cmd = input.GetCommand();
        }

        // ----------------------------------------------------
        // Process command
        // ----------------------------------------------------
        if (cmd != Command::NONE) {

            // Record normal gameplay commands.
            if (!isReplayMode && recordFile.is_open()) {
                recordFile << CommandToString(cmd) << "\n";
                recordFile.flush();
            }

            state.Update(cmd);

            // Render while game is still active.
            if (state.IsRunning()) {
                renderer.Render(state);
            }
        }

        // Replay should be slower so the judge can actually watch it.
        if (isReplayMode) {
            Sleep(100);
        } else {
            Sleep(16);
        }
    }

    // --------------------------------------------------------
    // Phase 11 end-state screens
    // --------------------------------------------------------

    if (state.IsGameWon()) {
        ShowVictoryScreen();
    }
    else if (!state.IsPlayerAlive() || state.HasSegfaulted()) {
        ShowDeathScreen();
    }
}