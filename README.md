# Car Game

> Simple 3D Car Game implemented in C++ (Computer Graphics course project)

---
## 👨‍💻 Authors & Contributions

**SOC Team Lead:**  
👤 **Ahmed Emad Eldeen Abdelmoneam**

<table>
  <tr>
    <td>
      <ul>
        <li>🔗 <b>LinkedIn:</b> <a href="https://www.linkedin.com/in/0x3omda/">linkedin.com/in/0x3omda</a></li>
        <li>🌐 <b>Portfolio:</b> <a href="https://eng-ahmed-emad.github.io/AhmedEmad-Dev/">Portfolio</a></li>
      </ul>
    </td>
    <td><img align="right" height="153" width="159" src="gif/anime-frieren.gif" /></td>
    <td><img align="right" height="153" width="159" src="gif/giphy.gif" /></td>
  </tr>
</table>

## Table of Contents

* [Project Overview](#project-overview)
* [Features](#features)
* [Requirements](#requirements)
* [Build & Run](#build--run)
* [Controls](#controls)
* [Assets](#assets)
* [Folder Structure](#folder-structure)
* [Contributing](#contributing)
* [Author](#author)
* [License](#license)

---

## Project Overview

**Car Game** is a lightweight 2D car game written in C++ as a computer graphics / game programming project. The game demonstrates basic rendering, simple physics (movement), collision detection, and input handling. It is suitable as a learning project to practice rendering pipelines and basic game loop design.

## Features

* Player-controlled car (left/right movement, acceleration)
* Moving road and enemy/obstacle cars
* Collision detection and game over handling
* Simple scoring system
* Configurable difficulty (speed increase over time)

## Requirements

* C++ compiler supporting C++11 or higher (g++, clang++, MSVC)
* One of the following graphics/input libraries (depending on the project's implementation):

  * OpenGL + GLUT/FreeGLUT or GLFW
  * SDL2
  * SFML
* CMake (optional but recommended) or an IDE (Visual Studio) / Makefile

> If you used a specific library (e.g. SFML or SDL2), replace the generic instructions above with the exact dependency and version.

## Build & Run

### Using CMake (recommended)

```bash
git clone https://github.com/Eng-Ahmed-Emad/Car-Game.git
cd Car-Game
mkdir -p build && cd build
cmake ..
cmake --build .
# then run the executable (name depends on your CMakeLists)
./CarGame
```

### Using Makefile

```bash
make
./CarGame
```

### Using Visual Studio (Windows)

* Open the provided Visual Studio solution/project (if exists) or generate a VS solution using CMake.
* Build the solution and run the generated executable.

> Note: Update the instructions above to match your actual build system and executable name if they differ.

## Controls

* Left / Right arrows or A / D — steer the car
* Up arrow or W — accelerate
* Down arrow or S — brake / slow down
* ESC — exit the game

## Assets

All game assets (images, sprites, sound files) are placed in the `assets/` folder. If your project requires runtime current working directory to be the project root, make sure to run the executable from the project root or update asset paths accordingly.

## Folder Structure

```
Car-Game/
├── src/            # C++ source files
├── include/        # Header files
├── assets/         # Images, sprites, audio
├── build/          # Build output (ignored in git)
├── CMakeLists.txt
├── Makefile
└── README.md
```

Adjust the structure above to match the actual layout in your repo.

## Contributing

Contributions are welcome. To contribute:

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/my-feature`
3. Commit your changes and push: `git push origin feature/my-feature`
4. Open a pull request describing your changes

Please add clear commit messages and keep changes focused.

## Author

## 👨‍💻 Authors & Contributions

**SOC Team Lead:**  
👤 **Ahmed Emad Eldeen Abdelmoneam**

<table>
  <tr>
    <td>
      <ul>
        <li>🔗 <b>LinkedIn:</b> <a href="https://www.linkedin.com/in/0x3omda/">linkedin.com/in/0x3omda</a></li>
        <li>🌐 <b>Portfolio:</b> <a href="https://eng-ahmed-emad.github.io/AhmedEmad-Dev/">Portfolio</a></li>
      </ul>
    </td>
    <td><img align="right" height="153" width="159" src="gif/anime-frieren.gif" /></td>
    <td><img align="right" height="153" width="159" src="gif/giphy.gif" /></td>
  </tr>
</table>

## License

Add a license file (e.g. `LICENSE`) or choose a license such as MIT. Example badge:

`MIT License`

---

*If you want, I can update this README to include exact build commands or the specific graphics library you used (OpenGL/SDL2/SFML), add screenshots, or include a GIF — just tell me what to add.*
