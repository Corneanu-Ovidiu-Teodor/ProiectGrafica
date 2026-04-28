#include <GL/freeglut.h>
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define PI 3.14159265f

// =========================================================================
// === CAMERA (Free Fly - Controlata cu WASD + Mouse) ===
// =========================================================================
// Aceste variabile controleaza spectatorul (camera) care zboara liber prin scena
float camX = 0.0f, camY = 8.0f, camZ = 45.0f;
float camYaw = -90.0f, camPitch = -15.0f; // Camera priveste putin in jos
float vitezaCam = 0.8f;
bool keyW = false, keyS = false, keyA = false, keyD = false;

// =========================================================================
// === [INCEPUT C1]: OBIECT CONTROLABIL (Variabile globale)
// Cerinta: Adaugarea unui obiect controlabil (masina).
// =========================================================================
// Aici retin pozitia si rotatia masinii mele.
// Masina este independenta de camera si o controlez din sageti.
float masinaX = 0.0f, masinaZ = 35.0f;
float masinaRot = 0.0f; // Rotatia masinii in grade (pentru a vira)
float vitezaMasina = 0.6f;
// Retin starea sagetilor de pe tastatura
bool keySus = false, keyJos = false, keyStanga = false, keyDreapta = false;
// === [SFARSIT C1]: Variabile obiect controlabil ==========================


int centruX = 512, centruY = 384;
bool isPaused = false;
bool desenamUmbre = false;

float posStalpi[7][2];
GLuint texIarba, texAsfalt, texCladire, texSky[6];


// =========================================================================
// === [INCEPUT C1]: DETECTAREA COLIZIUNILOR (Structuri si Functii)
// Cerinta: Detectarea coliziunilor dintre cladiri si obiect.
// =========================================================================
// Am facut o structura simpla pentru a retine datele cladirilor
// x, z = centrul cladirii pe harta; l, a = latime, adancime; h = inaltime
struct Cladire { float x, z, l, h, a; };

// Array cu toate cladirile din orasul meu (pentru a le verifica la coliziune)
Cladire cladiri[5] = {
    {0, 0, 3, 10, 3},
    {8, 5, 2, 8, 2},
    {-7, -8, 2.5f, 6, 4},
    {-8, 8, 2, 7, 2},
    {8, -8, 2, 5, 3}
};

// Functia care verifica daca viitoarea pozitie a masinii se loveste de o cladire
// Folosesc metoda AABB (Axis-Aligned Bounding Box) largita cu "razaObiect" (masina)
bool verificaColiziune(float nextX, float nextZ, float razaObiect) {
    for (int i = 0; i < 5; i++) {
        // Calculez marginile cladirii curente, la care adun raza masinii
        float minX = cladiri[i].x - cladiri[i].l - razaObiect;
        float maxX = cladiri[i].x + cladiri[i].l + razaObiect;
        float minZ = cladiri[i].z - cladiri[i].a - razaObiect;
        float maxZ = cladiri[i].z + cladiri[i].a + razaObiect;

        // Daca viitoarea pozitie intra in aceasta zona de interzis, returnam true (avem coliziune)
        if (nextX > minX && nextX < maxX && nextZ > minZ && nextZ < maxZ) {
            return true;
        }
    }
    return false; // Nu am lovit nimic, calea e libera
}
// === [SFARSIT C1]: DETECTAREA COLIZIUNILOR ===============================


// =========================================================================
// === [INCEPUT C2]: OBIECTE IN MISCARE (Date si Initializare)
// Cerinta: Adaugarea unor obiecte care se misca aleator + dupa o regula
// =========================================================================

// 1. Obiect cu REGULA PRESTABILITA (Autobuz pe traseu circular)
// Retin doar unghiul curent; autobuzul va merge in cerc la infinit.
float unghiAutobuz = 0.0f;

// 2. Obiecte cu MISCARE ALEATOARE (Drone care zboara haotic si ricosaza)
// Structura pentru a tine minte pozitia (x,y,z) si viteza pe fiecare axa (vx,vy,vz)
struct Drona { float x, y, z, vx, vy, vz; };
Drona drone[15]; // Folosesc 15 drone

// Functie apelata la inceput pentru a pune dronele pe pozitii random si a le da viteze random
void initializareDroneAleatoare() {
    srand((unsigned)time(NULL)); // Initializez seed-ul de random cu timpul curent
    for (int i = 0; i < 15; i++) {
        drone[i].x = (rand() % 40) - 20;
        drone[i].y = (rand() % 10) + 2;
        drone[i].z = (rand() % 40) - 20;

        // Viteze random intre -0.05 si +0.05
        drone[i].vx = ((rand() % 100) / 1000.0f) - 0.05f;
        drone[i].vy = ((rand() % 100) / 1000.0f) - 0.05f;
        drone[i].vz = ((rand() % 100) / 1000.0f) - 0.05f;
    }
}

// Functie apelata la fiecare frame pentru a misca dronele
void actualizeazaMiscareAleatoare() {
    for (int i = 0; i < 15; i++) {
        // Adaug viteza la pozitie
        drone[i].x += drone[i].vx;
        drone[i].y += drone[i].vy;
        drone[i].z += drone[i].vz;

        // Regula de Ricosare (daca ating marginea hartii invizibile, se intorc)
        if (drone[i].x < -30.0f || drone[i].x > 30.0f) drone[i].vx *= -1.0f;
        if (drone[i].y < 1.0f || drone[i].y > 15.0f) drone[i].vy *= -1.0f;
        if (drone[i].z < -30.0f || drone[i].z > 30.0f) drone[i].vz *= -1.0f;
    }
}
// === [SFARSIT C2]: OBIECTE IN MISCARE ====================================


// ==========================================
// FUNCTII UTILE & INCARCARE TEXTURI (P1)
// ==========================================
// Incarc texturile in siguranta, daca nu gaseste poza pe hard disk, pune o culoare (sa nu crape programul)
GLuint incarcaTexturaSigura(const char* fisier, unsigned char r, unsigned char g, unsigned char b, bool repeta = true) {
    GLuint texID; glGenTextures(1, &texID); glBindTexture(GL_TEXTURE_2D, texID);
    int w, h, c;
    unsigned char* data = stbi_load(fisier, &w, &h, &c, 4);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }
    else {
        unsigned char fallback[4] = { r, g, b, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, fallback);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (repeta) { glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); }
    else { glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F); }
    return texID;
}

// Matematica pentru a desena umbre pe pamant
void calculeazaMatriceUmbra(float mat[16], float lumina[4], float inaltimePlan) {
    float plan[4] = { 0.0f, 1.0f, 0.0f, -inaltimePlan };
    float dot = plan[0] * lumina[0] + plan[1] * lumina[1] + plan[2] * lumina[2] + plan[3] * lumina[3];
    mat[0] = dot - lumina[0] * plan[0]; mat[4] = 0.f - lumina[0] * plan[1]; mat[8] = 0.f - lumina[0] * plan[2]; mat[12] = 0.f - lumina[0] * plan[3];
    mat[1] = 0.f - lumina[1] * plan[0]; mat[5] = dot - lumina[1] * plan[1]; mat[9] = 0.f - lumina[1] * plan[2]; mat[13] = 0.f - lumina[1] * plan[3];
    mat[2] = 0.f - lumina[2] * plan[0]; mat[6] = 0.f - lumina[2] * plan[1]; mat[10] = dot - lumina[2] * plan[2]; mat[14] = 0.f - lumina[2] * plan[3];
    mat[3] = 0.f - lumina[3] * plan[0]; mat[7] = 0.f - lumina[3] * plan[1]; mat[11] = 0.f - lumina[3] * plan[2]; mat[15] = dot - lumina[3] * plan[3];
}

void initializareLumina() {
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    glEnable(GL_LIGHT0);
    GLfloat ambientSoare[] = { 0.1f, 0.1f, 0.15f, 1.0f };
    GLfloat difuzSoare[] = { 0.6f, 0.65f, 0.75f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientSoare);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, difuzSoare);
}

void initializare() {
    glEnable(GL_DEPTH_TEST); glEnable(GL_TEXTURE_2D); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Ajuta ca umbrele de pe asfalt sa nu aiba flicker vizual
    glEnable(GL_POLYGON_OFFSET_FILL); glPolygonOffset(1.0, 1.0);

    initializareLumina();

    // Aici chem functia ca sa dau startul miscarii aleatoare pentru C2
    initializareDroneAleatoare();

    stbi_set_flip_vertically_on_load(true);
    texIarba = incarcaTexturaSigura("iarba.jpg", 34, 139, 34);
    texAsfalt = incarcaTexturaSigura("asfalt.jpg", 100, 100, 100);
    texCladire = incarcaTexturaSigura("cladire.jpg", 200, 200, 200);

    stbi_set_flip_vertically_on_load(false);
    texSky[0] = incarcaTexturaSigura("Daylight Box_Front.bmp", 20, 20, 40, false);
    texSky[1] = incarcaTexturaSigura("Daylight Box_Back.bmp", 20, 20, 40, false);
    texSky[2] = incarcaTexturaSigura("Daylight Box_Left.bmp", 20, 20, 40, false);
    texSky[3] = incarcaTexturaSigura("Daylight Box_Right.bmp", 20, 20, 40, false);
    texSky[4] = incarcaTexturaSigura("Daylight Box_Top.bmp", 20, 20, 40, false);

    // Initializare pozitii stalpi in cerc
    for (int i = 0; i < 7; i++) {
        float u = (i * 51.4f) * (PI / 180.0f);
        posStalpi[i][0] = cos(u) * 18.0f;
        posStalpi[i][1] = sin(u) * 18.0f;
    }
    glutSetCursor(GLUT_CURSOR_NONE);
}

// ==========================================
// DESENARE MEDIU, CLADIRI (Geometrie statica)
// ==========================================
void deseneazaSkybox() {
    glDisable(GL_LIGHTING); float d = 120.0f; glColor3f(0.05f, 0.05f, 0.1f);
    glBindTexture(GL_TEXTURE_2D, texSky[1]); glBegin(GL_QUADS); glTexCoord2f(1, 0); glVertex3f(-d, -d, -d); glTexCoord2f(0, 0); glVertex3f(d, -d, -d); glTexCoord2f(0, 1); glVertex3f(d, d, -d); glTexCoord2f(1, 1); glVertex3f(-d, d, -d); glEnd();
    glBindTexture(GL_TEXTURE_2D, texSky[0]); glBegin(GL_QUADS); glTexCoord2f(0, 0); glVertex3f(-d, -d, d); glTexCoord2f(1, 0); glVertex3f(d, -d, d); glTexCoord2f(1, 1); glVertex3f(d, d, d); glTexCoord2f(0, 1); glVertex3f(-d, d, d); glEnd();
    glBindTexture(GL_TEXTURE_2D, texSky[2]); glBegin(GL_QUADS); glTexCoord2f(1, 0); glVertex3f(-d, -d, d); glTexCoord2f(0, 0); glVertex3f(-d, -d, -d); glTexCoord2f(0, 1); glVertex3f(-d, d, -d); glTexCoord2f(1, 1); glVertex3f(-d, d, d); glEnd();
    glBindTexture(GL_TEXTURE_2D, texSky[3]); glBegin(GL_QUADS); glTexCoord2f(0, 0); glVertex3f(d, -d, d); glTexCoord2f(1, 0); glVertex3f(d, -d, -d); glTexCoord2f(1, 1); glVertex3f(d, d, -d); glTexCoord2f(0, 1); glVertex3f(d, d, d); glEnd();
    glBindTexture(GL_TEXTURE_2D, texSky[4]); glBegin(GL_QUADS); glTexCoord2f(0, 1); glVertex3f(-d, d, -d); glTexCoord2f(1, 1); glVertex3f(d, d, -d); glTexCoord2f(1, 0); glVertex3f(d, d, d); glTexCoord2f(0, 0); glVertex3f(-d, d, d); glEnd();
    glEnable(GL_LIGHTING);
}

float inaltimeTeren(float x, float z) {
    float distanta = sqrt(x * x + z * z);
    if (distanta < 40.0f) return 0.0f;
    return (sin(x * 0.15f) * cos(z * 0.15f)) * (distanta - 40.0f) * 0.25f;
}

void deseneazaPodeaRelief() {
    glBindTexture(GL_TEXTURE_2D, texIarba); glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_QUADS);
    for (float x = -100.0f; x < 100.0f; x += 2.0f) {
        for (float z = -100.0f; z < 100.0f; z += 2.0f) {
            float y1 = inaltimeTeren(x, z), y2 = inaltimeTeren(x + 2, z);
            float y3 = inaltimeTeren(x + 2, z + 2), y4 = inaltimeTeren(x, z + 2);
            float t = 0.2f; glNormal3f(0.0f, 1.0f, 0.0f);
            glTexCoord2f(x * t, z * t);         glVertex3f(x, y1, z);
            glTexCoord2f((x + 2) * t, z * t);     glVertex3f(x + 2, y2, z);
            glTexCoord2f((x + 2) * t, (z + 2) * t); glVertex3f(x + 2, y3, z + 2);
            glTexCoord2f(x * t, (z + 2) * t);     glVertex3f(x, y4, z + 2);
        }
    }
    glEnd();
}

void deseneazaCircuit() {
    glBindTexture(GL_TEXTURE_2D, texAsfalt); glColor3f(0.7f, 0.7f, 0.7f);
    glBegin(GL_QUAD_STRIP); glNormal3f(0.0f, 1.0f, 0.0f);
    for (int i = 0; i <= 60; i++) {
        float u = (2.0f * PI * i) / 60;
        float xIn = cos(u) * 20.0f, zIn = sin(u) * 20.0f;
        float xOut = cos(u) * 28.0f, zOut = sin(u) * 28.0f;
        glTexCoord2f(i * 0.5f, 0.0f); glVertex3f(xIn, 0.05f, zIn);
        glTexCoord2f(i * 0.5f, 1.0f); glVertex3f(xOut, 0.05f, zOut);
    }
    glEnd();
}

void deseneazaCladire(float x, float z, float l, float h, float a) {
    glPushMatrix(); glTranslatef(x, 0.0f, z);
    if (!desenamUmbre) { glBindTexture(GL_TEXTURE_2D, texCladire); glColor3f(0.9f, 0.9f, 0.9f); }
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1); glTexCoord2f(0, 0); glVertex3f(-l, 0, a); glTexCoord2f(1, 0); glVertex3f(l, 0, a); glTexCoord2f(1, 1); glVertex3f(l, h, a); glTexCoord2f(0, 1); glVertex3f(-l, h, a);
    glNormal3f(0, 0, -1); glTexCoord2f(0, 0); glVertex3f(-l, 0, -a); glTexCoord2f(1, 0); glVertex3f(l, 0, -a); glTexCoord2f(1, 1); glVertex3f(l, h, -a); glTexCoord2f(0, 1); glVertex3f(-l, h, -a);
    glNormal3f(-1, 0, 0); glTexCoord2f(0, 0); glVertex3f(-l, 0, -a); glTexCoord2f(1, 0); glVertex3f(-l, 0, a); glTexCoord2f(1, 1); glVertex3f(-l, h, a); glTexCoord2f(0, 1); glVertex3f(-l, h, -a);
    glNormal3f(1, 0, 0); glTexCoord2f(0, 0); glVertex3f(l, 0, -a); glTexCoord2f(1, 0); glVertex3f(l, 0, a); glTexCoord2f(1, 1); glVertex3f(l, h, a); glTexCoord2f(0, 1); glVertex3f(l, h, -a);
    glEnd();
    if (!desenamUmbre) { glDisable(GL_TEXTURE_2D); glColor3f(0.1f, 0.1f, 0.1f); }
    glBegin(GL_QUADS); glNormal3f(0, 1, 0); glVertex3f(-l, h, -a); glVertex3f(l, h, -a); glVertex3f(l, h, a); glVertex3f(-l, h, a); glEnd();
    if (!desenamUmbre) { glEnable(GL_TEXTURE_2D); }
    glPopMatrix();
}

void deseneazaCopac(float x, float z) {
    float inaltime = inaltimeTeren(x, z);
    glPushMatrix(); glTranslatef(x, inaltime, z);
    if (!desenamUmbre) glDisable(GL_TEXTURE_2D);
    if (!desenamUmbre) glColor3f(0.25f, 0.15f, 0.05f);
    glPushMatrix(); glRotatef(-90.0f, 1, 0, 0); glutSolidCylinder(0.3, 1.5, 8, 1); glPopMatrix();
    if (!desenamUmbre) glColor3f(0.05f, 0.35f, 0.05f);
    glTranslatef(0.0f, 1.5f, 0.0f); glPushMatrix(); glRotatef(-90.0f, 1, 0, 0); glutSolidCone(1.8, 4.0, 8, 1); glPopMatrix();
    if (!desenamUmbre) glEnable(GL_TEXTURE_2D);
    glPopMatrix();
}

void deseneazaStalp(float x, float z) {
    glPushMatrix(); glTranslatef(x, 0.0f, z);
    if (!desenamUmbre) glDisable(GL_TEXTURE_2D);
    if (!desenamUmbre) glColor3f(0.2f, 0.2f, 0.2f);
    glPushMatrix(); glRotatef(-90.0f, 1, 0, 0); glutSolidCylinder(0.12, 6.0, 8, 1); glPopMatrix();
    if (!desenamUmbre) {
        GLfloat emisie[] = { 1.0f, 0.9f, 0.4f, 1.0f }; glMaterialfv(GL_FRONT, GL_EMISSION, emisie);
        glColor3f(1.0f, 1.0f, 0.8f);
        glTranslatef(0.0f, 6.2f, 0.0f); glutSolidSphere(0.4, 10, 10);
        GLfloat normal[] = { 0.0f, 0.0f, 0.0f, 1.0f }; glMaterialfv(GL_FRONT, GL_EMISSION, normal);
    }
    if (!desenamUmbre) glEnable(GL_TEXTURE_2D);
    glPopMatrix();
}

// =========================================================================
// === [INCEPUT C2]: DESENARE OBIECTE IN MISCARE
// Cerinta: Adaugarea unor obiecte care se misca aleator/regula prestabilita
// =========================================================================

// 1. Desenez obiectul pe regula prestabilita (merge pe cerc mereu)
void deseneazaAutobuzAutonom() {
    float pozX = cos(unghiAutobuz) * 24.0f; // Il tin centrat pe raza 24 (pe banda)
    float pozZ = sin(unghiAutobuz) * 24.0f;

    glPushMatrix();
    glTranslatef(pozX, 1.2f, pozZ);
    // Il rotesc sa priveasca de-a lungul curbei matematice
    glRotatef(-unghiAutobuz * 180.0f / PI, 0, 1, 0);

    if (!desenamUmbre) glDisable(GL_TEXTURE_2D);
    if (!desenamUmbre) glColor3f(0.9f, 0.8f, 0.1f); // Autobuz galben

    glScalef(2.0f, 1.2f, 4.0f);
    glutSolidCube(1.0f); // Cutie simpla pentru autobuz

    if (!desenamUmbre) glEnable(GL_TEXTURE_2D);
    glPopMatrix();
}

// 2. Desenez dronele cu pozitii complet aleatoare calculate din memorie
void deseneazaDroneAleatoare() {
    if (!desenamUmbre) glDisable(GL_TEXTURE_2D);
    for (int i = 0; i < 15; i++) {
        glPushMatrix();
        glTranslatef(drone[i].x, drone[i].y, drone[i].z);
        if (!desenamUmbre) {
            GLfloat emisie[] = { 0.1f, 0.9f, 0.1f, 1.0f }; // Material luminos
            glMaterialfv(GL_FRONT, GL_EMISSION, emisie);
            glColor3f(0.2f, 1.0f, 0.2f);
        }
        glutSolidSphere(0.3f, 10, 10);
        if (!desenamUmbre) {
            GLfloat normal[] = { 0.0f, 0.0f, 0.0f, 1.0f };
            glMaterialfv(GL_FRONT, GL_EMISSION, normal);
        }
        glPopMatrix();
    }
    if (!desenamUmbre) glEnable(GL_TEXTURE_2D);
}
// === [SFARSIT C2]: DESENARE OBIECTE IN MISCARE ===========================


// =========================================================================
// === [INCEPUT C1]: DESENARE OBIECT CONTROLABIL (MASINA MAI COMPLEXA)
// Cerinta: Adaugarea unui obiect controlabil.
// =========================================================================
void deseneazaMasinaPlayer() {
    glPushMatrix();
    // Asez masina pe asfalt si adaug inaltimea terenului daca as urca un deal
    float inaltime = inaltimeTeren(masinaX, masinaZ);
    glTranslatef(masinaX, inaltime + 0.5f, masinaZ);

    // Rotatia masinii este setata de tastele Stanga/Dreapta
    glRotatef(masinaRot, 0, 1, 0);

    if (!desenamUmbre) glDisable(GL_TEXTURE_2D);

    // CAROSERIA (Baza masinii) - Culoare Rosie
    if (!desenamUmbre) glColor3f(0.8f, 0.1f, 0.1f);
    glPushMatrix();
    glTranslatef(0.0f, 0.4f, 0.0f);
    glScalef(1.8f, 0.5f, 3.5f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // CABINA MASINII (Putin mai mica, pe mijloc-spate) - Culoare Rosu inchis
    if (!desenamUmbre) glColor3f(0.5f, 0.05f, 0.05f);
    glPushMatrix();
    glTranslatef(0.0f, 0.9f, 0.2f);
    glScalef(1.4f, 0.6f, 1.8f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // ROTILE (4 cilindri sau sfere negre)
    if (!desenamUmbre) glColor3f(0.1f, 0.1f, 0.1f);
    // Roata stanga fata
    glPushMatrix(); glTranslatef(-1.0f, 0.3f, -1.2f); glutSolidSphere(0.4f, 10, 10); glPopMatrix();
    // Roata dreapta fata
    glPushMatrix(); glTranslatef(1.0f, 0.3f, -1.2f); glutSolidSphere(0.4f, 10, 10); glPopMatrix();
    // Roata stanga spate
    glPushMatrix(); glTranslatef(-1.0f, 0.3f, 1.2f); glutSolidSphere(0.4f, 10, 10); glPopMatrix();
    // Roata dreapta spate
    glPushMatrix(); glTranslatef(1.0f, 0.3f, 1.2f); glutSolidSphere(0.4f, 10, 10); glPopMatrix();

    if (!desenamUmbre) glEnable(GL_TEXTURE_2D);
    glPopMatrix();
}
// === [SFARSIT C1]: DESENARE OBIECT CONTROLABIL ===========================

void deseneazaGeometriaOrasului() {
    // Orasul static (Cladiri, copaci)
    for (int i = 0; i < 5; i++) { deseneazaCladire(cladiri[i].x, cladiri[i].z, cladiri[i].l, cladiri[i].h, cladiri[i].a); }
    for (int i = 0; i < 360; i += 30) { float u = i * (PI / 180.0f); deseneazaCopac(cos(u) * 33.0f, sin(u) * 33.0f); }

    // Apelul desenarii obiectelor pentru nota finala (C1 si C2)
    deseneazaAutobuzAutonom();
    deseneazaDroneAleatoare();
    deseneazaMasinaPlayer();
}

// =========================================================================
// === ACTUALIZARE LOGICA (APELARE COLIZIUNI C1 SI MISCARI C2)
// =========================================================================
void actualizeazaLogica() {
    if (isPaused) return;

    // --- [C2] Actualizam logica obiectelor din cerinta de Miscare ---
    unghiAutobuz += 0.015f;
    actualizeazaMiscareAleatoare();

    // --- Control Camera Libera (Spectator) cu WASD ---
    float yawRad = camYaw * (PI / 180.0f);
    float pitchRad = camPitch * (PI / 180.0f);
    float dirX = cos(yawRad) * cos(pitchRad);
    float dirY = sin(pitchRad);
    float dirZ = sin(yawRad) * cos(pitchRad);
    float rightX = cos(yawRad + PI / 2.0f);
    float rightZ = sin(yawRad + PI / 2.0f);

    if (keyW) { camX += dirX * vitezaCam; camY += dirY * vitezaCam; camZ += dirZ * vitezaCam; }
    if (keyS) { camX -= dirX * vitezaCam; camY -= dirY * vitezaCam; camZ -= dirZ * vitezaCam; }
    if (keyA) { camX -= rightX * vitezaCam; camZ -= rightZ * vitezaCam; }
    if (keyD) { camX += rightX * vitezaCam; camZ += rightZ * vitezaCam; }

    float inaltimeSol = inaltimeTeren(camX, camZ);
    if (camY < inaltimeSol + 1.0f) camY = inaltimeSol + 1.0f; // Evitam sa intram cu camera sub pamant


    // =========================================================================
    // --- [INCEPUT C1]: CONTROL MASINA SI COLIZIUNI ---
    // Cerinta: Control si Coliziune obiect masina cu cladirile.
    // =========================================================================

    // Se updateaza rotatia masinii pe loc daca apasam stanga/dreapta
    if (keyStanga) masinaRot += 4.0f;
    if (keyDreapta) masinaRot -= 4.0f;

    // Calculam in ce directie vrea masina sa mearga bazat pe unghiul rotit
    float dirMasinaX = sin(masinaRot * PI / 180.0f);
    float dirMasinaZ = cos(masinaRot * PI / 180.0f);

    // Presupunem ca va face pasul in aceste variabile next
    float nextMasinaX = masinaX;
    float nextMasinaZ = masinaZ;

    // Daca apasam Sus/Jos, punem intentia de miscare in nextMasinaX si Z
    if (keySus) { nextMasinaX += dirMasinaX * vitezaMasina; nextMasinaZ += dirMasinaZ * vitezaMasina; }
    if (keyJos) { nextMasinaX -= dirMasinaX * vitezaMasina; nextMasinaZ -= dirMasinaZ * vitezaMasina; }

    // Test de COLIZIUNE! Separat pe axa X si axa Z.
    // De ce separat? Ca sa alunecam de-a lungul peretelui daca lovim cladirea la un unghi.
    float razaMasina = 1.8f;

    // Daca nu ma lovesc in viitorul pas pe X, aprob schimbarea
    if (!verificaColiziune(nextMasinaX, masinaZ, razaMasina)) {
        masinaX = nextMasinaX;
    }
    // Daca nu ma lovesc in viitorul pas pe Z, aprob schimbarea
    if (!verificaColiziune(masinaX, nextMasinaZ, razaMasina)) {
        masinaZ = nextMasinaZ;
    }
    // =========================================================================
    // --- [SFARSIT C1]: LOGICA CONTROL + COLIZIUNE ---
    // =========================================================================
}

// ==========================================
// RANDARE SCENA GLOBALA
// ==========================================
void renderScene(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); glLoadIdentity();

    actualizeazaLogica();

    // Orientam camera in spatiu
    float focusX = camX + cos(camYaw * PI / 180) * cos(camPitch * PI / 180);
    float focusY = camY + sin(camPitch * PI / 180);
    float focusZ = camZ + sin(camYaw * PI / 180) * cos(camPitch * PI / 180);
    gluLookAt(camX, camY, camZ, focusX, focusY, focusZ, 0, 1, 0);

    GLfloat lunaPos[] = { -60.0f, 80.0f, -40.0f, 0.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lunaPos);

    int lightID = GL_LIGHT1;
    for (int i = 0; i < 7; i++) {
        GLfloat pozLumina[] = { posStalpi[i][0], 6.2f, posStalpi[i][1], 1.0f };
        glLightfv(lightID, GL_POSITION, pozLumina);
        lightID++;
    }

    deseneazaSkybox();
    deseneazaPodeaRelief();
    deseneazaCircuit();

    // Aici se deseneaza cladiri, drone(C2), autobuz(C2) si masina(C1)
    deseneazaGeometriaOrasului();

    for (int i = 0; i < 7; i++) { deseneazaStalp(posStalpi[i][0], posStalpi[i][1]); }

    // RANDEAZA UMBRELE MULTIPLE
    desenamUmbre = true;
    glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D);
    float mat[16];

    glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
    glPushMatrix();
    glTranslatef(0, 0.1f, 0);
    calculeazaMatriceUmbra(mat, lunaPos, 0.0f); glMultMatrixf(mat);
    deseneazaGeometriaOrasului();
    for (int i = 0; i < 7; i++) deseneazaStalp(posStalpi[i][0], posStalpi[i][1]);
    glPopMatrix();

    glColor4f(0.0f, 0.0f, 0.0f, 0.3f);
    GLfloat pozStalpA[] = { posStalpi[0][0], 6.2f, posStalpi[0][1], 1.0f };
    glPushMatrix();
    glTranslatef(0, 0.12f, 0);
    calculeazaMatriceUmbra(mat, pozStalpA, 0.0f); glMultMatrixf(mat);
    deseneazaGeometriaOrasului();
    glPopMatrix();

    GLfloat pozStalpB[] = { posStalpi[3][0], 6.2f, posStalpi[3][1], 1.0f };
    glPushMatrix();
    glTranslatef(0, 0.14f, 0);
    calculeazaMatriceUmbra(mat, pozStalpB, 0.0f); glMultMatrixf(mat);
    deseneazaGeometriaOrasului();
    glPopMatrix();

    desenamUmbre = false;
    glEnable(GL_LIGHTING); glEnable(GL_TEXTURE_2D);

    if (isPaused) {
        glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, 1024, 0, 768);
        glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
        glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
        glColor4f(0, 0, 0, 0.7f); glBegin(GL_QUADS); glVertex2f(0, 0); glVertex2f(1024, 0); glVertex2f(1024, 768); glVertex2f(0, 768); glEnd();
        glColor3f(1, 0.8f, 0); glRasterPos2i(350, 400);
        const char* txt = "PAUZA: Apasa ESC pentru a continua.";
        for (const char* c = txt; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
        glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
    }

    glutSwapBuffers();
}

// ==========================================
// TASTE NORMALE (W, A, S, D, ESC) pentru Camera
// ==========================================
void keyDown(unsigned char key, int x, int y) {
    if (key == 27) {
        isPaused = !isPaused;
        if (isPaused) glutSetCursor(GLUT_CURSOR_INHERIT);
        else { glutSetCursor(GLUT_CURSOR_NONE); glutWarpPointer(centruX, centruY); }
    }
    if (key == 'w' || key == 'W') keyW = true; if (key == 's' || key == 'S') keyS = true;
    if (key == 'a' || key == 'A') keyA = true; if (key == 'd' || key == 'D') keyD = true;
}
void keyUp(unsigned char key, int x, int y) {
    if (key == 'w' || key == 'W') keyW = false; if (key == 's' || key == 'S') keyS = false;
    if (key == 'a' || key == 'A') keyA = false; if (key == 'd' || key == 'D') keyD = false;
}

// =========================================================================
// === [INCEPUT C1]: CONTROL TASTE SPECIALE MASINA
// =========================================================================
// Functia inregistreaza tastele Sageti (Sus, Jos, Stanga, Dreapta) pentru masina
void specialDown(int key, int x, int y) {
    if (key == GLUT_KEY_UP) keySus = true;
    if (key == GLUT_KEY_DOWN) keyJos = true;
    if (key == GLUT_KEY_LEFT) keyStanga = true;
    if (key == GLUT_KEY_RIGHT) keyDreapta = true;
}
void specialUp(int key, int x, int y) {
    if (key == GLUT_KEY_UP) keySus = false;
    if (key == GLUT_KEY_DOWN) keyJos = false;
    if (key == GLUT_KEY_LEFT) keyStanga = false;
    if (key == GLUT_KEY_RIGHT) keyDreapta = false;
}
// === [SFARSIT C1]: TASTE SPECIALE MASINA =================================

void mouseMove(int x, int y) {
    if (isPaused || (x == centruX && y == centruY)) return;
    camYaw += (x - centruX) * 0.15f; camPitch -= (y - centruY) * 0.15f;
    if (camPitch > 89.0f) camPitch = 89.0f; if (camPitch < -89.0f) camPitch = -89.0f;
    glutWarpPointer(centruX, centruY);
}
void resize(int w, int h) {
    if (h == 0) h = 1; centruX = w / 2; centruY = h / 2;
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glViewport(0, 0, w, h);
    gluPerspective(60.0f, (float)w / h, 0.1f, 400.0f); glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_ALPHA);
    glutInitWindowSize(1024, 768);
    glutCreateWindow("Proiect Grafica - Oras Complet (C1+C2)");

    initializare();

    glutDisplayFunc(renderScene);
    glutReshapeFunc(resize);
    glutIdleFunc([]() { glutPostRedisplay(); });

    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);

    // Initializare functii pentru tastele sageata (Control Masina)
    glutSpecialFunc(specialDown);
    glutSpecialUpFunc(specialUp);

    glutPassiveMotionFunc(mouseMove);

    glutMainLoop();
    return 0;
}