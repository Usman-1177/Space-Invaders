#include "C:\raylib\raylib\src\raylib.h"
#include <vector>
#include <algorithm>
#include <random>
#include <ctime>
#include <direct.h>
using namespace std;

const int screenWidth = 800;
const int screenHeight = 600;
const int playerSpeed = 5;
const int bulletSpeed = 7;
const float enemyDropDistance = 30.0f;
const float maxPlayerHealth = 100.0f;
const float maxBossHealth = 500.0f;

// Global texture variables
Texture2D playerTexture;
Texture2D enemyTexture;
Texture2D bossTexture;
Texture2D bulletTexture;
Texture2D enemyBulletTexture;

enum class EnemyType {
    STATIC,
    MOVING,
    BOSS
};

struct GameObject {
    Rectangle rect;
    bool active;
};

struct Player : GameObject {
    int lives;
    float health;
};

struct Bullet : GameObject {
    bool isEnemy;
};

struct Enemy : GameObject {
    EnemyType type;
    float speed;
    float timeSinceLastShot;
    float shootCooldown;
    float stopY;
    float health;  // For boss
    float moveDirection;  // For boss movement
};


enum class PowerUpType {
    RAPID_FIRE,
    SHIELD,
    SPREAD_SHOT
};

struct PowerUp : GameObject {
    PowerUpType type;
    float duration;
};

class Game {
private:
    Player player;
    vector<Enemy> enemies;
    vector<Bullet> bullets;
    int score;
    bool gameOver;
    bool bossSpawned;
    float timeSinceLastSpawn;
    float timeSinceLastShot;
    const float shotCooldown = 0.5f;
    const float spawnCooldown = 2.0f;
    const float bulletDamage = 20.0f;
    const float bossBulletDamage = 30.0f;
    mt19937 rng;

    void InitPlayer() {
        player.rect = { screenWidth / 2 - 25, screenHeight - 50, 50, 30 };
        player.active = true;
        player.lives = 3;
        player.health = maxPlayerHealth;
    }

    void SpawnBoss() {
        Enemy boss;
        boss.active = true;
        boss.rect = { screenWidth / 2 - 50, 50, 100, 60 };  // Bigger size for boss
        boss.type = EnemyType::BOSS;
        boss.speed = 3.0f;
        boss.moveDirection = 1.0f;  // Start moving right
        boss.health = maxBossHealth;
        boss.shootCooldown = 1.0f;  // Boss shoots more frequently
        boss.timeSinceLastShot = 0;
        enemies.push_back(boss);
        bossSpawned = true;
    }

    void SpawnEnemy() {
        if (score >= 100 && !bossSpawned) {
            SpawnBoss();
            return;
        }

        Enemy enemy;
        enemy.active = true;
        enemy.rect.width = 40;
        enemy.rect.height = 30;

        uniform_real_distribution<float> xDist(0, screenWidth - enemy.rect.width);
        enemy.rect.x = xDist(rng);
        enemy.rect.y = 0;

        uniform_int_distribution<int> typeDist(0, 1);
        enemy.type = static_cast<EnemyType>(typeDist(rng));

        uniform_real_distribution<float> stopDist(50.0f, screenHeight / 2 - 50.0f);
        enemy.stopY = stopDist(rng);

        if (enemy.type == EnemyType::MOVING) {
            uniform_real_distribution<float> speedDist(1.0f, 3.0f);
            enemy.speed = speedDist(rng);
        }
        else {
            uniform_real_distribution<float> speedDist(0.5f, 1.5f);
            enemy.speed = speedDist(rng);
        }

        uniform_real_distribution<float> cooldownDist(2.0f, 5.0f);
        enemy.shootCooldown = cooldownDist(rng);
        enemy.timeSinceLastShot = 0;

        enemies.push_back(enemy);
    }

    void EnemyShoot(const Enemy& enemy) {
        Bullet bullet;
        bullet.rect = {
            enemy.rect.x + enemy.rect.width / 2 - 2,
            enemy.rect.y + enemy.rect.height,
            4,
            10
        };
        bullet.active = true;
        bullet.isEnemy = true;
        bullets.push_back(bullet);

        // Boss shoots 3 bullets in spread pattern
        if (enemy.type == EnemyType::BOSS) {
            Bullet bulletLeft = bullet;
            bulletLeft.rect.x -= 20;
            bullets.push_back(bulletLeft);

            Bullet bulletRight = bullet;
            bulletRight.rect.x += 20;
            bullets.push_back(bulletRight);
        }
    }

public:
    Game() : rng(time(nullptr)) {
            // Load textures
            playerTexture = LoadTexture("assets/player.png");
            enemyTexture = LoadTexture("assets/enemy.png");
            bossTexture = LoadTexture("assets/boss.png");
            bulletTexture = LoadTexture("assets/bullet.png");
            enemyBulletTexture = LoadTexture("assets/enemy_bullet.png");

            InitPlayer();
            score = 0;
            gameOver = false;
            bossSpawned = false;
            timeSinceLastShot = 0;
            timeSinceLastSpawn = 0;
        }

    void Update() {
        if (gameOver) return;

        timeSinceLastShot += GetFrameTime();
        timeSinceLastSpawn += GetFrameTime();

        // Spawn new enemies
        if (timeSinceLastSpawn >= spawnCooldown && !bossSpawned) {
            SpawnEnemy();
            timeSinceLastSpawn = 0;
        }

        // Player movement with both WASD and arrow keys
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
            player.rect.x -= playerSpeed;
        }
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
            player.rect.x += playerSpeed;
        }
        if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
            player.rect.y -= playerSpeed;
        }
        if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
            player.rect.y += playerSpeed;
        }

        // Keep player within screen bounds
        player.rect.x = std::max(0.0f, std::min(player.rect.x, screenWidth - player.rect.width));
        player.rect.y = std::max(0.0f, std::min(player.rect.y, screenHeight - player.rect.height));

        // Player shooting
        if (IsKeyDown(KEY_SPACE) && timeSinceLastShot >= shotCooldown) {
            Bullet bullet;
            bullet.rect = {
                player.rect.x + player.rect.width / 2 - 2,
                player.rect.y,
                4,
                10
            };
            bullet.active = true;
            bullet.isEnemy = false;
            bullets.push_back(bullet);
            timeSinceLastShot = 0;
        }

        // Update bullets
        for (auto& bullet : bullets) {
            if (!bullet.active) continue;
            bullet.rect.y += bullet.isEnemy ? bulletSpeed : -bulletSpeed;

            if (bullet.rect.y < 0 || bullet.rect.y > screenHeight) {
                bullet.active = false;
            }
        }

        // Update enemies and enemy shooting
        for (auto& enemy : enemies) {
            if (!enemy.active) continue;

            enemy.timeSinceLastShot += GetFrameTime();

            // Enemy shooting
            if (enemy.timeSinceLastShot >= enemy.shootCooldown) {
                EnemyShoot(enemy);
                enemy.timeSinceLastShot = 0;
            }

            switch (enemy.type) {
            case EnemyType::MOVING:
                enemy.rect.y += enemy.speed;
                if (enemy.rect.y + enemy.rect.height >= player.rect.y) {
                    enemy.active = false;
                    player.health -= bulletDamage * 2;
                }
                break;

            case EnemyType::STATIC:
                if (enemy.rect.y < enemy.stopY) {
                    enemy.rect.y += enemy.speed;
                }
                break;

            case EnemyType::BOSS:
                // Boss moves side to side
                enemy.rect.x += enemy.speed * enemy.moveDirection;
                if (enemy.rect.x <= 0 || enemy.rect.x + enemy.rect.width >= screenWidth) {
                    enemy.moveDirection *= -1;  // Reverse direction
                }
                break;
            }
        }

        // Check collisions
        for (auto& bullet : bullets) {
            if (!bullet.active) continue;

            if (bullet.isEnemy) {
                // Enemy bullets hitting player
                if (CheckCollisionRecs(bullet.rect, player.rect)) {
                    bullet.active = false;
                    player.health -= (bossSpawned ? bossBulletDamage : bulletDamage);
                }
            }
            else {
                // Player bullets hitting enemies
                for (auto& enemy : enemies) {
                    if (!enemy.active) continue;
                    if (CheckCollisionRecs(bullet.rect, enemy.rect)) {
                        bullet.active = false;
                        if (enemy.type == EnemyType::BOSS) {
                            enemy.health -= bulletDamage;
                            if (enemy.health <= 0) {
                                enemy.active = false;
                                score += 500;  // Bonus points for killing boss
                            }
                        }
                        else {
                            enemy.active = false;
                            score += 10;  // 10 points per regular enemy
                        }
                    }
                }
            }
        }

        // Check player health
        if (player.health <= 0) {
            player.lives--;
            if (player.lives <= 0) {
                gameOver = true;
            }
            else {
                player.health = maxPlayerHealth;
            }
        }

        // Clean up inactive objects
        bullets.erase(
            remove_if(bullets.begin(), bullets.end(),
                [](const Bullet& b) { return !b.active; }),
            bullets.end());

        enemies.erase(
            remove_if(enemies.begin(), enemies.end(),
                [](const Enemy& e) { return !e.active; }),
            enemies.end());
    }
    ~Game() {
        UnloadTexture(playerTexture);
        UnloadTexture(enemyTexture);
        UnloadTexture(bossTexture);
        UnloadTexture(bulletTexture);
        UnloadTexture(enemyBulletTexture);
    }
    void Draw() {
        BeginDrawing();
        ClearBackground(BLACK);

        // Draw player
        if (player.active) {
            DrawTextureEx(playerTexture,
                { player.rect.x, player.rect.y },
                0.0f,
                player.rect.width / playerTexture.width,  // Scale to fit rect
                WHITE);

            // Draw health bar
            Rectangle healthBar = {
                player.rect.x,
                player.rect.y - 10,
                player.rect.width * (player.health / maxPlayerHealth),
                5
            };
            DrawRectangleRec(healthBar, RED);

            Rectangle healthBarBg = {
                player.rect.x,
                player.rect.y - 10,
                player.rect.width,
                5
            };
            DrawRectangleLinesEx(healthBarBg, 1, WHITE);
        }

        // Draw enemies
        for (const auto& enemy : enemies) {
            if (!enemy.active) continue;

            Color enemyColor;
            switch (enemy.type) {
            case EnemyType::STATIC:
                enemyColor = RED;
                break;
            case EnemyType::MOVING:
                enemyColor = ORANGE;
                break;
            case EnemyType::BOSS:
                enemyColor = PURPLE;
                // Draw boss health bar
                Rectangle bossHealthBar = {
                    enemy.rect.x,
                    enemy.rect.y - 10,
                    enemy.rect.width * (enemy.health / maxBossHealth),
                    5
                };
                DrawRectangleRec(bossHealthBar, RED);
                DrawRectangleLinesEx({ enemy.rect.x, enemy.rect.y - 10, enemy.rect.width, 5 }, 1, WHITE);
                break;
            }
            if (enemy.type == EnemyType::BOSS) {
                DrawTextureEx(bossTexture,
                    { enemy.rect.x, enemy.rect.y },
                    0.0f,
                    enemy.rect.width / bossTexture.width,
                    WHITE);
            }
            else {
                DrawTextureEx(enemyTexture,
                    { enemy.rect.x, enemy.rect.y },
                    0.0f,
                    enemy.rect.width / enemyTexture.width,
                    WHITE);
            }
        }

        // Draw bullets
        for (const auto& bullet : bullets) {
            if (bullet.active) {
                DrawTextureEx(bullet.isEnemy ? enemyBulletTexture : bulletTexture,
                    Vector2{ bullet.rect.x, bullet.rect.y },
                    0.0f,
                    bullet.rect.width / (bullet.isEnemy ? enemyBulletTexture.width : bulletTexture.width),
                    WHITE);
            }
        }

        // Draw HUD
        DrawText(TextFormat("Score: %d", score), 10, 10, 20, WHITE);
        DrawText(TextFormat("Lives: %d", player.lives), 10, 40, 20, WHITE);
        DrawText(TextFormat("Health: %.0f%%", player.health), 10, 70, 20, WHITE);

        if (gameOver) {
            const char* text = "Game Over!";
            DrawText(text,
                screenWidth / 2 - MeasureText(text, 40) / 2,
                screenHeight / 2 - 20,
                40,
                WHITE);
        }

        EndDrawing();
    }
};

int main() {
    InitWindow(screenWidth, screenHeight, "Space Invaders");
    // Add this after InitWindow() in main()
    if (!DirectoryExists("assets")) {
        _mkdir("assets");

        Image placeholder;

        // Create player placeholder
        placeholder = GenImageColor(50, 30, GREEN);
        ExportImage(placeholder, "assets/player.png");

        // Create enemy placeholder
        placeholder = GenImageColor(40, 30, RED);
        ExportImage(placeholder, "assets/enemy.png");

        // Create boss placeholder
        placeholder = GenImageColor(100, 60, PURPLE);
        ExportImage(placeholder, "assets/boss.png");

        // Create bullet placeholder
        placeholder = GenImageColor(4, 10, WHITE);
        ExportImage(placeholder, "assets/bullet.png");

        // Create enemy bullet placeholder
        placeholder = GenImageColor(4, 10, RED);
        ExportImage(placeholder, "assets/enemy_bullet.png");

        UnloadImage(placeholder);
    }
    SetTargetFPS(60);

    Game game;

    while (!WindowShouldClose()) {
        game.Update();
        game.Draw();
    }

    CloseWindow();
    return 0;
}