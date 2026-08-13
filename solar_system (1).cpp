/*
    ===========================================================
    SOLAR SYSTEM SIMULATION - Computer Graphics Project
    Language : C++
    Library  : OpenGL + GLUT (freeglut)
    ===========================================================

    Features:
    - Sun at center with self rotation + soft glowing corona (additive blending)
    - 8 Planets orbiting the sun (revolution) + rotating on own axis (rotation)
      -- slow, calm orbit/spin speeds tuned for a realistic, watchable pace
      -- each planet has its own axial tilt (like real planets)
      -- shiny specular material highlights for a more 3D, polished look
    - Saturn has a multi-band, semi-transparent ring
    - Earth has a soft blue atmosphere glow + an orbiting Moon
      (hierarchical / nested transformation)
    - Lighting (Sun acts as light source, local-viewer specular highlights)
    - Camera control using keyboard (rotate view, zoom in/out)
    - Twinkling, colorful starfield background (blue-white, white, yellow, orange, red stars -- just like real star temperatures)
    - Deep-space gradient background (indigo-to-black, instead of flat black)
    - Glowing shooting stars / meteors with a soft tapering, fading trail
    - Asteroid belt between Mars and Jupiter (small tumbling rocks)
    - Earth has a Moon; Mars has its two real moons, Phobos and Deimos
    - Floating planet name labels in 3D space
    - Stylish HUD: title bar, semi-transparent control panel, pause indicator
    - Faint, elegant orbit path lines
    - Mouse-drag camera orbiting + scroll-wheel zoom
    - Pause/Resume animation
    - On-screen instructions

    ===========================================================
    HOW TO COMPILE & RUN
    ===========================================================

    ---- On Linux (Ubuntu/Debian) ----
    1) Install freeglut:
         sudo apt-get install freeglut3-dev
    2) Compile:
         g++ solar_system.cpp -o solar_system -lGL -lGLU -lglut
    3) Run:
         ./solar_system

    ---- On Windows (Code::Blocks / Dev-C++) ----
    1) Install freeglut / setup GLUT for your IDE (search: "glut setup codeblocks")
    2) Create a new project, add this file
    3) Linker settings -> add libraries: opengl32, glu32, freeglut
    4) Build & Run

    ---- On Windows (g++ / MinGW, manual) ----
         g++ solar_system.cpp -o solar_system.exe -lfreeglut -lopengl32 -lglu32

    ===========================================================
    CONTROLS
    ===========================================================
    Left Mouse Drag       -> Rotate camera around the scene (orbit)
    Mouse Scroll Wheel    -> Zoom in / out
    Left / Right Arrow    -> Rotate camera around the scene (azimuth)
    Up / Down Arrow       -> Rotate camera up/down (elevation)
    '+' / '-'             -> Zoom in / Zoom out
    'p'                   -> Pause / Resume animation
    'r'                   -> Reset camera
    ESC                   -> Exit
    ===========================================================
*/

#include <GL/freeglut.h>
#include <cmath>
#include <cstdlib>
#include <ctime>

// ---------------- Global State ----------------
float camDistance = 55.0f;
float camAzimuth  = 0.0f;      // left-right angle
float camElevation = 25.0f;    // up-down angle

bool  paused = false;
float globalTime = 0.0f;       // drives all animation

// ---------------- Mouse drag camera control ----------------
bool  mouseDragging = false;
int   lastMouseX = 0, lastMouseY = 0;
const float MOUSE_SENSITIVITY = 0.35f;

// Starfield
#define NUM_STARS 500
float starX[NUM_STARS], starY[NUM_STARS], starZ[NUM_STARS];
float starBrightness[NUM_STARS], starSize[NUM_STARS], starPhase[NUM_STARS];
float starR[NUM_STARS], starG[NUM_STARS], starB[NUM_STARS];

// ---------------- Shooting stars / meteors ----------------
#define NUM_SHOOTING_STARS 4
#define TRAIL_LENGTH 14

struct ShootingStar {
    bool  active;
    float x, y, z;
    float vx, vy, vz;
    float life, maxLife;
    float trailX[TRAIL_LENGTH], trailY[TRAIL_LENGTH], trailZ[TRAIL_LENGTH];
};
ShootingStar shootingStars[NUM_SHOOTING_STARS];
float shootingStarClock = 0.0f;
float nextShootingStarAt = 120.0f;

// Asteroid belt (between Mars and Jupiter) -- purely visual detail
#define NUM_ASTEROIDS 160
float astAngle[NUM_ASTEROIDS], astRadius[NUM_ASTEROIDS], astY[NUM_ASTEROIDS], astSize[NUM_ASTEROIDS];

// ---------------- Planet Data Structure ----------------
struct Planet {
    const char* name;
    float orbitRadius;     // distance from sun
    float orbitSpeed;      // revolution speed (degrees/frame factor)
    float spinSpeed;       // self rotation speed
    float size;            // radius of the sphere
    float r, g, b;         // color
    bool hasRing;
    bool hasMoon;
    float axialTilt;      // degrees, tilts the spin axis for realism
    float shininess;      // specular highlight strength
};

// Approximate relative values (not to real scale, adjusted for good visuals)
// Colors chosen to match each planet's real/approximate appearance
Planet planets[] = {
    // name        orbitR  orbitSpd spinSpd size    r      g      b     ring   moon   tilt   shine
    { "Mercury",  8.0f, 1.20f, 0.35f, 0.5f, 0.62f, 0.58f, 0.55f, false, false,  0.1f, 10.0f }, // grey-brown, rocky
    { "Venus",   11.0f, 0.47f, 0.30f, 0.9f, 0.93f, 0.86f, 0.66f, false, false,177.4f, 15.0f }, // pale cream/yellow clouds
    { "Earth",   15.0f, 0.29f, 0.55f, 1.0f, 0.15f, 0.42f, 0.75f, false, true, 23.4f, 30.0f }, // blue oceans
    { "Mars",    19.0f, 0.15f, 0.50f, 0.7f, 0.77f, 0.33f, 0.18f, false, false, 25.2f, 8.0f  }, // rusty red
    { "Jupiter", 25.0f, 0.024f,1.10f, 2.4f, 0.82f, 0.65f, 0.48f, false, false,  3.1f, 25.0f }, // tan with bands
    { "Saturn",  31.0f, 0.010f,1.00f, 2.0f, 0.93f, 0.85f, 0.65f, true,  false, 26.7f, 25.0f }, // pale gold
    { "Uranus",  36.0f, 0.0035f,0.70f,1.5f, 0.60f, 0.85f, 0.88f, false, false, 97.8f, 20.0f }, // pale cyan
    { "Neptune", 40.0f, 0.0017f,0.65f,1.4f, 0.20f, 0.35f, 0.85f, false, false, 28.3f, 20.0f }, // deep blue
};
const int NUM_PLANETS = sizeof(planets) / sizeof(Planet);


// ---------------- Stylish deep-space gradient background ----------------
// Drawn as a full-screen quad behind everything (depth writes off, depth test off)
void drawBackgroundGradient() {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 1, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glBegin(GL_QUADS);
        glColor3f(0.02f, 0.015f, 0.05f); glVertex2f(0.0f, 0.0f); // bottom -> near-black
        glColor3f(0.02f, 0.015f, 0.05f); glVertex2f(1.0f, 0.0f);
        glColor3f(0.07f, 0.05f, 0.14f);  glVertex2f(1.0f, 1.0f); // top -> deep indigo
        glColor3f(0.07f, 0.05f, 0.14f);  glVertex2f(0.0f, 1.0f);
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

// ---------------- Draw a floating text label in 3D world space (always screen-facing text) ----------------
void drawLabel3D(float x, float y, float z, const char* text, void* font, float r, float g, float b) {
    glDisable(GL_LIGHTING);
    glColor3f(r, g, b);
    glRasterPos3f(x, y, z);
    for (const char* c = text; *c; c++)
        glutBitmapCharacter(font, *c);
    glEnable(GL_LIGHTING);
}

// ---------------- Init & Draw the asteroid belt (style detail between Mars & Jupiter) ----------------
void initAsteroids() {
    for (int i = 0; i < NUM_ASTEROIDS; i++) {
        astAngle[i]  = (rand() % 3600) / 10.0f;
        astRadius[i] = 21.5f + (rand() % 250) / 100.0f;   // band between Mars(19) and Jupiter(25)
        astY[i]      = ((rand() % 100) - 50) / 130.0f;     // slight vertical scatter
        astSize[i]   = 0.05f + (rand() % 10) / 100.0f;
    }
}

void drawAsteroidBelt() {
    glDisable(GL_LIGHTING);
    glColor3f(0.55f, 0.52f, 0.48f);
    for (int i = 0; i < NUM_ASTEROIDS; i++) {
        float angle = (astAngle[i] + globalTime * 0.6f) * 3.14159265f / 180.0f;
        float x = astRadius[i] * cos(angle);
        float z = astRadius[i] * sin(angle);
        glPushMatrix();
            glTranslatef(x, astY[i], z);
            glutSolidSphere(astSize[i], 6, 6);
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);
}

// ---------------- Shooting Star / Meteor: spawn, update, draw ----------------

// Pick a fresh random path across the outer sky dome for one shooting star
void spawnShootingStar(ShootingStar& s) {
    float startAngle = (rand() % 360) * 3.14159265f / 180.0f;
    float radius      = 70.0f + (rand() % 50);           // starts out near the starfield
    float startHeight = 30.0f + (rand() % 55);           // high up in the sky

    s.x = radius * cos(startAngle);
    s.z = radius * sin(startAngle);
    s.y = startHeight;

    // Travels diagonally downward across the sky in a random compass direction
    float dirAngle = (rand() % 360) * 3.14159265f / 180.0f;
    float speed    = 0.9f + (rand() % 100) / 120.0f;
    s.vx = cosf(dirAngle) * speed;
    s.vz = sinf(dirAngle) * speed;
    s.vy = -(0.35f + (rand() % 40) / 100.0f) * speed;

    s.life    = 0.0f;
    s.maxLife = 55.0f + (rand() % 35);
    s.active  = true;

    // Flatten the trail to the starting point so it doesn't "whip" in from the origin
    for (int j = 0; j < TRAIL_LENGTH; j++) {
        s.trailX[j] = s.x; s.trailY[j] = s.y; s.trailZ[j] = s.z;
    }
}

// Advances position + trail history of all currently-active shooting stars
void updateShootingStars() {
    for (int i = 0; i < NUM_SHOOTING_STARS; i++) {
        ShootingStar& s = shootingStars[i];
        if (!s.active) continue;

        // Shift trail history back one slot, then record the current head position
        for (int j = TRAIL_LENGTH - 1; j > 0; j--) {
            s.trailX[j] = s.trailX[j - 1];
            s.trailY[j] = s.trailY[j - 1];
            s.trailZ[j] = s.trailZ[j - 1];
        }
        s.trailX[0] = s.x; s.trailY[0] = s.y; s.trailZ[0] = s.z;

        s.x += s.vx; s.y += s.vy; s.z += s.vz;
        s.life += 1.0f;

        if (s.life > s.maxLife || s.y < -25.0f) {
            s.active = false;
        }
    }
}

// Renders each meteor as a soft glowing head with a tapering, fading trail
void drawShootingStars() {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);   // additive -> a bright, glowing streak instead of a flat line
    glDepthMask(GL_FALSE);               // don't let the glow occlude things behind it oddly

    for (int i = 0; i < NUM_SHOOTING_STARS; i++) {
        ShootingStar& s = shootingStars[i];
        if (!s.active) continue;

        // --- Tapering glow trail: drawn as a triangle ribbon that narrows and fades toward the tail ---
        glBegin(GL_TRIANGLE_STRIP);
        for (int j = 0; j < TRAIL_LENGTH; j++) {
            float t     = (float)j / (TRAIL_LENGTH - 1);      // 0 at head, 1 at tail
            float alpha = (1.0f - t) * (1.0f - t) * 0.85f;    // fades quickly for a crisp head, soft tail
            float width = (1.0f - t) * 0.22f + 0.01f;

            // Perpendicular offset (approx) so the ribbon has visible width facing the camera-ish plane
            float px = -s.vz, pz = s.vx;
            float len = sqrtf(px * px + pz * pz) + 0.0001f;
            px = px / len * width;
            pz = pz / len * width;

            glColor4f(0.80f, 0.88f, 1.0f, alpha);
            glVertex3f(s.trailX[j] + px, s.trailY[j], s.trailZ[j] + pz);
            glColor4f(1.0f, 0.97f, 0.85f, alpha);
            glVertex3f(s.trailX[j] - px, s.trailY[j], s.trailZ[j] - pz);
        }
        glEnd();

        // --- Bright core + soft halo at the head, like a tiny glowing ember ---
        glPushMatrix();
            glTranslatef(s.x, s.y, s.z);
            glColor4f(0.85f, 0.9f, 1.0f, 0.35f);
            glutSolidSphere(0.45f, 10, 10);   // outer halo
            glColor4f(1.0f, 1.0f, 0.95f, 0.9f);
            glutSolidSphere(0.16f, 10, 10);   // bright core
        glPopMatrix();
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

// ---------------- Utility: Set material with specular highlight (for realistic shiny look) ----------------
void setMaterial(float r, float g, float b, float shininess) {
    GLfloat diffuse[]  = { r, g, b, 1.0f };
    GLfloat specular[] = { 0.35f, 0.35f, 0.35f, 1.0f };
    glColor3f(r, g, b);                                   // works with GL_COLOR_MATERIAL (ambient+diffuse)
    glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
    glMaterialf(GL_FRONT, GL_SHININESS, shininess);
}

// ---------------- Utility: Draw a soft glow shell (additive blending) ----------------
// Used for the Sun's corona and a planet's thin atmosphere halo.
void drawGlow(float radius, float r, float g, float b, float alpha) {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);   // additive -> soft glow instead of a hard edge
    glDepthMask(GL_FALSE);
    glColor4f(r, g, b, alpha);
    glutSolidSphere(radius, 30, 30);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

// ---------------- Utility: Draw one ring band ----------------
void drawRingBand(float innerRadius, float outerRadius, float r, float g, float b, float alpha) {
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= 360; i += 4) {
        float angle = i * 3.14159265f / 180.0f;
        float xInner = innerRadius * cos(angle);
        float zInner = innerRadius * sin(angle);
        float xOuter = outerRadius * cos(angle);
        float zOuter = outerRadius * sin(angle);
        glColor4f(r, g, b, alpha);
        glVertex3f(xInner, 0.0f, zInner);
        glVertex3f(xOuter, 0.0f, zOuter);
    }
    glEnd();
}

// ---------------- Draw Saturn's ring as several bands -> looks layered/realistic ----------------
void drawRing(float baseRadius) {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);

    drawRingBand(baseRadius + 0.10f, baseRadius + 0.55f, 0.75f, 0.68f, 0.52f, 0.85f);
    drawRingBand(baseRadius + 0.60f, baseRadius + 0.90f, 0.85f, 0.80f, 0.65f, 0.55f);
    drawRingBand(baseRadius + 0.95f, baseRadius + 1.35f, 0.65f, 0.60f, 0.48f, 0.75f);
    drawRingBand(baseRadius + 1.40f, baseRadius + 1.55f, 0.90f, 0.85f, 0.72f, 0.40f);

    glEnable(GL_LIGHTING);
    glDisable(GL_BLEND);
}

// ---------------- Draw Orbit Path (circle outline) ----------------
void drawOrbitPath(float radius) {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.45f, 0.45f, 0.55f, 0.35f);   // faint, slightly cool tint -> elegant, not distracting
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 360; i += 2) {
        float angle = i * 3.14159265f / 180.0f;
        glVertex3f(radius * cos(angle), 0.0f, radius * sin(angle));
    }
    glEnd();
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

// ---------------- Draw Starfield ----------------
void initStars() {
    srand((unsigned int)time(0));
    for (int i = 0; i < NUM_STARS; i++) {
        starX[i] = (rand() % 4000 - 2000) / 10.0f;
        starY[i] = (rand() % 4000 - 2000) / 10.0f;
        starZ[i] = (rand() % 4000 - 2000) / 10.0f;
        starBrightness[i] = 0.4f + (rand() % 60) / 100.0f;   // 0.4 - 1.0
        starSize[i] = 1.0f + (rand() % 3) * 0.5f;            // 1.0 - 2.0
        starPhase[i] = (rand() % 628) / 100.0f;               // random twinkle phase

        // Real stars come in different colors depending on temperature.
        // Weighted so most are white/yellow, with fewer blue, orange, and red ones.
        int roll = rand() % 100;
        if (roll < 35) { starR[i] = 0.75f; starG[i] = 0.82f; starB[i] = 1.00f; }      // blue-white (hot)
        else if (roll < 65) { starR[i] = 1.00f; starG[i] = 1.00f; starB[i] = 1.00f; } // white
        else if (roll < 82) { starR[i] = 1.00f; starG[i] = 0.93f; starB[i] = 0.70f; } // pale yellow
        else if (roll < 93) { starR[i] = 1.00f; starG[i] = 0.75f; starB[i] = 0.50f; } // orange
        else { starR[i] = 1.00f; starG[i] = 0.55f; starB[i] = 0.50f; }               // reddish
    }
}

void drawStars() {
    glDisable(GL_LIGHTING);
    for (int i = 0; i < NUM_STARS; i++) {
        // Gentle twinkle using a slow sine wave unique to each star
        float twinkle = 0.75f + 0.25f * sinf(globalTime * 0.5f + starPhase[i]);
        float b = starBrightness[i] * twinkle;
        glPointSize(starSize[i]);
        glColor3f(starR[i] * b, starG[i] * b, starB[i] * b);
        glBegin(GL_POINTS);
            glVertex3f(starX[i], starY[i], starZ[i]);
        glEnd();
    }
    glEnable(GL_LIGHTING);
}

// ---------------- Draw the Sun ----------------
void drawSun() {
    glDisable(GL_LIGHTING);   // sun itself should look bright, not shaded

    // Gentle pulsing so the corona feels alive rather than static
    float pulse = 0.9f + 0.1f * sinf(globalTime * 0.8f);

    glPushMatrix();
        // Outer soft corona -> deep fiery red-orange, wide and very faint
        drawGlow(5.2f * pulse, 1.0f, 0.30f, 0.05f, 0.09f);
        // Middle glow -> vivid orange
        drawGlow(4.3f * pulse, 1.0f, 0.55f, 0.10f, 0.16f);
        // Inner glow -> warm gold, tighter and brighter
        drawGlow(3.5f, 1.0f, 0.78f, 0.30f, 0.22f);

        // Sun's solid body -- vivid golden-orange core
        glColor3f(1.0f, 0.72f, 0.20f);
        glRotatef(globalTime * 2.0f, 0.0f, 1.0f, 0.0f); // slow self-spin
        glutSolidSphere(3.0, 48, 48);
    glPopMatrix();

    glEnable(GL_LIGHTING);
}

// ---------------- Draw one Planet (with orbit + spin + optional moon/ring) ----------------
void drawPlanet(const Planet& p) {
    glPushMatrix();
        // Revolution around the sun (slow, steady orbit)
        glRotatef(globalTime * p.orbitSpeed, 0.0f, 1.0f, 0.0f);
        glTranslatef(p.orbitRadius, 0.0f, 0.0f);

        // Draw the planet itself (axial tilt + self spin + shiny material)
        glPushMatrix();
            glRotatef(p.axialTilt, 0.0f, 0.0f, 1.0f);              // tilt the spin axis
            glRotatef(globalTime * p.spinSpeed * 10.0f, 0.0f, 1.0f, 0.0f); // spin around tilted axis
            setMaterial(p.r, p.g, p.b, p.shininess);
            glutSolidSphere(p.size, 40, 40);

            // Thin atmosphere glow for Earth -> gives a soft "habitable planet" look
            if (p.name[0] == 'E' && p.name[1] == 'a') {
                drawGlow(p.size + 0.25f, 0.35f, 0.55f, 1.0f, 0.20f);
            }
        glPopMatrix();

        // Stylish floating name label just above the planet
        drawLabel3D(0.0f, p.size + 0.9f, 0.0f, p.name, GLUT_BITMAP_HELVETICA_10, 0.85f, 0.9f, 1.0f);

        // Ring (Saturn)
        if (p.hasRing) {
            glPushMatrix();
                glRotatef(20.0f, 1.0f, 0.0f, 1.0f); // tilt the ring a bit
                drawRing(p.size + 0.4f);
            glPopMatrix();
        }

        // Moon (Earth) -- nested/hierarchical transformation
        if (p.hasMoon) {
            glPushMatrix();
                glRotatef(globalTime * 3.0f, 0.0f, 1.0f, 0.0f); // moon orbit around planet
                glTranslatef(p.size + 1.4f, 0.0f, 0.0f);
                setMaterial(0.68f, 0.66f, 0.64f, 4.0f);
                glutSolidSphere(0.27, 20, 20);
            glPopMatrix();
        }

        // Mars's two small moons -- Phobos (closer & faster) and Deimos (farther & slower)
        if (p.name[0] == 'M' && p.name[1] == 'a') {
            glPushMatrix();
                glRotatef(globalTime * 9.0f, 0.1f, 1.0f, 0.0f);   // Phobos: fast, tiny, close orbit
                glTranslatef(p.size + 0.9f, 0.0f, 0.0f);
                setMaterial(0.55f, 0.50f, 0.46f, 3.0f);
                glutSolidSphere(0.10f, 12, 12);
            glPopMatrix();

            glPushMatrix();
                glRotatef(globalTime * 4.0f + 90.0f, -0.1f, 1.0f, 0.0f); // Deimos: slower, farther
                glTranslatef(p.size + 1.6f, 0.0f, 0.0f);
                setMaterial(0.60f, 0.56f, 0.52f, 3.0f);
                glutSolidSphere(0.07f, 12, 12);
            glPopMatrix();
        }

    glPopMatrix();
}

// ---------------- Display Callback ----------------
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawBackgroundGradient();   // stylish deep-space gradient behind everything
    glLoadIdentity();

    // ---- Camera positioning (spherical coordinates around origin) ----
    float camX = camDistance * cos(camElevation * 3.14159265f / 180.0f) * sin(camAzimuth * 3.14159265f / 180.0f);
    float camY = camDistance * sin(camElevation * 3.14159265f / 180.0f);
    float camZ = camDistance * cos(camElevation * 3.14159265f / 180.0f) * cos(camAzimuth * 3.14159265f / 180.0f);

    gluLookAt(camX, camY, camZ,   // eye position
              0.0, 0.0, 0.0,      // look at the sun (origin)
              0.0, 1.0, 0.0);     // up vector

    // Light position = at the sun (origin)
    GLfloat lightPos[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    drawStars();
    drawShootingStars();
    drawSun();
    drawAsteroidBelt();

    for (int i = 0; i < NUM_PLANETS; i++) {
        drawOrbitPath(planets[i].orbitRadius);
        drawPlanet(planets[i]);
    }

    // ---- Stylish on-screen HUD (title bar + semi-transparent instruction panel) ----
    glDisable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 1000, 0, 700);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Semi-transparent dark panel behind the bottom instruction bar
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.05f, 0.05f, 0.12f, 0.55f);
    glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2f(1000, 0);
        glVertex2f(1000, 34);
        glVertex2f(0, 34);
    glEnd();
    // A thin accent line above the panel
    glColor4f(0.55f, 0.65f, 1.0f, 0.6f);
    glBegin(GL_LINES);
        glVertex2f(0, 34);
        glVertex2f(1000, 34);
    glEnd();
    glDisable(GL_BLEND);

    // Title, top-left, larger stylish font
    glColor3f(0.75f, 0.85f, 1.0f);
    glRasterPos2i(15, 665);
    const char* title = "SOLAR SYSTEM SIMULATION";
    for (const char* c = title; *c; c++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

    // Status text (paused indicator), top-right
    glColor3f(1.0f, 0.85f, 0.4f);
    const char* status = paused ? "[ PAUSED ]" : "";
    glRasterPos2i(880, 665);
    for (const char* c = status; *c; c++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    // Controls, inside the bottom panel
    glColor3f(0.85f, 0.85f, 0.9f);
    const char* msg = "Mouse Drag: Rotate Camera | Scroll/+/-: Zoom | Arrows: Rotate | p: Pause | r: Reset | ESC: Exit";
    glRasterPos2i(15, 12);
    for (const char* c = msg; *c; c++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_LIGHTING);

    glutSwapBuffers();
}

// ---------------- Reshape Callback ----------------
void reshape(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)w / (double)h, 0.1, 500.0);
    glMatrixMode(GL_MODELVIEW);
}

// ---------------- Idle Callback (animation driver) ----------------
void idle() {
    if (!paused) {
        globalTime += 0.05f;   // controls overall simulation speed (lower = slower)

        // Occasionally spawn a new shooting star into a free slot
        shootingStarClock += 1.0f;
        if (shootingStarClock >= nextShootingStarAt) {
            for (int i = 0; i < NUM_SHOOTING_STARS; i++) {
                if (!shootingStars[i].active) {
                    spawnShootingStar(shootingStars[i]);
                    break;
                }
            }
            shootingStarClock   = 0.0f;
            nextShootingStarAt  = 130.0f + (rand() % 220); // next one in a little while
        }
        updateShootingStars();
    }
    glutPostRedisplay();
}

// ---------------- Keyboard: Special keys (arrows) ----------------
void specialKeys(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_LEFT:  camAzimuth   -= 3.0f; break;
        case GLUT_KEY_RIGHT: camAzimuth   += 3.0f; break;
        case GLUT_KEY_UP:    camElevation += 2.0f; if (camElevation > 89) camElevation = 89; break;
        case GLUT_KEY_DOWN:  camElevation -= 2.0f; if (camElevation < -89) camElevation = -89; break;
    }
    glutPostRedisplay();
}

// ---------------- Mouse: button press/release (drag start/stop + scroll-wheel zoom) ----------------
void mouseButton(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            mouseDragging = true;
            lastMouseX = x;
            lastMouseY = y;
        } else {
            mouseDragging = false;
        }
    }
    // Scroll wheel: button 3 = wheel up, button 4 = wheel down (freeglut convention)
    else if (button == 3 && state == GLUT_DOWN) {
        camDistance -= 3.0f; if (camDistance < 10.0f) camDistance = 10.0f;
    }
    else if (button == 4 && state == GLUT_DOWN) {
        camDistance += 3.0f; if (camDistance > 150.0f) camDistance = 150.0f;
    }
    glutPostRedisplay();
}

// ---------------- Mouse: motion while dragging -> orbit the camera around the scene ----------------
void mouseMotion(int x, int y) {
    if (mouseDragging) {
        int dx = x - lastMouseX;
        int dy = y - lastMouseY;

        camAzimuth   += dx * MOUSE_SENSITIVITY;
        camElevation += dy * MOUSE_SENSITIVITY;
        if (camElevation > 89.0f)  camElevation = 89.0f;
        if (camElevation < -89.0f) camElevation = -89.0f;

        lastMouseX = x;
        lastMouseY = y;
        glutPostRedisplay();
    }
}

// ---------------- Keyboard: Normal keys ----------------
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case '+': camDistance -= 2.0f; if (camDistance < 10) camDistance = 10; break;
        case '-': camDistance += 2.0f; if (camDistance > 150) camDistance = 150; break;
        case 'p': case 'P': paused = !paused; break;
        case 'r': case 'R':
            camDistance = 55.0f; camAzimuth = 0.0f; camElevation = 25.0f;
            break;
        case 27: exit(0); break; // ESC
    }
    glutPostRedisplay();
}

// ---------------- Lighting / GL Setup ----------------
void initGL() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    GLfloat lightAmbient[]  = { 0.15f, 0.15f, 0.15f, 1.0f };
    GLfloat lightDiffuse[]  = { 1.0f, 0.93f, 0.78f, 1.0f };
    GLfloat lightSpecular[] = { 1.0f, 0.95f, 0.85f, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT,  lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);

    glEnable(GL_NORMALIZE);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE); // sharper, more realistic specular highlights
    glShadeModel(GL_SMOOTH);
    initStars();
    initAsteroids();
}

// ---------------- Main ----------------
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1000, 700);
    glutInitWindowPosition(50, 50);
    glutCreateWindow("Solar System Simulation - Computer Graphics Project");

    initGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutSpecialFunc(specialKeys);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);

    glutMainLoop();
    return 0;
}
