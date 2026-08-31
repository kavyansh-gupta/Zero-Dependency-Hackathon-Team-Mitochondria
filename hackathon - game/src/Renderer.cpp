#include "Renderer.h"

#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <windows.h>
#include <cmath>

namespace {

    // ============================================================
    // CONSOLE COLORS
    // ============================================================

    const WORD COLOR_DEFAULT  = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    const WORD COLOR_WHITE    = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    const WORD COLOR_GRAY     = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    const WORD COLOR_RED      = FOREGROUND_RED | FOREGROUND_INTENSITY;
    const WORD COLOR_DARK_RED = FOREGROUND_RED;
    const WORD COLOR_GREEN    = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    const WORD COLOR_BLUE     = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    const WORD COLOR_CYAN     = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    const WORD COLOR_YELLOW   = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    const WORD COLOR_MAGENTA  = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;

    void SetColor(HANDLE console, WORD color) {
        SetConsoleTextAttribute(console, color);
    }

    void ResetColor(HANDLE console) {
        SetColor(console, COLOR_DEFAULT);
    }

    // Print one map tile using its appropriate color.
    void PrintTile(HANDLE console, char tile) {

        switch (tile) {

            // Player
            case '@':
                SetColor(console, COLOR_CYAN);
                break;

            // Cache
            case '$':
                SetColor(console, COLOR_YELLOW);
                break;

            // Clone
            case 'C':
                SetColor(console, COLOR_MAGENTA);
                break;

            // Active fork
            case 'F':
                SetColor(console, COLOR_GREEN);
                break;

            // Firewall
            case '+':
                SetColor(console, COLOR_BLUE);
                break;

            // Worker
            case 'W':
                SetColor(console, COLOR_RED);
                break;

            // Daemon
            case 'D':
                SetColor(console, COLOR_RED);
                break;

            // Zombie
            case 'Z':
            case '%':
                SetColor(console, COLOR_DARK_RED);
                break;

            // Deadlock A/B
            case 'A':
            case 'B':
                SetColor(console, COLOR_MAGENTA);
                break;

            // Fork bomb
            case 'f':
                SetColor(console, COLOR_YELLOW);
                break;

            // Walls
            case '#':
                SetColor(console, COLOR_GRAY);
                break;

            // Empty floor
            case '.':
                SetColor(console, COLOR_GRAY);
                break;

            default:
                SetColor(console, COLOR_DEFAULT);
                break;
        }

        std::cout << tile;
        ResetColor(console);
    }

    // Color individual panel lines.
    void PrintPanelLine(HANDLE console, const std::string& line) {

        // Section headers
        if (line == "COMMANDS" ||
            line == "ABILITIES" ||
            line == "SYSTEM SIGNALS" ||
            line == "PLAYER" ||
            line == "SYSTEM CACHE") {

            SetColor(console, COLOR_CYAN);
            std::cout << line;
            ResetColor(console);
            std::cout << "\n";
            return;
        }

        // Separator
        if (line == "------------------------------") {
            SetColor(console, COLOR_GRAY);
            std::cout << line;
            ResetColor(console);
            std::cout << "\n";
            return;
        }

        // Ability commands
        if (line.find("1         ") == 0 ||
            line.find("2         ") == 0 ||
            line.find("3         ") == 0 ||
            line.find("4         ") == 0 ||
            line.find("8         ") == 0) {

            SetColor(console, COLOR_GREEN);

            if (line.length() > 1) {
                std::cout << line[0];
                SetColor(console, COLOR_WHITE);
                std::cout << line.substr(1);
            }

            ResetColor(console);
            std::cout << "\n";
            return;
        }

        // System signals
        if (line.find("5         ") == 0 ||
            line.find("6         ") == 0 ||
            line.find("7         ") == 0) {

            SetColor(console, COLOR_RED);

            if (line.length() > 1) {
                std::cout << line[0];
                SetColor(console, COLOR_WHITE);
                std::cout << line.substr(1);
            }

            ResetColor(console);
            std::cout << "\n";
            return;
        }

        // Basic commands
        if (line.find("W / S") == 0 ||
            line.find("A / D") == 0 ||
            line.find("F         ") == 0 ||
            line.find("P         ") == 0 ||
            line.find("R         ") == 0 ||
            line.find("Q         ") == 0) {

            SetColor(console, COLOR_CYAN);
            std::cout << line;
            ResetColor(console);
            std::cout << "\n";
            return;
        }

        // Cache selections
        if (line.find("1  KERNEL") == 0 ||
            line.find("2  MEMORY") == 0 ||
            line.find("3  ENERGY") == 0 ||
            line.find("4  HEALTH") == 0) {

            SetColor(console, COLOR_YELLOW);
            std::cout << line;
            ResetColor(console);
            std::cout << "\n";
            return;
        }

        // Cache descriptions
        if (line.find("   +") == 0 ||
            line.find("   -") == 0) {

            SetColor(console, COLOR_WHITE);
            std::cout << line;
            ResetColor(console);
            std::cout << "\n";
            return;
        }

        // Player information
        if (line.find("HP          ") == 0) {

            SetColor(console, COLOR_RED);
            std::cout << line;
            ResetColor(console);
            std::cout << "\n";
            return;
        }

        if (line.find("RAM         ") == 0) {

            SetColor(console, COLOR_GREEN);
            std::cout << line;
            ResetColor(console);
            std::cout << "\n";
            return;
        }

        if (line.find("Cooldown    ") == 0) {

            SetColor(console, COLOR_YELLOW);
            std::cout << line;
            ResetColor(console);
            std::cout << "\n";
            return;
        }

        // Default
        SetColor(console, COLOR_DEFAULT);
        std::cout << line;
        ResetColor(console);
        std::cout << "\n";
    }
}

void Renderer::Render(const GameState& state) const {

    // ============================================================
    // LOCK RENDERING TO THE TOP OF THE CONSOLE
    // ============================================================

    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO csbi;

    int consoleWidth = 120;
    int consoleHeight = 40;

    // Safe defaults in case the console API fails.
    csbi.srWindow.Left = 0;
    csbi.srWindow.Top = 0;
    csbi.srWindow.Right = consoleWidth - 1;
    csbi.srWindow.Bottom = consoleHeight - 1;
    csbi.wAttributes = COLOR_DEFAULT;

    if (GetConsoleScreenBufferInfo(console, &csbi)) {

        consoleWidth =
            csbi.srWindow.Right -
            csbi.srWindow.Left +
            1;

        consoleHeight =
            csbi.srWindow.Bottom -
            csbi.srWindow.Top +
            1;
    }

    // Move cursor to top-left of visible console.
    COORD home;
    home.X = csbi.srWindow.Left;
    home.Y = csbi.srWindow.Top;

    SetConsoleCursorPosition(console, home);

    // Clear visible console area.
    DWORD written;

    COORD clearStart;
    clearStart.X = csbi.srWindow.Left;
    clearStart.Y = csbi.srWindow.Top;

    DWORD cells =
        static_cast<DWORD>(consoleWidth) *
        static_cast<DWORD>(consoleHeight);

    FillConsoleOutputCharacterA(
        console,
        ' ',
        cells,
        clearStart,
        &written
    );

    FillConsoleOutputAttribute(
        console,
        csbi.wAttributes,
        cells,
        clearStart,
        &written
    );

    SetConsoleCursorPosition(console, home);

    ResetColor(console);

    // ============================================================
    // GAME DATA
    // ============================================================

    const auto& map = state.GetMap();

    int px = state.GetPlayerX();
    int py = state.GetPlayerY();

    const auto& enemies = state.GetEnemies();

    const int MAP_WIDTH = 42;

    // ============================================================
    // HEADER
    // ============================================================

    std::string header =
        "[OS] PID 4201 | FL " +
        std::to_string(state.GetCurrentFloor()) +
        "/" +
        std::to_string(state.GetTotalFloors()) +
        " " +
        state.GetFloorName();

    bool warningHeader = false;

    if (state.GetFloorMsgTimer() > 0) {

        header += " | " + state.GetFloorMsg();

    }
    else if (state.GetCacheMsgTimer() > 0) {

        header += " | " + state.GetCacheMsg();

    }
    else if (state.IsBoss1Active()) {

        header +=
            " | [BOSS] THREAT: FORK BOMB EXPANSION";

        warningHeader = true;

    }
    else if (state.IsBoss2Active()) {

        if (state.IsDeadlockActive()) {

            header +=
                " | [BOSS] INVULNERABLE DEADLOCK: A->B, B->A";

            warningHeader = true;

        }
        else {

            header +=
                " | [BOSS] DEADLOCK BROKEN! EXECUTING...";

            warningHeader = true;
        }

    }
    else if (state.IsDeadlockActive()) {

        header +=
            " | [DEADLOCK] A: KEY_A->KEY_B | B: KEY_B->KEY_A";

        warningHeader = true;

    }
    else if (state.IsDeadlockResolved()) {

        header +=
            " | [DEADLOCK] RESOLVED: Cycle broken.";
    }

    if (warningHeader) {
        SetColor(console, COLOR_RED);
    }
    else {
        SetColor(console, COLOR_CYAN);
    }

    std::cout << header << "\n";
    ResetColor(console);

    // ============================================================
    // BUILD MAP
    // ============================================================

    std::vector<std::string> mapLines;

    for (int y = 0;
         y < static_cast<int>(map.size());
         ++y) {

        std::string line;

        for (int x = 0;
             x < static_cast<int>(map[y].size());
             ++x) {

            char tile = map[y][x];

            // Player
            if (x == px && y == py) {

                tile = '@';
            }

            // Cache
            else if (
                state.IsCacheActive() &&
                x == state.GetCacheX() &&
                y == state.GetCacheY()
            ) {

                tile = '$';
            }

            // Clone
            else if (
                state.GetCloneTimer() > 0 &&
                x == state.GetCloneX() &&
                y == state.GetCloneY()
            ) {

                tile = 'C';
            }

            // Fork
            else if (
                state.GetActiveForkTimer() > 0 &&
                x == state.GetActiveForkX() &&
                y == state.GetActiveForkY()
            ) {

                tile = 'F';
            }

            else {

                // Firewall
                if (
                    state.GetFirewallTimer() > 0 &&
                    tile == '.'
                ) {

                    if (
                        std::abs(x - px) <= 1 &&
                        std::abs(y - py) <= 1
                    ) {

                        tile = '+';
                    }
                }

                // Enemies
                for (const auto& enemy : enemies) {

                    if (
                        enemy.alive &&
                        enemy.x == x &&
                        enemy.y == y
                    ) {

                        if (
                            enemy.processState ==
                            ProcessState::ZOMBIE
                        ) {

                            tile = '%';
                        }
                        else {

                            switch (enemy.type) {

                                case EnemyType::WORKER:
                                    tile = 'W';
                                    break;

                                case EnemyType::DAEMON:
                                    tile = 'D';
                                    break;

                                case EnemyType::ZOMBIE:
                                    tile = 'Z';
                                    break;

                                case EnemyType::DEADLOCK_A:
                                    tile = 'A';
                                    break;

                                case EnemyType::DEADLOCK_B:
                                    tile = 'B';
                                    break;

                                case EnemyType::FORK_BOMB:
                                    tile = 'f';
                                    break;
                            }
                        }

                        break;
                    }
                }
            }

            line += tile;
        }

        mapLines.push_back(line);
    }

    // ============================================================
    // RIGHT SIDE COMMAND PANEL
    // ============================================================

    std::vector<std::string> panel;

    if (state.IsCacheUIOpen()) {

        panel.push_back("SYSTEM CACHE");
        panel.push_back("------------------------------");
        panel.push_back("");
        panel.push_back("1  KERNEL BLADE");
        panel.push_back("   +20 Damage");
        panel.push_back("   4 RAM / attack");
        panel.push_back("");
        panel.push_back("2  MEMORY UPGRADE");
        panel.push_back("   +16 Maximum RAM");
        panel.push_back("");
        panel.push_back("3  ENERGY CELL");
        panel.push_back("   -20 Used RAM");
        panel.push_back("");
        panel.push_back("4  HEALTH PACK");
        panel.push_back("   +25 HP");
        panel.push_back("");
        panel.push_back("Choose: 1 / 2 / 3 / 4");

    }
    else {

        panel.push_back("COMMANDS");
        panel.push_back("------------------------------");
        panel.push_back("W / S     Move Up / Down");
        panel.push_back("A / D     Move Left / Right");
        panel.push_back("F         Attack");
        panel.push_back("P         Save Game");
        panel.push_back("R         Clear RAM");
        panel.push_back("Q         Quit");

        panel.push_back("");
        panel.push_back("ABILITIES");
        panel.push_back("------------------------------");

        panel.push_back("1         Ability 1");
        panel.push_back("2         Ability 2");
        panel.push_back("3         Ability 3");
        panel.push_back("4         Ability 4");
        panel.push_back("8         Fork");

        panel.push_back("");
        panel.push_back("SYSTEM SIGNALS");
        panel.push_back("------------------------------");

        panel.push_back("5         SIGSTOP");
        panel.push_back("6         SIGTERM");
        panel.push_back("7         SIGKILL");

        panel.push_back("");
        panel.push_back("PLAYER");
        panel.push_back("------------------------------");

        std::string cdStr =
            state.GetPlayerCooldown() == 0
            ? "Ready"
            : std::to_string(state.GetPlayerCooldown());

        panel.push_back(
            "HP          " +
            std::to_string(state.GetPlayerHP())
        );

        panel.push_back(
            "RAM         " +
            std::to_string(state.GetPlayerUsedRAM()) +
            "/" +
            std::to_string(state.GetPlayerMaxRAM()) +
            " MB"
        );

        panel.push_back(
            "Cooldown    " + cdStr
        );
    }

    // ============================================================
    // MAP + PANEL
    // ============================================================

    size_t rows =
        mapLines.size() > panel.size()
        ? mapLines.size()
        : panel.size();

    // Never allow renderer to print past visible console.
    int availableRows = consoleHeight - 3;

    if (availableRows < 1) {
        availableRows = 1;
    }

    if (static_cast<int>(rows) > availableRows) {
        rows = static_cast<size_t>(availableRows);
    }

    for (size_t i = 0; i < rows; ++i) {

        std::string left =
            i < mapLines.size()
            ? mapLines[i]
            : "";

        std::string right =
            i < panel.size()
            ? panel[i]
            : "";

        if (left.length() > MAP_WIDTH) {
            left = left.substr(0, MAP_WIDTH);
        }

        // Print colored map.
        for (size_t x = 0; x < left.length(); ++x) {
            PrintTile(console, left[x]);
        }

        // Padding between map and panel.
        if (left.length() < static_cast<size_t>(MAP_WIDTH)) {

            std::cout
                << std::string(
                    MAP_WIDTH - left.length(),
                    ' '
                );
        }

        SetColor(console, COLOR_GRAY);
        std::cout << " | ";
        ResetColor(console);

        // Print colored panel.
        if (i < panel.size()) {
            PrintPanelLine(console, right);
        }
        else {
            std::cout << "\n";
        }
    }

    // ============================================================
    // BUFFS
    // ============================================================

    if (consoleHeight > 2) {

        std::string buffs = "BUFFS: ";

        bool activeBuffs = false;

        SetColor(console, COLOR_CYAN);
        std::cout << "BUFFS: ";
        ResetColor(console);

        if (state.GetCloneTimer() > 0) {

            SetColor(console, COLOR_MAGENTA);

            std::cout
                << "[Decay:"
                << state.GetCloneTimer()
                << "] ";

            ResetColor(console);

            activeBuffs = true;
        }

        if (state.GetFirewallTimer() > 0) {

            SetColor(console, COLOR_BLUE);

            std::cout
                << "[Firewall:"
                << state.GetFirewallTimer()
                << "] ";

            ResetColor(console);

            activeBuffs = true;
        }

        if (state.GetOverclockTimer() > 0) {

            SetColor(console, COLOR_YELLOW);

            std::cout
                << "[Overclock:"
                << state.GetOverclockTimer()
                << "] ";

            ResetColor(console);

            activeBuffs = true;
        }

        if (state.GetActiveForkTimer() > 0) {

            SetColor(console, COLOR_GREEN);

            std::cout
                << "[Fork:"
                << state.GetActiveForkTimer()
                << "] ";

            ResetColor(console);

            activeBuffs = true;
        }

        if (!activeBuffs) {

            SetColor(console, COLOR_GRAY);
            std::cout << "None";
            ResetColor(console);
        }

        std::cout << "\n";
    }

    // ============================================================
    // ENEMY STATUS
    // ============================================================

    int remainingRows =
        consoleHeight - 2;

    if (remainingRows > 0) {

        SetColor(console, COLOR_RED);
        std::cout << "\nENEMIES\n";
        ResetColor(console);

        remainingRows--;

        int printedEnemies = 0;

        for (const auto& enemy : enemies) {

            if (
                !enemy.alive ||
                enemy.processState ==
                ProcessState::TERMINATED
            ) {
                continue;
            }

            // Prevent enemy list from causing scrolling.
            if (printedEnemies >= remainingRows) {

                SetColor(console, COLOR_YELLOW);
                std::cout << "... more enemies ...\n";
                ResetColor(console);

                break;
            }

            std::string type;

            switch (enemy.type) {

                case EnemyType::WORKER:
                    type = "WORKER";
                    break;

                case EnemyType::DAEMON:
                    type = "DAEMON";
                    break;

                case EnemyType::ZOMBIE:
                    type = "ZOMBIE";
                    break;

                case EnemyType::DEADLOCK_A:
                    type = "DEADLOCK_A";
                    break;

                case EnemyType::DEADLOCK_B:
                    type = "DEADLOCK_B";
                    break;

                case EnemyType::FORK_BOMB:
                    type = "FORK_BOMB";
                    break;
            }

            std::string processState;

            switch (enemy.processState) {

                case ProcessState::RUNNING:
                    processState = "RUNNING";
                    break;

                case ProcessState::READY:
                    processState = "READY";
                    break;

                case ProcessState::WAITING:
                    processState = "WAITING";
                    break;

                case ProcessState::BLOCKED:
                    processState = "BLOCKED";
                    break;

                case ProcessState::ZOMBIE:
                    processState = "ZOMBIE";
                    break;

                case ProcessState::TERMINATED:
                    processState = "TERMINATED";
                    break;
            }

            std::string enemyState;

            switch (enemy.state) {

                case EnemyState::IDLE:
                    enemyState = "IDLE";
                    break;

                case EnemyState::CHASE:
                    enemyState = "CHASE";
                    break;

                case EnemyState::ATTACK:
                    enemyState = "ATTACK";
                    break;

                case EnemyState::SEARCH:
                    enemyState = "SEARCH";
                    break;
            }

            // Enemy color based on type.
            if (enemy.processState == ProcessState::ZOMBIE) {
                SetColor(console, COLOR_DARK_RED);
            }
            else if (
                enemy.type == EnemyType::DEADLOCK_A ||
                enemy.type == EnemyType::DEADLOCK_B
            ) {
                SetColor(console, COLOR_MAGENTA);
            }
            else if (enemy.type == EnemyType::FORK_BOMB) {
                SetColor(console, COLOR_YELLOW);
            }
            else {
                SetColor(console, COLOR_RED);
            }

            std::cout
                << type;

            ResetColor(console);

            std::cout
                << " | HP: ";

            // Low HP enemies are highlighted.
            if (enemy.currentHP <= 20) {
                SetColor(console, COLOR_RED);
            }
            else {
                SetColor(console, COLOR_WHITE);
            }

            std::cout
                << enemy.currentHP;

            ResetColor(console);

            std::cout
                << " | ";

            // Process state colors.
            if (enemy.processState == ProcessState::RUNNING) {
                SetColor(console, COLOR_GREEN);
            }
            else if (enemy.processState == ProcessState::BLOCKED) {
                SetColor(console, COLOR_YELLOW);
            }
            else if (enemy.processState == ProcessState::ZOMBIE) {
                SetColor(console, COLOR_DARK_RED);
            }
            else {
                SetColor(console, COLOR_GRAY);
            }

            std::cout << processState;

            ResetColor(console);

            std::cout << " | ";

            // Enemy AI state.
            if (enemy.state == EnemyState::ATTACK) {
                SetColor(console, COLOR_RED);
            }
            else if (enemy.state == EnemyState::CHASE) {
                SetColor(console, COLOR_YELLOW);
            }
            else if (enemy.state == EnemyState::SEARCH) {
                SetColor(console, COLOR_MAGENTA);
            }
            else {
                SetColor(console, COLOR_GRAY);
            }

            std::cout << enemyState;

            ResetColor(console);

            if (enemy.state == EnemyState::SEARCH) {

                std::cout
                    << " | Last Seen: ("
                    << enemy.lastKnownPlayerX
                    << ","
                    << enemy.lastKnownPlayerY
                    << ")";
            }

            std::cout << "\n";

            printedEnemies++;
        }

        if (printedEnemies == 0) {

            SetColor(console, COLOR_GREEN);
            std::cout << "None\n";
            ResetColor(console);
        }
    }

    // ============================================================
    // END GAME MESSAGE
    // ============================================================

    if (state.IsGameWon()) {

        SetColor(console, COLOR_GREEN);

        std::cout
            << "\n[SYSTEM] KERNEL ACCESS GRANTED\n"
            << "[SYSTEM] RUN COMPLETE\n";

        ResetColor(console);

    }
    else if (state.HasSegfaulted()) {

        SetColor(console, COLOR_RED);

        std::cout
            << "\n[FATAL ERROR] SEGMENTATION FAULT "
            << "(RAM EXCEEDED)\n";

        ResetColor(console);

    }
    else if (!state.IsPlayerAlive()) {

        SetColor(console, COLOR_RED);

        std::cout
            << "\n[FATAL ERROR] YOU DIED\n";

        ResetColor(console);
    }

    // Always return to normal console color.
    ResetColor(console);

    std::cout.flush();
}