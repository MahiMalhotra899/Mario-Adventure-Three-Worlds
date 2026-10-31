 Mario Adventure – The Three Worlds

 Project Overview

Mario Adventure – The Three Worlds is a 2D graphical adventure game developed using C++. The player explores three different worlds, collects coins, avoids enemies, uses power-ups, and fights a final boss to complete the game.

This project is developed as a BCA mini project using C++ and the Windows API.

 Features

- Three different game worlds:
  - Forest World
  - Desert World
  - Ice Kingdom
- Player movement and jumping
- Coin collection and score system
- Three lives system
- Moving enemies and collision detection
- Checkpoint system
- Power-ups:
  - Speed
  - Shield
  - Super Jump
- Final boss battle
- Game over and victory screens
- Restart option

 Technologies Used

- C++
- Windows API
- Windows GDI
- Visual Studio Code
- MinGW-w64 Compiler

 Game Controls

| Key | Action |
|---|---|
| Enter | Start the game |
| I | Open instructions |
| Left Arrow | Move left |
| Right Arrow | Move right |
| Space / Up Arrow | Jump |
| R | Restart the game |
| Esc | Return to main menu |

 How to Compile and Run

 Requirements

- Windows 10 or Windows 11
- Visual Studio Code
- MinGW-w64 C++ compiler

 Step 1: Open the Project

Download or clone this repository and open the project folder in Visual Studio Code.

 Step 2: Open Terminal

In Visual Studio Code, select Terminal → New Terminal.

Step 3: Compile the Code

Run the following command:

```bash
g++ main.cpp -o MarioAdventure.exe -mwindows -lgdi32 -luser32
```

 Step 4: Run the Game

Run this command:

```powershell
.\MarioAdventure.exe
```

The graphical game window will open.

"Note:" The compiler must be installed and configured in the Windows PATH environment variable.

 Project Files

- `main.cpp` – Complete C++ source code.
- `Mario_Adventure_Report.pdf` – Project documentation.
- `Mario_UML_Diagram.jpg` – Hand-drawn UML diagram.
- `README.md` – Project description and execution instructions.

 Project Purpose

The purpose of this project is to develop an interactive 2D game using C++ programming and understand programming concepts such as functions, structures, conditional statements, loops, and event handling.

 Author

BCA Mini Project