#include "GameState.h"
#include <cmath>
#include <algorithm>
#include <queue>
#include <functional>
#include <fstream>
#include <sstream>

// Helper logic to sanitize and serialize strings simply
static std::string escapeSpaces(const std::string& s) {
    if (s.empty()) return "NONE";
    std::string res = s;
    for (char& c : res) if (c == ' ') c = '_';
    return res;
}

static std::string unescapeSpaces(const std::string& s) {
    if (s == "NONE") return "";
    std::string res = s;
    for (char& c : res) if (c == '_') c = ' ';
    return res;
}

struct AStarNode {
    int x, y, f, g, h;
    bool operator>(const AStarNode& other) const {
        if (f == other.f) return h > other.h;
        return f > other.f;
    }
};

GameState::GameState(unsigned int seed) : baseSeed(seed), currentFloor(0), totalFloors(6), gameWon(false),
                         playerHP(100), playerDamage(15), playerArmor(3), playerCooldown(0), playerAlive(true), 
                         running(true), playerUsedRAM(0), playerMaxRAM(64), segfault(false),
                         cloneTimer(0), cloneX(0), cloneY(0), firewallTimer(0), overclockTimer(0),
                         activeForkTimer(0), activeForkX(0), activeForkY(0),
                         deadlockActive(false), deadlockResolved(false),
                         cacheX(0), cacheY(0), cacheActive(false), cacheUIOpen(false),
                         playerAttackRamCost(0), cacheMsg(""), cacheMsgTimer(0),
                         floorMsg(""), floorMsgTimer(0),
                         boss1Active(false), boss1Defeated(false),
                         boss2Active(false), boss2Defeated(false),
                         boss3Active(false), boss3Defeated(false),
                         saveFilename("save.dat") {
    GenerateDungeon(baseSeed + currentFloor); 
}

bool GameState::RoomsIntersect(const Room& a, const Room& b) const {
    return (a.x <= b.x + b.w + 1 && a.x + a.w + 1 >= b.x &&
            a.y <= b.y + b.h + 1 && a.y + a.h + 1 >= b.y);
}

void GameState::AllocateRAM(int amount) {
    playerUsedRAM += amount;
    if (playerUsedRAM > playerMaxRAM) {
        segfault = true;
        playerAlive = false; 
    }
}

void GameState::FreeRAM(int amount) {
    playerUsedRAM -= amount;
    if (playerUsedRAM < 0) playerUsedRAM = 0;
}

std::string GameState::GetFloorName() const {
    const char* names[] = {"/boot", "/tmp", "/var", "/etc", "/proc", "/kernel"};
    if (currentFloor >= 0 && currentFloor < 6) return names[currentFloor];
    return "/unknown";
}

bool GameState::SaveToFile(const std::string& filename) const {
    std::ofstream out(filename);
    if (!out.is_open()) return false;

    out << "LAST_PROCESS_SAVE\n";
    out << "VERSION 1\n";
    out << "SEED " << baseSeed << "\n";
    out << "FLOOR " << currentFloor << "\n";
    out << "TOTAL_FLOORS " << totalFloors << "\n";
    out << "EXIT " << exitX << " " << exitY << "\n";
    out << "GAME_WON " << (gameWon ? 1 : 0) << "\n";

    out << "PLAYER\n";
    out << "X " << playerX << "\n";
    out << "Y " << playerY << "\n";
    out << "HP " << playerHP << "\n";
    out << "DAMAGE " << playerDamage << "\n";
    out << "ARMOR " << playerArmor << "\n";
    out << "COOLDOWN " << playerCooldown << "\n";
    out << "USED_RAM " << playerUsedRAM << "\n";
    out << "MAX_RAM " << playerMaxRAM << "\n";
    out << "ATTACK_RAM_COST " << playerAttackRamCost << "\n";
    out << "ALIVE " << (playerAlive ? 1 : 0) << "\n";
    out << "SEGFAULT " << (segfault ? 1 : 0) << "\n";
    out << "RUNNING " << (running ? 1 : 0) << "\n";

    out << "EFFECTS\n";
    out << "CLONE " << cloneTimer << " " << cloneX << " " << cloneY << "\n";
    out << "FIREWALL " << firewallTimer << "\n";
    out << "OVERCLOCK " << overclockTimer << "\n";
    out << "FORK " << activeForkTimer << " " << activeForkX << " " << activeForkY << "\n";

    out << "WORLD\n";
    out << "DEADLOCK " << (deadlockActive ? 1 : 0) << " " << (deadlockResolved ? 1 : 0) << "\n";
    out << "CACHE " << cacheX << " " << cacheY << " " << (cacheActive ? 1 : 0) << " " << (cacheUIOpen ? 1 : 0) << "\n";
    out << "MSG_TIMERS " << cacheMsgTimer << " " << floorMsgTimer << "\n";
    out << "CACHE_MSG " << escapeSpaces(cacheMsg) << "\n";
    out << "FLOOR_MSG " << escapeSpaces(floorMsg) << "\n";

    out << "BOSS " << (boss1Active ? 1 : 0) << " " << (boss1Defeated ? 1 : 0) << " "
        << (boss2Active ? 1 : 0) << " " << (boss2Defeated ? 1 : 0) << " "
        << (boss3Active ? 1 : 0) << " " << (boss3Defeated ? 1 : 0) << "\n";

    out << "MAP\n";
    for (const auto& row : map) {
        out << row << "\n";
    }

    out << "ENEMIES " << enemies.size() << "\n";
    for (const auto& e : enemies) {
        out << "ENEMY\n";
        out << "TYPE " << static_cast<int>(e.type) << "\n";
        out << "POS " << e.x << " " << e.y << "\n";
        out << "HP " << e.maxHP << " " << e.currentHP << "\n";
        out << "STATS " << e.damage << " " << e.armor << " " << e.speed << " " << e.turnCounter << "\n";
        out << "ALIVE " << (e.alive ? 1 : 0) << "\n";
        out << "STATE " << static_cast<int>(e.state) << "\n";
        out << "LAST_POS " << e.lastKnownPlayerX << " " << e.lastKnownPlayerY << "\n";
        out << "PROC_STATE " << static_cast<int>(e.processState) << "\n";
        out << "TIMERS " << e.blockedTimer << " " << e.zombieTimer << "\n";
        out << "HOLDS " << escapeSpaces(e.holdsResource) << "\n";
        out << "WAITS " << escapeSpaces(e.waitsForResource) << "\n";
        out << "END_ENEMY\n";
    }

    out << "END_SAVE\n";
    return true;
}

bool GameState::LoadFromFile(const std::string& filename) {
    std::ifstream in(filename);
    if (!in.is_open()) return false;

    GameState temp(0);

    std::string token;
    if (!(in >> token) || token != "LAST_PROCESS_SAVE") return false;
    
    int version;
    if (!(in >> token >> version) || token != "VERSION" || version != 1) return false;

    if (!(in >> token >> temp.baseSeed) || token != "SEED") return false;
    if (!(in >> token >> temp.currentFloor) || token != "FLOOR") return false;
    if (!(in >> token >> temp.totalFloors) || token != "TOTAL_FLOORS") return false;
    if (!(in >> token >> temp.exitX >> temp.exitY) || token != "EXIT") return false;
    
    int tGW;
    if (!(in >> token >> tGW) || token != "GAME_WON") return false;
    temp.gameWon = tGW;

    if (!(in >> token) || token != "PLAYER") return false;
    if (!(in >> token >> temp.playerX) || token != "X") return false;
    if (!(in >> token >> temp.playerY) || token != "Y") return false;
    if (!(in >> token >> temp.playerHP) || token != "HP") return false;
    if (!(in >> token >> temp.playerDamage) || token != "DAMAGE") return false;
    if (!(in >> token >> temp.playerArmor) || token != "ARMOR") return false;
    if (!(in >> token >> temp.playerCooldown) || token != "COOLDOWN") return false;
    if (!(in >> token >> temp.playerUsedRAM) || token != "USED_RAM") return false;
    if (!(in >> token >> temp.playerMaxRAM) || token != "MAX_RAM") return false;
    if (!(in >> token >> temp.playerAttackRamCost) || token != "ATTACK_RAM_COST") return false;

    int tAlive, tSeg, tRun;
    if (!(in >> token >> tAlive) || token != "ALIVE") return false;
    if (!(in >> token >> tSeg) || token != "SEGFAULT") return false;
    if (!(in >> token >> tRun) || token != "RUNNING") return false;
    temp.playerAlive = tAlive;
    temp.segfault = tSeg;
    temp.running = tRun;

    if (!(in >> token) || token != "EFFECTS") return false;
    if (!(in >> token >> temp.cloneTimer >> temp.cloneX >> temp.cloneY) || token != "CLONE") return false;
    if (!(in >> token >> temp.firewallTimer) || token != "FIREWALL") return false;
    if (!(in >> token >> temp.overclockTimer) || token != "OVERCLOCK") return false;
    if (!(in >> token >> temp.activeForkTimer >> temp.activeForkX >> temp.activeForkY) || token != "FORK") return false;

    if (!(in >> token) || token != "WORLD") return false;
    int tDA, tDR;
    if (!(in >> token >> tDA >> tDR) || token != "DEADLOCK") return false;
    temp.deadlockActive = tDA; temp.deadlockResolved = tDR;

    int tCA, tCUI;
    if (!(in >> token >> temp.cacheX >> temp.cacheY >> tCA >> tCUI) || token != "CACHE") return false;
    temp.cacheActive = tCA; temp.cacheUIOpen = tCUI;

    if (!(in >> token >> temp.cacheMsgTimer >> temp.floorMsgTimer) || token != "MSG_TIMERS") return false;
    
    std::string tCMSG, tFMSG;
    if (!(in >> token >> tCMSG) || token != "CACHE_MSG") return false;
    if (!(in >> token >> tFMSG) || token != "FLOOR_MSG") return false;
    temp.cacheMsg = unescapeSpaces(tCMSG);
    temp.floorMsg = unescapeSpaces(tFMSG);

    int tB1A, tB1D, tB2A, tB2D, tB3A, tB3D;
    if (!(in >> token >> tB1A >> tB1D >> tB2A >> tB2D >> tB3A >> tB3D) || token != "BOSS") return false;
    temp.boss1Active = tB1A; temp.boss1Defeated = tB1D;
    temp.boss2Active = tB2A; temp.boss2Defeated = tB2D;
    temp.boss3Active = tB3A; temp.boss3Defeated = tB3D;

    if (!(in >> token) || token != "MAP") return false;
    temp.map.clear();
    for (int i = 0; i < 20; ++i) {
        std::string row;
        if (!(in >> row) || row.size() != 20) return false;
        temp.map.push_back(row);
    }

    int eCount;
    if (!(in >> token >> eCount) || token != "ENEMIES") return false;
    temp.enemies.clear();
    for (int i = 0; i < eCount; ++i) {
        EnemyData ed;
        if (!(in >> token) || token != "ENEMY") return false;
        
        int tType, tState, tProcState, tEAlive;
        if (!(in >> token >> tType) || token != "TYPE") return false;
        if (!(in >> token >> ed.x >> ed.y) || token != "POS") return false;
        if (!(in >> token >> ed.maxHP >> ed.currentHP) || token != "HP") return false;
        if (!(in >> token >> ed.damage >> ed.armor >> ed.speed >> ed.turnCounter) || token != "STATS") return false;
        if (!(in >> token >> tEAlive) || token != "ALIVE") return false;
        if (!(in >> token >> tState) || token != "STATE") return false;
        if (!(in >> token >> ed.lastKnownPlayerX >> ed.lastKnownPlayerY) || token != "LAST_POS") return false;
        if (!(in >> token >> tProcState) || token != "PROC_STATE") return false;
        if (!(in >> token >> ed.blockedTimer >> ed.zombieTimer) || token != "TIMERS") return false;
        
        std::string hRes, wRes;
        if (!(in >> token >> hRes) || token != "HOLDS") return false;
        if (!(in >> token >> wRes) || token != "WAITS") return false;
        ed.holdsResource = unescapeSpaces(hRes);
        ed.waitsForResource = unescapeSpaces(wRes);

        if (!(in >> token) || token != "END_ENEMY") return false;

        if (tType < 0 || tType > 5) return false;
        if (tState < 0 || tState > 3) return false;
        if (tProcState < 0 || tProcState > 5) return false;

        ed.type = static_cast<EnemyType>(tType);
        ed.state = static_cast<EnemyState>(tState);
        ed.processState = static_cast<ProcessState>(tProcState);
        ed.alive = tEAlive;

        temp.enemies.push_back(ed);
    }

    if (!(in >> token) || token != "END_SAVE") return false;

    std::string currentSaveName = this->saveFilename;
    *this = temp;
    this->saveFilename = currentSaveName;

    return true;
}

void GameState::GenerateDungeon(unsigned int seed) {
    if (activeForkTimer > 0) {
        FreeRAM(16);
    }
    
    cloneTimer = 0; firewallTimer = 0; overclockTimer = 0; activeForkTimer = 0;
    playerCooldown = 0;
    deadlockActive = false; deadlockResolved = false;
    cacheUIOpen = false;
    
    boss1Active = false; boss1Defeated = false;
    boss2Active = false; boss2Defeated = false;
    boss3Active = false; boss3Defeated = false;

    std::mt19937 rng(seed);
    bool valid = false;

    while (!valid) {
        map.assign(20, std::string(20, '#'));
        std::vector<Room> rooms;
        
        std::uniform_int_distribution<int> roomCountDist(5, 8);
        std::uniform_int_distribution<int> dimDist(3, 6);
        int targetRooms = roomCountDist(rng);

        for (int i = 0; i < 50 && rooms.size() < targetRooms; ++i) {
            int w = dimDist(rng);
            int h = dimDist(rng);
            std::uniform_int_distribution<int> xDist(1, 18 - w);
            std::uniform_int_distribution<int> yDist(1, 18 - h);
            
            Room newRoom{ xDist(rng), yDist(rng), w, h };
            
            bool overlap = false;
            for (const auto& r : rooms) {
                if (RoomsIntersect(newRoom, r)) {
                    overlap = true;
                    break;
                }
            }
            if (!overlap) rooms.push_back(newRoom);
        }

        for (const auto& room : rooms) {
            for (int y = room.y; y < room.y + room.h; ++y) {
                for (int x = room.x; x < room.x + room.w; ++x) {
                    map[y][x] = '.';
                }
            }
        }

        for (size_t i = 1; i < rooms.size(); ++i) {
            int x1 = rooms[i-1].centerX();
            int y1 = rooms[i-1].centerY();
            int x2 = rooms[i].centerX();
            int y2 = rooms[i].centerY();

            int currX = x1;
            int currY = y1;
            while (currX != x2) {
                map[currY][currX] = '.';
                currX += (x2 > currX) ? 1 : -1;
            }
            while (currY != y2) {
                map[currY][currX] = '.';
                currY += (y2 > currY) ? 1 : -1;
            }
        }

        int floorCount = 0;
        for (const auto& row : map) {
            for (char c : row) {
                if (c == '.') floorCount++;
            }
        }

        if (rooms.empty()) continue;

        int startX = rooms[0].centerX();
        int startY = rooms[0].centerY();
        std::vector<std::vector<bool>> visited(20, std::vector<bool>(20, false));
        std::queue<std::pair<int, int>> q;
        
        q.push({startX, startY});
        visited[startY][startX] = true;
        int reachable = 0;

        while (!q.empty()) {
            int cx = q.front().first;
            int cy = q.front().second;
            q.pop();
            reachable++;

            int dx[] = {0, 0, -1, 1};
            int dy[] = {-1, 1, 0, 0};
            for (int i = 0; i < 4; ++i) {
                int nx = cx + dx[i];
                int ny = cy + dy[i];
                if (nx >= 0 && nx < 20 && ny >= 0 && ny < 20 && map[ny][nx] == '.' && !visited[ny][nx]) {
                    visited[ny][nx] = true;
                    q.push({nx, ny});
                }
            }
        }

        if (reachable == floorCount) {
            valid = true;
            playerX = startX;
            playerY = startY;
            
            enemies.clear();
            std::vector<std::pair<int, int>> floorTiles;
            for (int y = 0; y < 20; ++y) {
                for (int x = 0; x < 20; ++x) {
                    if (map[y][x] == '.' && !(x == playerX && y == playerY)) {
                        floorTiles.push_back({x, y});
                    }
                }
            }
            
            std::shuffle(floorTiles.begin(), floorTiles.end(), rng);
            
            int tIdx = 0;
            auto spawnEnemy = [&](EnemyType type, int hp, int dmg, int armor, int speed, std::string holds, std::string waits) {
                if (tIdx < floorTiles.size()) {
                    enemies.push_back({type, floorTiles[tIdx].first, floorTiles[tIdx].second, hp, hp, dmg, armor, speed, 0, true, EnemyState::IDLE, -1, -1, ProcessState::READY, 0, 0, holds, waits});
                    tIdx++;
                }
            };
            
            if (currentFloor == 0) { // /boot
                spawnEnemy(EnemyType::WORKER, 30, 5, 3, 2, "", "");
                spawnEnemy(EnemyType::DAEMON, 50, 8, 5, 3, "", "");
                spawnEnemy(EnemyType::ZOMBIE, 20, 12, 1, 1, "", "");
                spawnEnemy(EnemyType::DEADLOCK_A, 40, 0, 2, 2, "KEY_A", "KEY_B");
                spawnEnemy(EnemyType::DEADLOCK_B, 40, 0, 2, 2, "KEY_B", "KEY_A");
                for (auto& e : enemies) {
                    if (e.type == EnemyType::DEADLOCK_A || e.type == EnemyType::DEADLOCK_B) e.processState = ProcessState::WAITING;
                }
                deadlockActive = true;
            } else if (currentFloor == 1) { // /tmp
                spawnEnemy(EnemyType::WORKER, 30, 5, 3, 1, "", "");
                spawnEnemy(EnemyType::DAEMON, 50, 8, 5, 2, "", "");
                spawnEnemy(EnemyType::WORKER, 30, 5, 3, 1, "", "");
                spawnEnemy(EnemyType::ZOMBIE, 20, 12, 1, 1, "", "");
            } else if (currentFloor == 2) { // /var (Boss 1)
                spawnEnemy(EnemyType::FORK_BOMB, 15, 5, 0, 3, "", ""); 
                boss1Active = true;
            } else if (currentFloor == 3) { // /etc
                spawnEnemy(EnemyType::WORKER, 30, 5, 3, 2, "", "");
                spawnEnemy(EnemyType::DAEMON, 50, 8, 5, 3, "", "");
                spawnEnemy(EnemyType::ZOMBIE, 20, 12, 1, 1, "", "");
            } else if (currentFloor == 4) { // /proc (Boss 2)
                spawnEnemy(EnemyType::WORKER, 60, 10, 5, 2, "", "");
                spawnEnemy(EnemyType::DAEMON, 100, 16, 8, 3, "", "");
                spawnEnemy(EnemyType::ZOMBIE, 40, 20, 3, 1, "", "");
                spawnEnemy(EnemyType::DEADLOCK_A, 150, 20, 999, 2, "KEY_A", "KEY_B");
                spawnEnemy(EnemyType::DEADLOCK_B, 150, 20, 999, 2, "KEY_B", "KEY_A");
                for (auto& e : enemies) {
                    if (e.type == EnemyType::DEADLOCK_A || e.type == EnemyType::DEADLOCK_B) e.processState = ProcessState::WAITING;
                }
                deadlockActive = true;
                boss2Active = true;
            } else if (currentFloor == 5) { // /kernel
                spawnEnemy(EnemyType::WORKER, 80, 12, 6, 2, "", "");
                spawnEnemy(EnemyType::DAEMON, 120, 20, 10, 3, "", "");
                spawnEnemy(EnemyType::ZOMBIE, 50, 25, 4, 1, "", "");
                spawnEnemy(EnemyType::DEADLOCK_A, 80, 0, 5, 2, "KEY_A", "KEY_B");
                spawnEnemy(EnemyType::DEADLOCK_B, 80, 0, 5, 2, "KEY_B", "KEY_A");
                for (auto& e : enemies) {
                    if (e.type == EnemyType::DEADLOCK_A || e.type == EnemyType::DEADLOCK_B) e.processState = ProcessState::WAITING;
                }
                deadlockActive = true;
                boss3Active = true; 
            }

            if (tIdx < floorTiles.size()) {
                exitX = floorTiles[tIdx].first;
                exitY = floorTiles[tIdx].second;
                if (currentFloor == 5) {
                    map[exitY][exitX] = 'T';
                } else if (currentFloor == 2 && boss1Active) {
                    map[exitY][exitX] = '.'; 
                } else if (currentFloor == 4 && boss2Active) {
                    map[exitY][exitX] = '.'; 
                } else {
                    map[exitY][exitX] = '>';
                }
                tIdx++;
            }
            if (tIdx < floorTiles.size()) {
                cacheX = floorTiles[tIdx].first;
                cacheY = floorTiles[tIdx].second;
                cacheActive = true;
                tIdx++;
            }
            if (currentFloor == 3) { 
                for (int i = 0; i < 10; ++i) { 
                    if (tIdx < floorTiles.size()) {
                        map[floorTiles[tIdx].second][floorTiles[tIdx].first] = '^';
                        tIdx++;
                    }
                }
            }
            
            if (currentFloor > 0) {
                if (currentFloor == 2) floorMsg = "[SYS] BOSS DETECTED: FORK BOMB";
                else if (currentFloor == 4) floorMsg = "[SYS] BOSS DETECTED: INVULNERABLE DEADLOCK";
                else floorMsg = "[SYS] Entering " + GetFloorName() + "...";
                floorMsgTimer = 5;
            }
        }
    }
}

std::vector<std::pair<int, int>> GameState::FindPath(int startX, int startY, int goalX, int goalY) const {
    std::vector<std::pair<int, int>> path;
    if (startX == goalX && startY == goalY) return path;

    int gCost[20][20];
    std::pair<int, int> parent[20][20];
    
    for (int i = 0; i < 20; ++i) {
        for (int j = 0; j < 20; ++j) {
            gCost[i][j] = 999999;
            parent[i][j] = {-1, -1};
        }
    }

    std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> pq;

    gCost[startY][startX] = 0;
    int initialH = std::abs(startX - goalX) + std::abs(startY - goalY);
    pq.push({startX, startY, initialH, 0, initialH});

    bool found = false;

    while (!pq.empty()) {
        AStarNode curr = pq.top();
        pq.pop();

        if (curr.x == goalX && curr.y == goalY) {
            found = true;
            break;
        }

        if (curr.g > gCost[curr.y][curr.x]) continue;

        int dx[] = {0, 0, -1, 1};
        int dy[] = {-1, 1, 0, 0};

        for (int i = 0; i < 4; ++i) {
            int nx = curr.x + dx[i];
            int ny = curr.y + dy[i];

            if (nx >= 0 && nx < 20 && ny >= 0 && ny < 20) {
                if (map[ny][nx] != '#') {
                    int tentative_g = curr.g + 1;
                    if (tentative_g < gCost[ny][nx]) {
                        gCost[ny][nx] = tentative_g;
                        parent[ny][nx] = {curr.x, curr.y};
                        int h = std::abs(nx - goalX) + std::abs(ny - goalY);
                        pq.push({nx, ny, tentative_g + h, tentative_g, h});
                    }
                }
            }
        }
    }

    if (found) {
        int cx = goalX;
        int cy = goalY;
        while (cx != startX || cy != startY) {
            path.push_back({cx, cy});
            auto p = parent[cy][cx];
            cx = p.first;
            cy = p.second;
        }
    }

    return path;
}

void GameState::Update(Command cmd) {
    if (cmd == Command::QUIT) {
        running = false;
        return;
    }

    if (!playerAlive || segfault || gameWon || cmd == Command::NONE) {
        return;
    }

    if (cmd == Command::SAVE_CMD) {
        if (SaveToFile(saveFilename)) {
            floorMsg = "[SYSTEM] GAME SAVED.";
        } else {
            floorMsg = "[SYSTEM] SAVE FAILED.";
        }
        floorMsgTimer = 5;
        return;
    }

    if (cacheUIOpen) {
        if (cmd == Command::ABILITY_1) {
            playerDamage += 20;
            playerAttackRamCost = 4;
            cacheMsg = "Acquired KERNEL BLADE!";
            cacheMsgTimer = 5;
            cacheActive = false;
            cacheUIOpen = false;
        } else if (cmd == Command::ABILITY_2) {
            playerMaxRAM += 16;
            cacheMsg = "Acquired MEMORY UPGRADE!";
            cacheMsgTimer = 5;
            cacheActive = false;
            cacheUIOpen = false;
        } else if (cmd == Command::ABILITY_3) {
            FreeRAM(20);
            cacheMsg = "Acquired ENERGY CELL!";
            cacheMsgTimer = 5;
            cacheActive = false;
            cacheUIOpen = false;
        } else if (cmd == Command::ABILITY_4) {
            playerHP = std::min(100, playerHP + 25);
            cacheMsg = "Acquired HEALTH PACK!";
            cacheMsgTimer = 5;
            cacheActive = false;
            cacheUIOpen = false;
        }
        return;
    }

    if (cmd == Command::ATTACK && playerUsedRAM + playerAttackRamCost > playerMaxRAM) return;
    if (cmd == Command::ABILITY_1) {
        if (playerUsedRAM + 12 > playerMaxRAM) return;
        bool spaceFound = false;
        int dx[] = {0, 0, -1, 1};
        int dy[] = {-1, 1, 0, 0};
        for (int i = 0; i < 4; ++i) {
            int nx = playerX + dx[i];
            int ny = playerY + dy[i];
            if (nx >= 0 && nx < 20 && ny >= 0 && ny < 20 && map[ny][nx] != '#') {
                bool occupied = false;
                for (const auto& e : enemies) {
                    if (e.alive && e.processState != ProcessState::TERMINATED && e.x == nx && e.y == ny) occupied = true;
                }
                if (activeForkTimer > 0 && nx == activeForkX && ny == activeForkY) occupied = true;
                if (!occupied) { spaceFound = true; break; }
            }
        }
        if (!spaceFound) return;
    } 
    else if (cmd == Command::ABILITY_2 && playerUsedRAM + 8 > playerMaxRAM) return;
    else if (cmd == Command::ABILITY_3 && playerUsedRAM + 16 > playerMaxRAM) return;
    else if (cmd == Command::ABILITY_4 && playerUsedRAM + 20 > playerMaxRAM) return;
    else if (cmd == Command::SIGSTOP_CMD && playerUsedRAM + 8 > playerMaxRAM) return;
    else if (cmd == Command::SIGTERM_CMD && playerUsedRAM + 12 > playerMaxRAM) return;
    else if (cmd == Command::SIGKILL_CMD && playerUsedRAM + 30 > playerMaxRAM) return;
    else if (cmd == Command::FORK_CMD) {
        if (activeForkTimer > 0) return; 
        if (playerUsedRAM + 16 > playerMaxRAM) return;
        bool spaceFound = false;
        int dx[] = {0, 0, -1, 1};
        int dy[] = {-1, 1, 0, 0};
        for (int i = 0; i < 4; ++i) {
            int nx = playerX + dx[i];
            int ny = playerY + dy[i];
            if (nx >= 0 && nx < 20 && ny >= 0 && ny < 20 && map[ny][nx] != '#') {
                bool occupied = false;
                for (const auto& e : enemies) {
                    if (e.alive && e.processState != ProcessState::TERMINATED && e.x == nx && e.y == ny) occupied = true;
                }
                if (cloneTimer > 0 && nx == cloneX && ny == cloneY) occupied = true;
                if (!occupied) { spaceFound = true; break; }
            }
        }
        if (!spaceFound) return;
    }

    if (cloneTimer > 0) cloneTimer--;
    if (firewallTimer > 0) firewallTimer--;
    if (overclockTimer > 0) overclockTimer--;
    if (activeForkTimer > 0) {
        activeForkTimer--;
        if (activeForkTimer == 0) FreeRAM(16);
    }
    if (cacheMsgTimer > 0) cacheMsgTimer--;
    if (floorMsgTimer > 0) floorMsgTimer--;

    bool justAttacked = false;

    auto getTarget = [&]() -> EnemyData* {
        int minDist = 9999;
        EnemyData* target = nullptr;
        for (auto& enemy : enemies) {
            if (enemy.alive && enemy.processState != ProcessState::ZOMBIE && enemy.processState != ProcessState::TERMINATED) {
                int dist = std::abs(playerX - enemy.x) + std::abs(playerY - enemy.y);
                if (dist <= 3) {
                    if (dist < minDist) {
                        minDist = dist;
                        target = &enemy;
                    } else if (dist == minDist && target != nullptr) {
                        if (enemy.y < target->y || (enemy.y == target->y && enemy.x < target->x)) {
                            target = &enemy;
                        }
                    }
                }
            }
        }
        return target;
    };

    if (cmd == Command::ATTACK) {
        if (playerCooldown == 0) {
            for (auto& enemy : enemies) {
                if (enemy.alive && enemy.processState != ProcessState::ZOMBIE) {
                    int dist = std::abs(playerX - enemy.x) + std::abs(playerY - enemy.y);
                    if (dist == 1) {
                        AllocateRAM(playerAttackRamCost); 
                        int currentDamage = (overclockTimer > 0) ? (playerDamage * 2) : playerDamage;
                        int dmg = std::max(0, currentDamage - enemy.armor);
                        enemy.currentHP -= dmg;
                        if (enemy.currentHP <= 0) {
                            enemy.processState = ProcessState::ZOMBIE;
                            enemy.zombieTimer = 3;
                        }
                        playerCooldown = 2;
                        justAttacked = true;
                        break; 
                    }
                }
            }
        }
    } else if (cmd == Command::ABILITY_1) {
        AllocateRAM(12);
        int dx[] = {0, 0, -1, 1};
        int dy[] = {-1, 1, 0, 0};
        for (int i = 0; i < 4; ++i) {
            int nx = playerX + dx[i];
            int ny = playerY + dy[i];
            if (nx >= 0 && nx < 20 && ny >= 0 && ny < 20 && map[ny][nx] != '#') {
                bool occupied = false;
                for (const auto& e : enemies) {
                    if (e.alive && e.processState != ProcessState::TERMINATED && e.x == nx && e.y == ny) occupied = true;
                }
                if (activeForkTimer > 0 && nx == activeForkX && ny == activeForkY) occupied = true;
                if (!occupied) {
                    cloneX = nx;
                    cloneY = ny;
                    cloneTimer = 3;
                    break;
                }
            }
        }
    } else if (cmd == Command::ABILITY_2) {
        AllocateRAM(8);
        firewallTimer = 3;
    } else if (cmd == Command::ABILITY_3) {
        AllocateRAM(16);
        overclockTimer = 3;
    } else if (cmd == Command::ABILITY_4) {
        AllocateRAM(20);
        for (auto& enemy : enemies) {
            if (enemy.alive && enemy.processState != ProcessState::ZOMBIE) {
                int dist = std::abs(playerX - enemy.x) + std::abs(playerY - enemy.y);
                if (dist <= 3) {
                    enemy.currentHP -= 30; 
                    if (enemy.currentHP <= 0) {
                        enemy.processState = ProcessState::ZOMBIE;
                        enemy.zombieTimer = 3;
                    }
                }
            }
        }
    } else if (cmd == Command::SIGSTOP_CMD) {
        AllocateRAM(8);
        EnemyData* target = getTarget();
        if (target) {
            target->processState = ProcessState::BLOCKED;
            target->blockedTimer = 3;
        }
    } else if (cmd == Command::SIGTERM_CMD) {
        AllocateRAM(12);
        EnemyData* target = getTarget();
        if (target) {
            target->currentHP -= 25;
            if (target->currentHP <= 0) {
                target->processState = ProcessState::ZOMBIE;
                target->zombieTimer = 3;
            }
        }
    } else if (cmd == Command::SIGKILL_CMD) {
        AllocateRAM(30);
        EnemyData* target = getTarget();
        if (target) {
            target->currentHP = 0;
            target->processState = ProcessState::TERMINATED;
            target->alive = false;
        }
    } else if (cmd == Command::FORK_CMD) {
        AllocateRAM(16);
        int dx[] = {0, 0, -1, 1};
        int dy[] = {-1, 1, 0, 0};
        for (int i = 0; i < 4; ++i) {
            int nx = playerX + dx[i];
            int ny = playerY + dy[i];
            if (nx >= 0 && nx < 20 && ny >= 0 && ny < 20 && map[ny][nx] != '#') {
                bool occupied = false;
                for (const auto& e : enemies) {
                    if (e.alive && e.processState != ProcessState::TERMINATED && e.x == nx && e.y == ny) occupied = true;
                }
                if (cloneTimer > 0 && nx == cloneX && ny == cloneY) occupied = true;
                if (!occupied) {
                    activeForkX = nx;
                    activeForkY = ny;
                    activeForkTimer = 8;
                    break;
                }
            }
        }
    } else if (cmd == Command::CLEAR_RAM) {
        FreeRAM(playerMaxRAM);
    } else {
        int nextX = playerX;
        int nextY = playerY;

        switch (cmd) {
            case Command::UP:    nextY--; break;
            case Command::DOWN:  nextY++; break;
            case Command::LEFT:  nextX--; break;
            case Command::RIGHT: nextX++; break;
            default: break;
        }

        if (nextY >= 0 && nextY < map.size() && nextX >= 0 && nextX < map[nextY].size()) {
            if (map[nextY][nextX] != '#') {
                bool occupied = false;
                for (const auto& enemy : enemies) {
                    if (enemy.alive && enemy.processState != ProcessState::TERMINATED && enemy.x == nextX && enemy.y == nextY) {
                        occupied = true;
                        break;
                    }
                }
                if (activeForkTimer > 0 && nextX == activeForkX && nextY == activeForkY) occupied = true;
                
                if (!occupied) {
                    playerX = nextX;
                    playerY = nextY;

                    if (map[playerY][playerX] == '^') {
                        playerHP -= 5;
                        map[playerY][playerX] = '.'; 
                        if (playerHP <= 0) playerAlive = false;
                    } else if (playerX == exitX && playerY == exitY) {
                        if (currentFloor == 2 && boss1Active && !boss1Defeated) {
                            // Block exit
                        } else if (currentFloor == 4 && boss2Active && !boss2Defeated) {
                            // Block exit
                        } else {
                            if (currentFloor < 5) {
                                currentFloor++;
                                GenerateDungeon(baseSeed + currentFloor);
                                return; 
                            } else if (currentFloor == 5) {
                                gameWon = true;
                                return;
                            }
                        }
                    }

                    if (cacheActive && playerX == cacheX && playerY == cacheY) {
                        cacheUIOpen = true;
                    }
                }
            }
        }
    }

    if (segfault || gameWon) return;

    if (!justAttacked && playerCooldown > 0) {
        playerCooldown--;
    }

    if (activeForkTimer > 0) {
        int minDist = 9999;
        EnemyData* targetEnemy = nullptr;
        for (auto& enemy : enemies) {
            if (enemy.alive && enemy.processState != ProcessState::ZOMBIE && enemy.processState != ProcessState::TERMINATED && enemy.processState != ProcessState::WAITING) {
                int d = std::abs(activeForkX - enemy.x) + std::abs(activeForkY - enemy.y);
                if (d < minDist) {
                    minDist = d;
                    targetEnemy = &enemy;
                }
            }
        }

        if (targetEnemy) {
            if (minDist == 1) {
                int currentDamage = (overclockTimer > 0) ? (playerDamage * 2) : playerDamage;
                int dmg = std::max(0, currentDamage - targetEnemy->armor);
                targetEnemy->currentHP -= dmg;
                if (targetEnemy->currentHP <= 0) {
                    targetEnemy->processState = ProcessState::ZOMBIE;
                    targetEnemy->zombieTimer = 3;
                }
            } else {
                auto path = FindPath(activeForkX, activeForkY, targetEnemy->x, targetEnemy->y);
                if (!path.empty()) {
                    int nx = path.back().first;
                    int ny = path.back().second;
                    bool occupied = (nx == playerX && ny == playerY) || (cloneTimer > 0 && nx == cloneX && ny == cloneY);
                    for (const auto& e : enemies) {
                        if (e.alive && e.processState != ProcessState::TERMINATED && e.x == nx && e.y == ny) occupied = true;
                    }
                    if (!occupied) {
                        activeForkX = nx;
                        activeForkY = ny;
                    }
                }
            }
        }
    }

    int currentForkBombs = 0;
    for (const auto& e : enemies) {
        if (e.alive && e.type == EnemyType::FORK_BOMB && e.processState != ProcessState::TERMINATED) {
            currentForkBombs++;
        }
    }
    
    std::vector<EnemyData> newEnemies;

    for (auto& enemy : enemies) {
        if (!enemy.alive || !playerAlive) continue;

        if (enemy.processState == ProcessState::ZOMBIE) {
            enemy.zombieTimer--;
            if (enemy.zombieTimer <= 0) {
                enemy.processState = ProcessState::TERMINATED;
                enemy.alive = false;
            }
            continue;
        }
        
        if (enemy.processState == ProcessState::BLOCKED) {
            enemy.blockedTimer--;
            if (enemy.blockedTimer <= 0) {
                enemy.processState = ProcessState::READY;
            }
            continue; 
        }

        if (enemy.processState == ProcessState::WAITING) continue;

        enemy.turnCounter++;
        if (enemy.turnCounter % enemy.speed == 0) {
            enemy.processState = ProcessState::RUNNING; 

            if (enemy.type == EnemyType::FORK_BOMB) {
                if (currentForkBombs + newEnemies.size() < 40) {
                    int dx[] = {0, 0, -1, 1};
                    int dy[] = {-1, 1, 0, 0};
                    for(int i = 0; i < 4; ++i){
                        int nx = enemy.x + dx[i];
                        int ny = enemy.y + dy[i];
                        if (nx >= 0 && nx < 20 && ny >= 0 && ny < 20 && map[ny][nx] == '.') {
                            bool occupied = (nx == playerX && ny == playerY) || (cloneTimer > 0 && nx == cloneX && ny == cloneY) || (activeForkTimer > 0 && nx == activeForkX && ny == activeForkY);
                            for (const auto& e : enemies) {
                                if (e.alive && e.processState != ProcessState::TERMINATED && e.x == nx && e.y == ny) occupied = true;
                            }
                            for (const auto& ne : newEnemies) {
                                if (ne.x == nx && ne.y == ny) occupied = true;
                            }
                            if (!occupied) {
                                newEnemies.push_back({EnemyType::FORK_BOMB, nx, ny, 15, 15, 5, 0, 3, 0, true, EnemyState::IDLE, -1, -1, ProcessState::READY, 0, 0, "", ""});
                                break; 
                            }
                        }
                    }
                }
            }

            int distToPlayer = std::abs(playerX - enemy.x) + std::abs(playerY - enemy.y);
            int distToFork = 9999;
            if (activeForkTimer > 0) {
                distToFork = std::abs(activeForkX - enemy.x) + std::abs(activeForkY - enemy.y);
            }

            int targetX = playerX;
            int targetY = playerY;
            int dist = distToPlayer;
            bool targetingFork = false;

            if (activeForkTimer > 0 && distToFork < distToPlayer) {
                targetX = activeForkX;
                targetY = activeForkY;
                dist = distToFork;
                targetingFork = true;
            }
            
            switch (enemy.state) {
                case EnemyState::IDLE:
                    if (dist <= 5) enemy.state = EnemyState::CHASE;
                    break;
                case EnemyState::CHASE:
                    if (dist == 1) {
                        enemy.state = EnemyState::ATTACK;
                    } else if (dist > 5) {
                        enemy.state = EnemyState::SEARCH;
                        enemy.lastKnownPlayerX = targetX;
                        enemy.lastKnownPlayerY = targetY;
                    }
                    break;
                case EnemyState::ATTACK:
                    if (dist > 1) enemy.state = EnemyState::CHASE;
                    break;
                case EnemyState::SEARCH:
                    if (dist <= 5) {
                        enemy.state = EnemyState::CHASE;
                    } else if (enemy.x == enemy.lastKnownPlayerX && enemy.y == enemy.lastKnownPlayerY) {
                        enemy.state = EnemyState::IDLE;
                    }
                    break;
            }
            
            switch (enemy.state) {
                case EnemyState::IDLE:
                    break;
                    
                case EnemyState::ATTACK:
                    {
                        if (targetingFork) {
                            // Harmless distraction
                        } else if (firewallTimer > 0) {
                            // Blocked
                        } else {
                            int dmg = std::max(0, enemy.damage - playerArmor);
                            playerHP -= dmg;
                            if (playerHP <= 0) {
                                playerAlive = false;
                            }
                        }
                    }
                    break;
                    
                case EnemyState::CHASE:
                    {
                        auto path = FindPath(enemy.x, enemy.y, targetX, targetY);
                        if (!path.empty()) {
                            int nextX = path.back().first;
                            int nextY = path.back().second;

                            if (!(nextX == playerX && nextY == playerY) && !(cloneTimer > 0 && nextX == cloneX && nextY == cloneY) && !(activeForkTimer > 0 && nextX == activeForkX && nextY == activeForkY)) {
                                bool occupied = false;
                                for (const auto& other : enemies) {
                                    if (other.alive && other.processState != ProcessState::TERMINATED && other.x == nextX && other.y == nextY) {
                                        occupied = true;
                                        break;
                                    }
                                }
                                if (!occupied) {
                                    enemy.x = nextX;
                                    enemy.y = nextY;
                                }
                            }
                        }
                    }
                    break;
                    
                case EnemyState::SEARCH:
                    {
                        auto path = FindPath(enemy.x, enemy.y, enemy.lastKnownPlayerX, enemy.lastKnownPlayerY);
                        if (!path.empty()) {
                            int nextX = path.back().first;
                            int nextY = path.back().second;

                            if (!(nextX == playerX && nextY == playerY) && !(cloneTimer > 0 && nextX == cloneX && nextY == cloneY) && !(activeForkTimer > 0 && nextX == activeForkX && nextY == activeForkY)) {
                                bool occupied = false;
                                for (const auto& other : enemies) {
                                    if (other.alive && other.processState != ProcessState::TERMINATED && other.x == nextX && other.y == nextY) {
                                        occupied = true;
                                        break;
                                    }
                                }
                                if (!occupied) {
                                    enemy.x = nextX;
                                    enemy.y = nextY;
                                }
                            }
                        }
                    }
                    break;
            }
            
            if (enemy.processState == ProcessState::RUNNING) {
                enemy.processState = ProcessState::READY;
            }

            if (!playerAlive) break;
        }
    }

    for (const auto& ne : newEnemies) {
        enemies.push_back(ne);
    }

    if (deadlockActive && !deadlockResolved) {
        bool aWaiting = false;
        bool bWaiting = false;
        for (const auto& e : enemies) {
            if (e.alive && e.processState != ProcessState::TERMINATED && e.processState != ProcessState::ZOMBIE) {
                if (e.type == EnemyType::DEADLOCK_A && e.processState == ProcessState::WAITING) aWaiting = true;
                if (e.type == EnemyType::DEADLOCK_B && e.processState == ProcessState::WAITING) bWaiting = true;
            }
        }

        if (!aWaiting || !bWaiting) {
            deadlockActive = false;
            deadlockResolved = true;
            for (auto& e : enemies) {
                if ((e.type == EnemyType::DEADLOCK_A || e.type == EnemyType::DEADLOCK_B) &&
                    e.alive && e.processState != ProcessState::TERMINATED && e.processState != ProcessState::ZOMBIE) {
                    e.waitsForResource = "";
                    if (e.processState == ProcessState::WAITING) {
                        e.processState = ProcessState::READY;
                    }
                    if (currentFloor == 4) {
                        e.armor = 8;
                    } else if (currentFloor == 5) {
                        e.armor = 5;
                    } else {
                        e.armor = 2;
                    }
                }
            }
        }
    }

    if (boss1Active && !boss1Defeated) {
        bool forkAlive = false;
        for (const auto& e : enemies) {
            if (e.alive && e.type == EnemyType::FORK_BOMB && e.processState != ProcessState::TERMINATED && e.processState != ProcessState::ZOMBIE) {
                forkAlive = true;
                break;
            }
        }
        if (!forkAlive) {
            boss1Defeated = true;
            boss1Active = false;
            map[exitY][exitX] = '>'; 
            floorMsg = "[SYS] FORK BOMB DEFEATED. EXIT OPEN.";
            floorMsgTimer = 10;
        }
    }

    if (boss2Active && !boss2Defeated) {
        bool deadlockBossAlive = false;
        for (const auto& e : enemies) {
            if (e.alive && (e.type == EnemyType::DEADLOCK_A || e.type == EnemyType::DEADLOCK_B) && e.processState != ProcessState::TERMINATED && e.processState != ProcessState::ZOMBIE) {
                deadlockBossAlive = true;
                break;
            }
        }
        if (!deadlockBossAlive) {
            boss2Defeated = true;
            boss2Active = false;
            map[exitY][exitX] = '>'; 
            floorMsg = "[SYS] DEADLOCK BOSS DEFEATED. EXIT OPEN.";
            floorMsgTimer = 10;
        }
    }
}

bool GameState::IsRunning() const { return running; }
int GameState::GetPlayerX() const { return playerX; }
int GameState::GetPlayerY() const { return playerY; }
const std::vector<std::string>& GameState::GetMap() const { return map; }
bool GameState::IsPlayerAlive() const { return playerAlive; }
bool GameState::HasSegfaulted() const { return segfault; }
int GameState::GetPlayerHP() const { return playerHP; }
int GameState::GetPlayerCooldown() const { return playerCooldown; }
int GameState::GetPlayerUsedRAM() const { return playerUsedRAM; }
int GameState::GetPlayerMaxRAM() const { return playerMaxRAM; }
const std::vector<EnemyData>& GameState::GetEnemies() const { return enemies; }

int GameState::GetCloneTimer() const { return cloneTimer; }
int GameState::GetCloneX() const { return cloneX; }
int GameState::GetCloneY() const { return cloneY; }
int GameState::GetFirewallTimer() const { return firewallTimer; }
int GameState::GetOverclockTimer() const { return overclockTimer; }

int GameState::GetActiveForkTimer() const { return activeForkTimer; }
int GameState::GetActiveForkX() const { return activeForkX; }
int GameState::GetActiveForkY() const { return activeForkY; }

bool GameState::IsDeadlockActive() const { return deadlockActive; }
bool GameState::IsDeadlockResolved() const { return deadlockResolved; }