/*
 * ============================================================
 *        TRAFFIC SIGNAL SIMULATION  -  OpenGL / GLUT
 * ============================================================
 *  Cars drive along a two-lane road and obey a traffic light.
 *  The light cycles automatically GREEN -> YELLOW -> RED, or can
 *  be controlled manually.
 *
 *  CONTROLS
 *    g / y / r : force GREEN / YELLOW / RED   (switches to manual mode)
 *    a         : back to AUTOMATIC mode
 *    n         : add a new car
 *    + / -     : increase / decrease simulation speed
 *    d         : toggle day / night
 *    p         : pause / resume
 *    Left click on the signal : change to next light
 *    Right click : menu
 *    ESC       : quit
 *
 *  COMPILE
 *    Linux  : g++ traffic_signal.cpp -o traffic -lGL -lGLU -lglut
 *    Windows: g++ traffic_signal.cpp -o traffic.exe -lfreeglut -lopengl32 -lglu32
 *    macOS  : g++ traffic_signal.cpp -o traffic -framework OpenGL -framework GLUT
 * ============================================================
 */

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <algorithm>

// ---------------- Window / world constants ----------------
const int   WIN_W = 1000, WIN_H = 600;
const float STOP_LINE  = 600.0f;           // x position of the stop line
const float LANE_Y[2]  = {150.0f, 230.0f}; // y of the two lanes
const float PI = 3.14159265f;

// ---------------- Traffic light state ----------------
enum Light { GREEN = 0, YELLOW = 1, RED = 2 };
Light light = GREEN;
const float DURATION[3] = {6.0f, 2.0f, 5.0f};  // seconds for each light
float lightTimer = 0.0f;
bool  autoMode   = true;

// ---------------- Simulation state ----------------
bool  paused     = false;
bool  night      = false;
float speedScale = 1.0f;
float spawnTimer = 0.0f;
float cloudX     = 0.0f;
int   carsPassed = 0;

struct Car {
    float x;          // rear-left x
    int   lane;
    float length;
    float speed;      // current speed (px per frame at scale 1)
    float maxSpeed;
    float r, g, b;    // body colour
    bool  counted;
};
std::vector<Car> cars;

// ============================================================
//                    Drawing helpers
// ============================================================
void drawRect(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
    glVertex2f(x, y);     glVertex2f(x + w, y);
    glVertex2f(x + w, y + h); glVertex2f(x, y + h);
    glEnd();
}

void drawCircle(float cx, float cy, float r, int seg = 30) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= seg; i++) {
        float a = 2 * PI * i / seg;
        glVertex2f(cx + r * cosf(a), cy + r * sinf(a));
    }
    glEnd();
}

void drawText(float x, float y, const char* s, void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    while (*s) glutBitmapCharacter(font, *s++);
}

// ============================================================
//                    Scene elements
// ============================================================
void drawSky() {
    glBegin(GL_QUADS);
    if (night) glColor3f(0.02f, 0.02f, 0.12f); else glColor3f(0.35f, 0.65f, 0.95f);
    glVertex2f(0, WIN_H); glVertex2f(WIN_W, WIN_H);
    if (night) glColor3f(0.10f, 0.10f, 0.25f); else glColor3f(0.75f, 0.90f, 1.0f);
    glVertex2f(WIN_W, 300); glVertex2f(0, 300);
    glEnd();

    if (night) {
        glColor3f(0.95f, 0.95f, 0.85f); drawCircle(850, 520, 35);     // moon
        glColor3f(0.02f, 0.02f, 0.12f); drawCircle(865, 530, 30);     // crescent cut
        glColor3f(1, 1, 1);
        glPointSize(2);
        glBegin(GL_POINTS);
        srand(7);                                                      // fixed stars
        for (int i = 0; i < 80; i++) glVertex2f(rand() % WIN_W, 320 + rand() % 280);
        glEnd();
        srand((unsigned)time(0) + carsPassed);
    } else {
        glColor3f(1.0f, 0.9f, 0.2f); drawCircle(850, 520, 40);        // sun
        glColor3f(1, 1, 1);                                            // clouds
        for (int i = 0; i < 3; i++) {
            float cx = fmodf(cloudX + i * 380, WIN_W + 200) - 100;
            float cy = 480 + (i % 2) * 40;
            drawCircle(cx, cy, 25); drawCircle(cx + 25, cy + 12, 30); drawCircle(cx + 55, cy, 25);
        }
    }
}

void drawBuildings() {
    float xs[] = {30, 140, 260, 380, 700, 820, 920};
    float hs[] = {150, 220, 120, 180, 200, 140, 170};
    for (int i = 0; i < 7; i++) {
        float shade = 0.45f + 0.05f * (i % 3);
        if (night) glColor3f(shade * 0.3f, shade * 0.3f, shade * 0.4f);
        else       glColor3f(shade, shade * 0.9f, shade * 0.8f);
        drawRect(xs[i], 300, 90, hs[i]);
        // windows
        for (float wy = 315; wy < 300 + hs[i] - 25; wy += 30)
            for (float wx = xs[i] + 12; wx < xs[i] + 80; wx += 25) {
                if (night) glColor3f(1.0f, 0.9f, 0.4f); else glColor3f(0.7f, 0.85f, 0.95f);
                drawRect(wx, wy, 14, 18);
            }
    }
}

void drawRoad() {
    // grass / footpath
    if (night) glColor3f(0.05f, 0.25f, 0.05f); else glColor3f(0.2f, 0.65f, 0.2f);
    drawRect(0, 0, WIN_W, 110);
    glColor3f(0.6f, 0.6f, 0.6f); drawRect(0, 280, WIN_W, 20);    // sidewalk top
    glColor3f(0.6f, 0.6f, 0.6f); drawRect(0, 110, WIN_W, 15);    // sidewalk bottom

    // asphalt
    glColor3f(0.18f, 0.18f, 0.18f);
    drawRect(0, 125, WIN_W, 155);

    // dashed lane divider
    glColor3f(1, 1, 1);
    for (int x = 0; x < WIN_W; x += 60) drawRect(x, 200, 35, 5);

    // zebra crossing just after stop line
    for (float y = 130; y < 275; y += 20) drawRect(STOP_LINE + 15, y, 45, 10);

    // stop line
    glColor3f(1, 1, 1);
    drawRect(STOP_LINE - 4, 125, 6, 155);
}

void drawTrafficLight() {
    float px = STOP_LINE + 70, py = 300;
    // pole
    glColor3f(0.25f, 0.25f, 0.25f);
    drawRect(px, py, 10, 150);
    // housing
    glColor3f(0.1f, 0.1f, 0.1f);
    drawRect(px - 20, py + 150, 50, 130);

    float cy[3] = {py + 175, py + 215, py + 255};   // green, yellow, red (bottom->top)
    float on[3][3]  = {{0.0f, 1.0f, 0.2f}, {1.0f, 0.85f, 0.0f}, {1.0f, 0.1f, 0.1f}};
    for (int i = 0; i < 3; i++) {
        if (i == light) {
            // glow
            glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(on[i][0], on[i][1], on[i][2], 0.3f);
            drawCircle(px + 5, cy[i], 24);
            glDisable(GL_BLEND);
            glColor3f(on[i][0], on[i][1], on[i][2]);
        } else {
            glColor3f(on[i][0] * 0.25f, on[i][1] * 0.25f, on[i][2] * 0.25f);
        }
        drawCircle(px + 5, cy[i], 15);
    }

    // countdown above the signal
    if (autoMode) {
        char buf[16];
        sprintf(buf, "%d", (int)ceilf(DURATION[light] - lightTimer));
        glColor3f(1, 1, 1);
        drawText(px - 1, py + 290, buf);
    }
}

void drawCar(const Car& c) {
    float x = c.x, y = LANE_Y[c.lane] - 25, L = c.length;

    // shadow
    glColor3f(0.1f, 0.1f, 0.1f);
    drawRect(x + 3, y - 3, L, 6);
    // body
    glColor3f(c.r, c.g, c.b);
    drawRect(x, y + 5, L, 22);
    // roof
    glBegin(GL_POLYGON);
    glVertex2f(x + L * 0.20f, y + 27); glVertex2f(x + L * 0.80f, y + 27);
    glVertex2f(x + L * 0.68f, y + 45); glVertex2f(x + L * 0.30f, y + 45);
    glEnd();
    // windows
    glColor3f(0.75f, 0.9f, 1.0f);
    glBegin(GL_POLYGON);
    glVertex2f(x + L * 0.26f, y + 29); glVertex2f(x + L * 0.48f, y + 29);
    glVertex2f(x + L * 0.48f, y + 42); glVertex2f(x + L * 0.32f, y + 42);
    glEnd();
    glBegin(GL_POLYGON);
    glVertex2f(x + L * 0.52f, y + 29); glVertex2f(x + L * 0.74f, y + 29);
    glVertex2f(x + L * 0.66f, y + 42); glVertex2f(x + L * 0.52f, y + 42);
    glEnd();
    // headlight / tail light
    if (night) {
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1, 1, 0.6f, 0.25f);
        glBegin(GL_TRIANGLES);
        glVertex2f(x + L, y + 18); glVertex2f(x + L + 90, y + 35); glVertex2f(x + L + 90, y);
        glEnd();
        glDisable(GL_BLEND);
    }
    glColor3f(1, 1, 0.6f); drawRect(x + L - 5, y + 15, 5, 7);
    bool braking = c.speed < c.maxSpeed * 0.5f;
    if (braking) glColor3f(1, 0, 0); else glColor3f(0.5f, 0, 0);
    drawRect(x, y + 15, 5, 7);
    // wheels
    glColor3f(0.05f, 0.05f, 0.05f);
    drawCircle(x + L * 0.22f, y + 5, 9);
    drawCircle(x + L * 0.78f, y + 5, 9);
    glColor3f(0.7f, 0.7f, 0.7f);
    drawCircle(x + L * 0.22f, y + 5, 4);
    drawCircle(x + L * 0.78f, y + 5, 4);
}

void drawHUD() {
    glColor3f(0, 0, 0);
    drawRect(0, 0, WIN_W, 40);
    glColor3f(1, 1, 1);
    const char* names[3] = {"GREEN", "YELLOW", "RED"};
    char buf[200];
    sprintf(buf, "Signal: %-6s | Mode: %s | Speed: x%.1f | Cars: %d | Passed: %d %s",
            names[light], autoMode ? "AUTO" : "MANUAL", speedScale,
            (int)cars.size(), carsPassed, paused ? "| PAUSED" : "");
    drawText(10, 14, buf);

    glColor3f(1, 1, 1);
    drawText(10, 580, "TRAFFIC SIGNAL SIMULATION", GLUT_BITMAP_TIMES_ROMAN_24);
    drawText(10, 560, "g/y/r: set light  a: auto  n: add car  +/-: speed  d: day/night  p: pause  ESC: quit",
             GLUT_BITMAP_HELVETICA_12);
}

// ============================================================
//                    Simulation logic
// ============================================================
void addCar(int lane = -1) {
    if (lane < 0) lane = rand() % 2;
    // don't spawn on top of another car
    for (const Car& c : cars)
        if (c.lane == lane && c.x < 20) return;

    Car c;
    c.lane = lane;
    c.length = 80 + rand() % 30;
    c.x = -c.length - 10;
    c.maxSpeed = 2.0f + (rand() % 100) / 50.0f;   // 2.0 - 4.0
    c.speed = c.maxSpeed;
    c.r = (rand() % 100) / 100.0f;
    c.g = (rand() % 100) / 100.0f;
    c.b = (rand() % 100) / 100.0f;
    c.counted = false;
    cars.push_back(c);
}

void nextLight() {
    light = (Light)((light + 1) % 3);
    lightTimer = 0;
}

void updateCars() {
    for (int lane = 0; lane < 2; lane++) {
        // collect cars in this lane, front-most first
        std::vector<Car*> laneCars;
        for (Car& c : cars) if (c.lane == lane) laneCars.push_back(&c);
        std::sort(laneCars.begin(), laneCars.end(),
                  [](Car* a, Car* b) { return a->x > b->x; });

        for (size_t i = 0; i < laneCars.size(); i++) {
            Car* c = laneCars[i];
            float front = c->x + c->length;
            float limit = 1e9f;   // furthest x the front of the car may reach

            // 1. obey the traffic signal
            float distToLine = STOP_LINE - front;
            if (light != GREEN && distToLine >= -2) {
                // on yellow, a car too close to stop safely keeps going
                bool tooCloseToStop = (light == YELLOW && distToLine < 25);
                if (!tooCloseToStop) limit = STOP_LINE - 3;
            }
            // 2. keep a safe gap to the car in front
            if (i > 0) limit = std::min(limit, laneCars[i - 1]->x - 18);

            // smooth acceleration / braking
            float target = c->maxSpeed;
            float gap = limit - front;
            if (gap < 120) target = std::max(0.0f, std::min(c->maxSpeed, gap * 0.06f));
            if (c->speed < target) c->speed = std::min(target, c->speed + 0.08f);
            else                   c->speed = target;
            if (gap < 0.5f) c->speed = 0;

            c->x += c->speed * speedScale;

            if (!c->counted && c->x > STOP_LINE) { c->counted = true; carsPassed++; }
        }
    }
    // remove cars that left the screen
    cars.erase(std::remove_if(cars.begin(), cars.end(),
               [](const Car& c) { return c.x > WIN_W + 20; }), cars.end());
}

void timer(int) {
    if (!paused) {
        float dt = 0.016f * speedScale;
        if (autoMode) {
            lightTimer += dt;
            if (lightTimer >= DURATION[light]) nextLight();
        }
        spawnTimer += dt;
        if (spawnTimer > 1.2f) { spawnTimer = 0; if (cars.size() < 16) addCar(); }
        cloudX += 0.3f * speedScale;
        updateCars();
    }
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

// ============================================================
//                    GLUT callbacks
// ============================================================
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    drawSky();
    drawBuildings();
    drawRoad();
    // draw upper lane first so lower lane cars appear in front
    for (const Car& c : cars) if (c.lane == 1) drawCar(c);
    for (const Car& c : cars) if (c.lane == 0) drawCar(c);
    drawTrafficLight();
    drawHUD();
    glutSwapBuffers();
}

void keyboard(unsigned char key, int, int) {
    switch (key) {
        case 'g': case 'G': light = GREEN;  autoMode = false; break;
        case 'y': case 'Y': light = YELLOW; autoMode = false; break;
        case 'r': case 'R': light = RED;    autoMode = false; break;
        case 'a': case 'A': autoMode = true; lightTimer = 0;  break;
        case 'n': case 'N': addCar(); break;
        case '+': case '=': speedScale = std::min(4.0f, speedScale + 0.5f); break;
        case '-': case '_': speedScale = std::max(0.5f, speedScale - 0.5f); break;
        case 'd': case 'D': night = !night; break;
        case 'p': case 'P': paused = !paused; break;
        case 27: exit(0);
    }
}

void mouse(int button, int state, int mx, int my) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        // convert window coordinates to world coordinates
        float wx = mx * (float)WIN_W / glutGet(GLUT_WINDOW_WIDTH);
        float wy = WIN_H - my * (float)WIN_H / glutGet(GLUT_WINDOW_HEIGHT);
        float px = STOP_LINE + 70;
        if (wx > px - 20 && wx < px + 30 && wy > 450 && wy < 580) {
            autoMode = false;
            nextLight();
        }
    }
}

void menu(int option) { keyboard((unsigned char)option, 0, 0); }

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    srand((unsigned)time(0));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WIN_W, WIN_H);
    glutInitWindowPosition(100, 50);
    glutCreateWindow("Traffic Signal Simulation - OpenGL");

    glClearColor(0, 0, 0, 1);
    glEnable(GL_POINT_SMOOTH);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutTimerFunc(16, timer, 0);

    // right-click menu
    glutCreateMenu(menu);
    glutAddMenuEntry("Green light",      'g');
    glutAddMenuEntry("Yellow light",     'y');
    glutAddMenuEntry("Red light",        'r');
    glutAddMenuEntry("Automatic mode",   'a');
    glutAddMenuEntry("Add car",          'n');
    glutAddMenuEntry("Toggle day/night", 'd');
    glutAddMenuEntry("Pause / Resume",   'p');
    glutAddMenuEntry("Exit",             27);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    for (int i = 0; i < 4; i++) addCar(i % 2);

    glutMainLoop();
    return 0;
}
