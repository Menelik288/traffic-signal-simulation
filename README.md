# 🚦 Traffic Signal Simulation

A computer graphics project that simulates a road intersection controlled by a traffic light. Cars drive along a two-lane road, slow down and queue at the stop line on red, react to yellow like real drivers, and move off again on green.

The project comes in two versions:

| Version | Technology | Folder |
|---|---|---|
| **Web** (runs in any browser) | WebGL (OpenGL ES) + JavaScript | [`index.html`](index.html) |
| **Desktop** | C++ with OpenGL and GLUT / FreeGLUT | [`opengl-cpp/`](opengl-cpp/) |

**Live demo:** _add your Vercel link here after deploying_

---

## ✨ Features

- **Traffic light cycle**: Green → Yellow → Red, with a countdown timer in automatic mode
- **Manual control**: set the light yourself from buttons, the keyboard, or by clicking the signal
- **Realistic car behaviour**
  - Cars brake smoothly and stop at the stop line on red
  - On yellow, a car too close to stop safely keeps going
  - Cars keep a safe gap from the car in front, so queues form naturally
  - Brake lights glow when a car slows down
- **Random traffic**: cars in random colours and speeds, plus occasional trucks
- **Day and night modes**: sun and moving clouds by day; moon, twinkling stars, lit windows, street lamps and headlights by night
- **Adjustable simulation** (web version): light durations, simulation speed and traffic density
- **Live statistics**: cars on the road, cars waiting, and cars that have passed the signal

## 🎮 Controls

| Key / Action | Effect |
|---|---|
| `G` / `Y` / `R` | Set the light to Green / Yellow / Red (manual mode) |
| `A` | Return to automatic mode |
| `N` | Add a car |
| `+` / `-` | Increase / decrease simulation speed |
| `D` | Toggle day / night |
| `P` | Pause / resume |
| Click the traffic light | Switch to the next colour |
| Right-click (desktop version) | Open the options menu |
| `ESC` (desktop version) | Quit |

The web version also has a control panel with buttons and sliders for all of these.

## 🚀 Running the project

### Web version

Open `index.html` in any modern browser. No installation or build step is needed.

### Desktop version (C++ / OpenGL)

Install a C++ compiler and FreeGLUT, then compile:

**Linux (Ubuntu / Debian)**
```bash
sudo apt install build-essential freeglut3-dev
g++ opengl-cpp/traffic_signal.cpp -o traffic -lGL -lGLU -lglut
./traffic
```

**Windows (MinGW + FreeGLUT)**
```bash
g++ opengl-cpp/traffic_signal.cpp -o traffic.exe -lfreeglut -lopengl32 -lglu32
traffic.exe
```

**macOS**
```bash
g++ opengl-cpp/traffic_signal.cpp -o traffic -framework OpenGL -framework GLUT
./traffic
```

## 🧠 Computer graphics concepts used

| Concept | Where it is used |
|---|---|
| **2D primitives** (`GL_QUADS`, `GL_POLYGON`, `GL_TRIANGLE_FAN`, `GL_POINTS`) | Road, buildings, cars, stars |
| **Circle drawing** (parametric equation `x = r·cosθ`, `y = r·sinθ`) | Wheels, lights, sun, moon, clouds |
| **Orthographic projection** (`gluOrtho2D`) | Maps world coordinates (1000 × 600) to the window |
| **Animation** (`glutTimerFunc` / `requestAnimationFrame`) | Moving cars, clouds, light timer |
| **Double buffering** | Flicker-free animation |
| **Alpha blending** | Glow of the traffic light, headlight beams |
| **Colour gradients** | Sky |
| **Keyboard and mouse input** | Light control and interaction |
| **Pop-up menu** (`glutCreateMenu`) | Right-click options (desktop) |
| **Shaders** (vertex + fragment, web version) | Rendering every shape as coloured triangles |
| **Simple collision avoidance** | Cars keep a distance from the car ahead |

## 🗂️ How it works

1. **Traffic light state machine**: the light holds one of three states. In automatic mode a timer moves it to the next state when its duration runs out.
2. **Car update loop**: every frame, the cars in each lane are sorted from front to back. Each car works out the furthest point it may reach: the stop line (if the light is not green) or just behind the car in front. It then speeds up or brakes towards that point.
3. **Rendering**: the scene is drawn back to front: sky, buildings, road, far lane, near lane, then the traffic light.

## 📁 Project structure

```
traffic-signal-simulation/
├── index.html              # Web version (WebGL), deployed to Vercel
├── opengl-cpp/
│   └── traffic_signal.cpp  # Desktop version (C++ / OpenGL / GLUT)
└── README.md
```

## 📜 License

Free to use for learning and academic projects.
