#pragma once
#include <vector>
#include <string>
#include <random>
#include <utility>
#include "Input.h"

enum class EnemyState {
    IDLE,
    CHASE,
    ATTACK,
    SEARCH
};

enum class ProcessState {
    RUNNING,
    READY,
    WAITING,
    BLOCKED,
    ZOMBIE,
    TERMINATED
};

enum class EnemyType { WORKER, DAEMON, ZOMBIE, DEADLOCK_A, DEADLOCK_B, FORK_BOMB };

struct EnemyData {
    EnemyType type;
    int x, y, maxHP, currentHP, damage, armor, speed, turnCounter;
    bool alive;
    EnemyState state;
    int lastKnownPlayerX;
    int lastKnownPlayerY;
    
    ProcessState processState;
    int blockedTimer;
    int zombieTimer;

    std::string holdsResource;
    std::string waitsForResource;
};

struct Room {
    int x, y, w, h;
    int centerX() const { return x + w / 2; }
    int centerY() const { return y + h / 2; }
};

class GameState {
public:
    GameState(unsigned int seed);
    void Update(Command cmd);
    bool IsRunning() const;
    int GetPlayerX() const;
    int GetPlayerY() const;
    const std::vector<std::string>& GetMap() const;
    
    bool IsPlayerAlive() const;
    bool HasSegfaulted() const;
    int GetPlayerHP() const;
    int GetPlayerCooldown() const;
    int GetPlayerUsedRAM() const;
    int GetPlayerMaxRAM() const;
    const std::vector<EnemyData>& GetEnemies() const;

    int GetCloneTimer() const;
    int GetCloneX() const;
    int GetCloneY() const;
    int GetFirewallTimer() const;
    int GetOverclockTimer() const;
    
    int GetActiveForkTimer() const;
    int GetActiveForkX() const;
    int GetActiveForkY() const;

    bool IsDeadlockActive() const;
    bool IsDeadlockResolved() const;

    bool IsCacheActive() const { return cacheActive; }
    bool IsCacheUIOpen() const { return cacheUIOpen; }
    int GetCacheX() const { return cacheX; }
    int GetCacheY() const { return cacheY; }
    std::string GetCacheMsg() const { return cacheMsg; }
    int GetCacheMsgTimer() const { return cacheMsgTimer; }

    std::string GetFloorName() const;
    int GetCurrentFloor() const { return currentFloor + 1; }
    int GetTotalFloors() const { return totalFloors; }
    bool IsGameWon() const { return gameWon; }
    unsigned int GetSeed() const { return baseSeed; }
    std::string GetFloorMsg() const { return floorMsg; }
    int GetFloorMsgTimer() const { return floorMsgTimer; }

    bool IsBoss1Active() const { return boss1Active; }
    bool IsBoss2Active() const { return boss2Active; }
    bool IsBoss3Active() const { return boss3Active; }

    // Phase 9: Persistence
    bool SaveToFile(const std::string& filename) const;
    bool LoadFromFile(const std::string& filename);
    void SetSaveFilename(const std::string& file) { saveFilename = file; }

private:
    std::vector<std::string> map;
    int playerX;
    int playerY;
    int playerHP;
    int playerDamage;
    int playerArmor;
    int playerCooldown;
    int playerUsedRAM;
    int playerMaxRAM;
    bool playerAlive;
    bool segfault;
    bool running;
    
    int cloneTimer;
    int cloneX;
    int cloneY;
    int firewallTimer;
    int overclockTimer;
    
    int activeForkTimer;
    int activeForkX;
    int activeForkY;

    bool deadlockActive;
    bool deadlockResolved;
    
    int cacheX;
    int cacheY;
    bool cacheActive;
    bool cacheUIOpen;
    int playerAttackRamCost;
    std::string cacheMsg;
    int cacheMsgTimer;

    unsigned int baseSeed;
    int currentFloor;
    int totalFloors;
    int exitX;
    int exitY;
    bool gameWon;
    std::string floorMsg;
    int floorMsgTimer;

    bool boss1Active;
    bool boss1Defeated;
    bool boss2Active;
    bool boss2Defeated;
    bool boss3Active;
    bool boss3Defeated;

    std::string saveFilename;

    std::vector<EnemyData> enemies;

    void GenerateDungeon(unsigned int seed);
    bool RoomsIntersect(const Room& a, const Room& b) const;
    std::vector<std::pair<int, int>> FindPath(int startX, int startY, int goalX, int goalY) const;
    
    void AllocateRAM(int amount);
    void FreeRAM(int amount);
};