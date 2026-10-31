
#include <windows.h>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstring>

using namespace std;

const int SCREEN_W = 900;
const int SCREEN_H = 600;
const int WORLD_W = 2300;

const int PLAYER_W = 38;
const int PLAYER_H = 48;

const int EXIT_X = 2140;
const int BOSS_Y = 485;

HWND hwnd;
HDC backDC;
HBITMAP backBitmap;
HBITMAP oldBitmap;

enum GameMode {
    MENU,
    INSTRUCTIONS,
    PLAYING,
    LEVEL_COMPLETE,
    GAME_OVER,
    WIN
};

GameMode mode = MENU;

int playerX = 80;
int playerY = 450;
int velocityY = 0;
int previousBottom = 0;

int cameraX = 0;
int score = 0;
int lives = 3;
int stage = 0;

int powerTimer = 0;
int powerType = 0;

int transitionTimer = 0;

int bossX = 1850;
int bossHealth = 3;
int bossDirection = 1;
int bossCooldown = 0;

bool onGround = false;
bool checkpointReached = false;
bool bossAlive = true;

struct Platform {
    int x, y, w, h;
};

struct Coin {
    int x, y;
    bool collected;
};

struct Enemy {
    int x, y;
    int left, right;
    int direction;
    bool alive;
};

vector<Platform> platforms;
vector<Coin> coins;
vector<Enemy> enemies;

bool Collision(int x1, int y1, int w1, int h1,
               int x2, int y2, int w2, int h2) {
    return x1 < x2 + w2 &&
           x1 + w1 > x2 &&
           y1 < y2 + h2 &&
           y1 + h1 > y2;
}

void AddPlatform(int x, int y, int w, int h) {
    platforms.push_back({x, y, w, h});
}

void AddCoin(int x, int y) {
    coins.push_back({x, y, false});
}

void AddEnemy(int x, int left, int right) {
    enemies.push_back({x, 510, left, right, 1, true});
}

void LoadStage() {
    platforms.clear();
    coins.clear();
    enemies.clear();

    playerX = 80;
    playerY = 450;
    velocityY = 0;
    cameraX = 0;

    onGround = false;
    checkpointReached = false;

    powerTimer = 0;
    powerType = 0;

    bossAlive = (stage == 2);
    bossHealth = 3;
    bossX = 1850;
    bossDirection = 1;
    bossCooldown = 0;

    AddPlatform(0, 550, WORLD_W, 50);

    if (stage == 0) {
        AddPlatform(300, 450, 150, 20);
        AddPlatform(600, 390, 150, 20);
        AddPlatform(950, 450, 160, 20);
        AddPlatform(1300, 390, 150, 20);
        AddPlatform(1650, 450, 180, 20);

        AddEnemy(500, 450, 750);
        AddEnemy(1150, 1000, 1400);
        AddEnemy(1750, 1650, 1900);
    }
    else if (stage == 1) {
        AddPlatform(250, 430, 150, 20);
        AddPlatform(550, 360, 170, 20);
        AddPlatform(900, 430, 180, 20);
        AddPlatform(1250, 350, 160, 20);
        AddPlatform(1650, 420, 180, 20);

        AddEnemy(450, 350, 650);
        AddEnemy(1000, 900, 1200);
        AddEnemy(1750, 1600, 1900);
    }
    else {
        AddPlatform(280, 420, 150, 20);
        AddPlatform(600, 350, 150, 20);
        AddPlatform(950, 420, 180, 20);
        AddPlatform(1300, 350, 160, 20);
        AddPlatform(1700, 420, 180, 20);

        AddEnemy(500, 400, 700);
        AddEnemy(1150, 1000, 1350);
    }

    for (int i = 0; i < 20; i++) {
        AddCoin(180 + i * 95, 480 - (i % 3) * 65);
    }
}

void StartGame() {
    score = 0;
    lives = 3;
    stage = 0;
    transitionTimer = 0;

    LoadStage();
    mode = PLAYING;
}

void NextStage() {
    stage++;

    if (stage >= 3) {
        mode = WIN;
        return;
    }

    LoadStage();
    mode = PLAYING;
}

void CompleteStage() {
    if (mode != PLAYING) return;

    mode = LEVEL_COMPLETE;
    transitionTimer = 100;
}

void LoseLife() {
    if (powerType == 2 && powerTimer > 0) {
        return;
    }

    lives--;

    if (lives <= 0) {
        mode = GAME_OVER;
        return;
    }

    playerX = checkpointReached ? 1100 : 80;
    playerY = 450;
    velocityY = 0;
    onGround = false;
}

void DrawRectangle(HDC dc, int x, int y, int w, int h,
                   COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    RECT r = {x, y, x + w, y + h};
    FillRect(dc, &r, brush);
    DeleteObject(brush);
}

void DrawCircle(HDC dc, int x, int y, int r, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    HBRUSH oldBrush = (HBRUSH)SelectObject(dc, brush);

    Ellipse(dc, x - r, y - r, x + r, y + r);

    SelectObject(dc, oldBrush);
    DeleteObject(brush);
}

void DrawTextCenter(HDC dc, const char* text, int y,
                    int size, COLORREF color) {
    SetTextColor(dc, color);
    SetBkMode(dc, TRANSPARENT);

    HFONT font = CreateFontA(
        size, 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY,
        DEFAULT_PITCH,
        "Arial"
    );

    HFONT oldFont = (HFONT)SelectObject(dc, font);

    RECT r = {0, y, SCREEN_W, y + 60};
    DrawTextA(dc, text, -1, &r, DT_CENTER | DT_SINGLELINE);

    SelectObject(dc, oldFont);
    DeleteObject(font);
}

void DrawPlayer(HDC dc) {
    int x = playerX - cameraX;

    DrawRectangle(dc, x + 5, playerY, 28, 20,
                  RGB(220, 40, 40));

    DrawRectangle(dc, x + 3, playerY + 18, 32, 22,
                  RGB(30, 80, 220));

    DrawRectangle(dc, x + 7, playerY + 40, 10, 8,
                  RGB(80, 40, 20));

    DrawRectangle(dc, x + 23, playerY + 40, 10, 8,
                  RGB(80, 40, 20));

    DrawCircle(dc, x + 19, playerY + 9, 3,
               RGB(255, 255, 255));

    if (powerType == 2 && powerTimer > 0) {
        DrawCircle(dc, x + 19, playerY + 24, 30,
                   RGB(100, 220, 255));
    }
}

void DrawGame(HDC dc) {
    COLORREF sky;
    COLORREF ground;

    if (stage == 0) {
        sky = RGB(120, 210, 255);
        ground = RGB(30, 150, 60);
    }
    else if (stage == 1) {
        sky = RGB(255, 190, 100);
        ground = RGB(205, 140, 55);
    }
    else {
        sky = RGB(130, 190, 255);
        ground = RGB(180, 230, 255);
    }

    DrawRectangle(dc, 0, 0, SCREEN_W, SCREEN_H, sky);

    if (stage == 0) {
        for (int i = 0; i < SCREEN_W; i += 180) {
            DrawCircle(dc, i + 40, 440, 35,
                       RGB(30, 130, 40));

            DrawRectangle(dc, i + 32, 440, 16, 80,
                          RGB(110, 65, 30));
        }
    }
    else if (stage == 1) {
        DrawCircle(dc, 760, 100, 45, RGB(255, 220, 40));
    }
    else {
        for (int i = 0; i < SCREEN_W; i += 150) {
            DrawCircle(dc, i + 50, 500, 35,
                       RGB(230, 250, 255));
        }
    }

    DrawRectangle(dc, 0, 550, SCREEN_W, 50, ground);

    for (auto p : platforms) {
        int x = p.x - cameraX;

        if (x + p.w > 0 && x < SCREEN_W) {
            DrawRectangle(dc, x, p.y, p.w, p.h,
                          RGB(100, 65, 35));

            DrawRectangle(dc, x, p.y, p.w, 6,
                          RGB(40, 180, 60));
        }
    }

    for (auto c : coins) {
        if (!c.collected) {
            int x = c.x - cameraX;

            if (x > -30 && x < SCREEN_W + 30) {
                DrawCircle(dc, x, c.y, 10,
                           RGB(255, 215, 0));
            }
        }
    }

    for (auto e : enemies) {
        if (e.alive) {
            int x = e.x - cameraX;

            DrawCircle(dc, x + 15, e.y + 15, 17,
                       RGB(180, 40, 40));

            DrawCircle(dc, x + 9, e.y + 10, 3,
                       RGB(255, 255, 255));

            DrawCircle(dc, x + 21, e.y + 10, 3,
                       RGB(255, 255, 255));
        }
    }

    if (stage == 2 && bossAlive) {
        int x = bossX - cameraX;

        DrawRectangle(dc, x, BOSS_Y, 72, 82,
                      RGB(100, 20, 150));

        DrawCircle(dc, x + 20, 510, 7,
                   RGB(255, 255, 255));

        DrawCircle(dc, x + 52, 510, 7,
                   RGB(255, 255, 255));

        DrawRectangle(dc, x + 10, 470, 52, 12,
                      RGB(70, 10, 100));

        stringstream ss;
        ss << "BOSS HP: " << bossHealth;
        string s = ss.str();

        SetTextColor(dc, RGB(255, 255, 255));
        TextOutA(dc, x, 455, s.c_str(), (int)s.size());
    }

    int exitScreenX = EXIT_X - cameraX;

    DrawRectangle(dc, exitScreenX, 490, 18, 60,
                  RGB(255, 255, 255));

    DrawRectangle(dc, exitScreenX + 18, 490, 45, 25,
                  RGB(255, 40, 40));

    DrawPlayer(dc);

    stringstream ss;
    ss << "Score: " << score
       << "    Lives: " << lives
       << "    World: " << stage + 1;

    string hud = ss.str();

    SetTextColor(dc, RGB(0, 0, 0));
    SetBkMode(dc, TRANSPARENT);
    TextOutA(dc, 15, 15, hud.c_str(), (int)hud.size());

    if (powerTimer > 0) {
        const char* powerName = "POWER ACTIVE";

        if (powerType == 1) powerName = "SPEED POWER";
        if (powerType == 2) powerName = "SHIELD POWER";
        if (powerType == 3) powerName = "SUPER JUMP";

        TextOutA(dc, 15, 40, powerName,
                 (int)strlen(powerName));
    }

    if (checkpointReached) {
        TextOutA(dc, 700, 15, "CHECKPOINT SAVED", 16);
    }

    if (stage == 2 && bossAlive) {
        TextOutA(dc, 300, 15,
                 "DEFEAT THE BOSS FIRST", 21);
    }
}

void UpdateGame() {
    if (mode == LEVEL_COMPLETE) {
        transitionTimer--;

        if (transitionTimer <= 0) {
            NextStage();
        }

        return;
    }

    if (mode != PLAYING) return;

    int speed = 5;

    if (powerType == 1 && powerTimer > 0) {
        speed = 9;
    }

    if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
        playerX -= speed;
    }

    if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
        playerX += speed;
    }

    if (playerX < 0) playerX = 0;

    if (playerX > WORLD_W - PLAYER_W) {
        playerX = WORLD_W - PLAYER_W;
    }

    if ((GetAsyncKeyState(VK_SPACE) & 0x8000 ||
         GetAsyncKeyState(VK_UP) & 0x8000) && onGround) {

        velocityY = -16;

        if (powerType == 3 && powerTimer > 0) {
            velocityY = -22;
        }

        onGround = false;
    }

    previousBottom = playerY + PLAYER_H;

    velocityY += 1;

    if (velocityY > 15) velocityY = 15;

    playerY += velocityY;
    onGround = false;

    for (auto p : platforms) {
        if (Collision(playerX, playerY, PLAYER_W, PLAYER_H,
                      p.x, p.y, p.w, p.h)) {

            if (velocityY > 0) {
                playerY = p.y - PLAYER_H;
                velocityY = 0;
                onGround = true;
            }
            else if (velocityY < 0) {
                playerY = p.y + p.h;
                velocityY = 0;
            }
        }
    }

    if (playerY > SCREEN_H) {
        LoseLife();
        return;
    }

    for (auto& c : coins) {
        if (!c.collected &&
            Collision(playerX, playerY, PLAYER_W, PLAYER_H,
                      c.x - 10, c.y - 10, 20, 20)) {

            c.collected = true;
            score += 10;
        }
    }

    for (auto& e : enemies) {
        if (!e.alive) continue;

        e.x += e.direction * 2;

        if (e.x < e.left || e.x > e.right) {
            e.direction *= -1;
        }

        if (Collision(playerX, playerY, PLAYER_W, PLAYER_H,
                      e.x, e.y, 30, 30)) {

            if (velocityY > 0 &&
                previousBottom <= e.y + 20) {

                e.alive = false;
                velocityY = -10;
                score += 20;
            }
            else {
                LoseLife();
                return;
            }
        }
    }

    if (playerX > 1050 && !checkpointReached) {
        checkpointReached = true;
        score += 50;
    }

    if (stage == 2 && bossAlive) {
        if (bossCooldown > 0) {
            bossCooldown--;
        }

        bossX += bossDirection * 2;

        if (bossX < 1800 || bossX > 1980) {
            bossDirection *= -1;
        }

        bool touchingBoss =
            Collision(playerX, playerY, PLAYER_W, PLAYER_H,
                      bossX, BOSS_Y, 72, 82);

        bool stompingBoss =
            velocityY > 0 &&
            previousBottom <= BOSS_Y + 20;

        if (touchingBoss && bossCooldown == 0) {
            if (stompingBoss) {
                bossHealth--;
                velocityY = -14;
                score += 50;
                bossCooldown = 35;

                if (bossHealth <= 0) {
                    bossAlive = false;
                    score += 200;
                }
            }
            else {
                LoseLife();
                return;
            }
        }
    }

    if (powerTimer > 0) {
        powerTimer--;
    }
    else {
        powerType = 0;
    }

    cameraX = playerX - SCREEN_W / 2;

    if (cameraX < 0) cameraX = 0;

    if (cameraX > WORLD_W - SCREEN_W) {
        cameraX = WORLD_W - SCREEN_W;
    }

    // FINISH CHECK: reach the exit area.
    // World 1 and 2 finish immediately.
    // World 3 finishes only after defeating the boss.

    if (playerX + PLAYER_W >= EXIT_X) {
        if (stage != 2 || !bossAlive) {
            CompleteStage();
        }
    }
}

void DrawMenu(HDC dc) {
    DrawRectangle(dc, 0, 0, SCREEN_W, SCREEN_H,
                  RGB(80, 160, 240));

    DrawTextCenter(dc, "MARIO ADVENTURE", 120, 48,
                   RGB(255, 255, 255));

    DrawTextCenter(dc, "THE THREE WORLDS", 190, 25,
                   RGB(255, 240, 0));

    DrawTextCenter(dc, "Press ENTER to Start", 300, 25,
                   RGB(255, 255, 255));

    DrawTextCenter(dc, "Press I for Instructions", 350, 20,
                   RGB(255, 255, 255));

    DrawTextCenter(dc, "C++ Game Project", 480, 18,
                   RGB(255, 255, 255));
}

void DrawInstructions(HDC dc) {
    DrawRectangle(dc, 0, 0, SCREEN_W, SCREEN_H,
                  RGB(40, 80, 130));

    DrawTextCenter(dc, "HOW TO PLAY", 80, 38,
                   RGB(255, 255, 255));

    DrawTextCenter(dc, "LEFT / RIGHT ARROW : Move", 160, 21,
                   RGB(255, 255, 255));

    DrawTextCenter(dc, "SPACE / UP ARROW : Jump", 205, 21,
                   RGB(255, 255, 255));

    DrawTextCenter(dc, "Collect coins to increase your score", 250, 19,
                   RGB(255, 255, 255));

    DrawTextCenter(dc, "Reach the finish flag to change worlds", 295, 19,
                   RGB(255, 255, 255));

    DrawTextCenter(dc, "Defeat the boss in World 3", 340, 19,
                   RGB(255, 255, 255));

    DrawTextCenter(dc, "Press ENTER to Start", 430, 23,
                   RGB(255, 240, 0));

    DrawTextCenter(dc, "Press ESC to Return", 480, 18,
                   RGB(255, 255, 255));
}

void Paint(HDC dc) {
    if (mode == MENU) {
        DrawMenu(dc);
    }
    else if (mode == INSTRUCTIONS) {
        DrawInstructions(dc);
    }
    else if (mode == PLAYING) {
        DrawGame(dc);
    }
    else if (mode == LEVEL_COMPLETE) {
        DrawRectangle(dc, 0, 0, SCREEN_W, SCREEN_H,
                      RGB(20, 120, 70));

        stringstream ss;
        ss << "WORLD " << stage + 1 << " COMPLETED!";

        string message = ss.str();

        DrawTextCenter(dc, message.c_str(), 180, 42,
                       RGB(255, 255, 255));

        DrawTextCenter(dc, "Get Ready for the Next World!", 270, 25,
                       RGB(255, 240, 0));
    }
    else if (mode == GAME_OVER) {
        DrawRectangle(dc, 0, 0, SCREEN_W, SCREEN_H,
                      RGB(80, 20, 20));

        DrawTextCenter(dc, "GAME OVER", 180, 50,
                       RGB(255, 255, 255));

        DrawTextCenter(dc, "Press R to Restart", 300, 25,
                       RGB(255, 240, 0));

        DrawTextCenter(dc, "Press ESC for Menu", 350, 20,
                       RGB(255, 255, 255));
    }
    else if (mode == WIN) {
        DrawRectangle(dc, 0, 0, SCREEN_W, SCREEN_H,
                      RGB(20, 130, 70));

        DrawTextCenter(dc, "YOU WIN!", 150, 55,
                       RGB(255, 255, 255));

        stringstream ss;
        ss << "Final Score: " << score;

        string result = ss.str();

        DrawTextCenter(dc, result.c_str(), 250, 28,
                       RGB(255, 240, 0));

        DrawTextCenter(dc, "You Completed All Three Worlds!", 320, 24,
                       RGB(255, 255, 255));

        DrawTextCenter(dc, "Press R to Play Again", 390, 23,
                       RGB(255, 255, 255));

        DrawTextCenter(dc, "Press ESC for Menu", 440, 20,
                       RGB(255, 255, 255));
    }
}

void CreateBuffer(HWND window) {
    HDC dc = GetDC(window);

    backDC = CreateCompatibleDC(dc);
    backBitmap = CreateCompatibleBitmap(dc, SCREEN_W, SCREEN_H);

    oldBitmap = (HBITMAP)SelectObject(backDC, backBitmap);

    ReleaseDC(window, dc);
}

void DeleteBuffer() {
    if (backDC) {
        SelectObject(backDC, oldBitmap);
        DeleteObject(backBitmap);
        DeleteDC(backDC);
    }
}

LRESULT CALLBACK WindowProcedure(HWND window, UINT message,
                                 WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        CreateBuffer(window);
        SetTimer(window, 1, 16, NULL);
        return 0;

    case WM_TIMER:
        UpdateGame();
        InvalidateRect(window, NULL, FALSE);
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_RETURN) {
            if (mode == MENU || mode == INSTRUCTIONS) {
                StartGame();
            }
        }

        if (wParam == 'I' && mode == MENU) {
            mode = INSTRUCTIONS;
        }

        if (wParam == 'R') {
            StartGame();
        }

        if (wParam == VK_ESCAPE) {
            mode = MENU;
        }

        if (mode == PLAYING) {
            if (wParam == 'S') {
                powerType = 1;
                powerTimer = 300;
            }

            if (wParam == 'H') {
                powerType = 2;
                powerTimer = 300;
            }

            if (wParam == 'J') {
                powerType = 3;
                powerTimer = 300;
            }
        }

        InvalidateRect(window, NULL, FALSE);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(window, &ps);

        Paint(backDC);

        BitBlt(dc, 0, 0, SCREEN_W, SCREEN_H,
               backDC, 0, 0, SRCCOPY);

        EndPaint(window, &ps);
        return 0;
    }

    case WM_DESTROY:
        KillTimer(window, 1);
        DeleteBuffer();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(window, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous,
                   LPSTR commandLine, int showCommand) {
    const char CLASS_NAME[] = "MarioAdventureWindow";

    WNDCLASSA wc = {};

    wc.lpfnWndProc = WindowProcedure;
    wc.hInstance = instance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassA(&wc);

    hwnd = CreateWindowExA(
        0,
        CLASS_NAME,
        "Mario Adventure - The Three Worlds",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        SCREEN_W, SCREEN_H,
        NULL, NULL, instance, NULL
    );

    if (!hwnd) return 0;

    ShowWindow(hwnd, showCommand);
    UpdateWindow(hwnd);

    MSG msg = {};

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
