#include <GL/freeglut.h>
#include <iostream>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define PI 3.14159265f

// =========================================================================
// === INCEPUT P3: VARIABILE PENTRU CONTROL CAMERA, INPUT SI STARE JOC ===
// =========================================================================
// Pozitia camerei in scena
float camX = 0.0f, camY = 4.0f, camZ = 35.0f;
// Unghiurile pentru a calcula directia privirii (ca intr-un joc FPS)
float camYaw = -90.0f, camPitch = 0.0f;
float viteza = 0.8f;

// Retinem starea tastelor (apasat/eliberat) pentru miscare fluida
bool keyW = false, keyS = false, keyA = false, keyD = false;

// Coordonatele centrului ferestrei pentru a bloca mouse-ul (Pointer Lock)
int centruX = 512, centruY = 384;
bool isPaused = false;

// Steag pentru a sti cand sa dezactivam texturile/luminile ca sa desenam doar pete negre (umbre)
bool desenamUmbre = false;
// =========================================================================
// === SFARSIT VARIABILE P3 ===
// =========================================================================


// Locatii pentru obiectele statice (Stalpi)
float posStalpi[8][2];
GLuint texIarba, texAsfalt, texCladire, texSky[6];

// ==========================================
// FUNCTII UTILE (INCARCARE TEXTURI)
// ==========================================
// Incarca pozele. Daca fisierul lipseste, generez un pixel colorat de rezerva sa nu pice programul
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

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (repeta) { glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); }
    else { glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F); }

    return texID;
}

// =========================================================================
// === INCEPUT P3: CALCUL MATRICE PENTRU UMBRE PLANARE ===
// =========================================================================
// Aceasta functie calculeaza matricea care "turteste" geometria scenei pe planul p?mântului (Y=inaltimePlan)
// Folosesc ecuatia planului si pozitia sursei de lumina (Directionala sau Punctiforma)
void calculeazaMatriceUmbra(float mat[16], float lumina[4], float inaltimePlan) {
    float plan[4] = { 0.0f, 1.0f, 0.0f, -inaltimePlan };
    float dot = plan[0] * lumina[0] + plan[1] * lumina[1] + plan[2] * lumina[2] + plan[3] * lumina[3];

    mat[0] = dot - lumina[0] * plan[0]; mat[4] = 0.f - lumina[0] * plan[1]; mat[8] = 0.f - lumina[0] * plan[2]; mat[12] = 0.f - lumina[0] * plan[3];
    mat[1] = 0.f - lumina[1] * plan[0]; mat[5] = dot - lumina[1] * plan[1]; mat[9] = 0.f - lumina[1] * plan[2]; mat[13] = 0.f - lumina[1] * plan[3];
    mat[2] = 0.f - lumina[2] * plan[0]; mat[6] = 0.f - lumina[2] * plan[1]; mat[10] = dot - lumina[2] * plan[2]; mat[14] = 0.f - lumina[2] * plan[3];
    mat[3] = 0.f - lumina[3] * plan[0]; mat[7] = 0.f - lumina[3] * plan[1]; mat[11] = 0.f - lumina[3] * plan[2]; mat[15] = dot - lumina[3] * plan[3];
}
// =========================================================================
// === SFARSIT P3 (CALCUL MATRICE UMBRA) ===
// =========================================================================

// =========================================================================
// === INCEPUT P3: INITIALIZARE LUMINAT HARDWARE ===
// =========================================================================
void initializareLumina() {
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // Am setat LIGHT0 ca lumina globala (LUNA). Aceasta genereaza lumina ambientala si difuza.
    glEnable(GL_LIGHT0);
    GLfloat ambientSoare[] = { 0.1f, 0.1f, 0.15f, 1.0f };
    GLfloat difuzSoare[] = { 0.6f, 0.65f, 0.75f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientSoare);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, difuzSoare);
}
// =========================================================================
// === SFARSIT P3: INITIALIZARE LUMINAT ===
// =========================================================================

void initializare() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Corectie pentru Z-Fighting la desenarea umbrelor pe suprafete plate
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0, 1.0);

    initializareLumina();

    // P1 + P2: Incarcarea texturilor
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

    // Calcularea pozitiilor pe cerc pentru stalpii de iluminat
    for (int i = 0; i < 8; i++) {
        float u = (i * 45.0f) * (PI / 180.0f);
        posStalpi[i][0] = cos(u) * 18.0f;
        posStalpi[i][1] = sin(u) * 18.0f;
    }

    // [P3] Ascund mouse-ul la pornire pentru modul First Person
    glutSetCursor(GLUT_CURSOR_NONE);
}

// ==========================================
// [P1] MEDIU SI RELIEF (TEREN)
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

// Generez relieful matematic la periferie, pastrand centrul plat pentru circuit
float inaltimeTeren(float x, float z) {
    float distanta = sqrt(x * x + z * z);
    if (distanta < 40.0f) return 0.0f;
    return (sin(x * 0.15f) * cos(z * 0.15f)) * (distanta - 40.0f) * 0.25f;
}

void deseneazaPodeaRelief() {
    glBindTexture(GL_TEXTURE_2D, texIarba);
    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_QUADS);
    for (float x = -100.0f; x < 100.0f; x += 2.0f) {
        for (float z = -100.0f; z < 100.0f; z += 2.0f) {
            float y1 = inaltimeTeren(x, z), y2 = inaltimeTeren(x + 2, z);
            float y3 = inaltimeTeren(x + 2, z + 2), y4 = inaltimeTeren(x, z + 2);
            float t = 0.2f;
            glNormal3f(0.0f, 1.0f, 0.0f);
            glTexCoord2f(x * t, z * t);         glVertex3f(x, y1, z);
            glTexCoord2f((x + 2) * t, z * t);     glVertex3f(x + 2, y2, z);
            glTexCoord2f((x + 2) * t, (z + 2) * t); glVertex3f(x + 2, y3, z + 2);
            glTexCoord2f(x * t, (z + 2) * t);     glVertex3f(x, y4, z + 2);
        }
    }
    glEnd();
}

// ==========================================
// [P2] CIRCUIT SI OBIECTE (Oras, Copaci, Stalpi)
// ==========================================
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

    // Tavanul
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

    // Becul stalpului. Are material emisiv ca sa straluceasca in intuneric fara a genera lumina falsa
    if (!desenamUmbre) {
        GLfloat emisie[] = { 1.0f, 0.9f, 0.4f, 1.0f };
        glMaterialfv(GL_FRONT, GL_EMISSION, emisie);
        glColor3f(1.0f, 1.0f, 0.8f);

        glTranslatef(0.0f, 6.2f, 0.0f); glutSolidSphere(0.4, 10, 10);

        // Resetam materialul dupa bec
        GLfloat normal[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        glMaterialfv(GL_FRONT, GL_EMISSION, normal);
    }
    if (!desenamUmbre) glEnable(GL_TEXTURE_2D);
    glPopMatrix();
}

// Grup de obiecte pentru care calculam umbrele
void deseneazaLumeaStatica() {
    deseneazaCladire(0, 0, 3, 10, 3);
    deseneazaCladire(8, 5, 2, 8, 2);
    deseneazaCladire(-7, -8, 2.5f, 6, 4);
    deseneazaCladire(-8, 8, 2, 7, 2);
    deseneazaCladire(8, -8, 2, 5, 3);

    for (int i = 0; i < 360; i += 30) {
        float u = i * (PI / 180.0f);
        deseneazaCopac(cos(u) * 33.0f, sin(u) * 33.0f);
    }
}


// =========================================================================
// === INCEPUT P3: ACTUALIZARE CAMERA FPS ===
// =========================================================================
// Convertesc unghiurile Yaw/Pitch in coordonate 3D pentru directie.
// Astfel, tasta W adauga vectorul directie la pozitia mea (merge fix inainte).
void actualizeazaCamera() {
    if (isPaused) return;

    float yawRad = camYaw * (PI / 180.0f);
    float pitchRad = camPitch * (PI / 180.0f);

    // Directia de privire (vectorul FORWARD)
    float dirX = cos(yawRad) * cos(pitchRad);
    float dirY = sin(pitchRad);
    float dirZ = sin(yawRad) * cos(pitchRad);

    // Directia laterala (vectorul RIGHT), derivata din yaw adaugand 90 de grade
    float rightX = cos(yawRad + PI / 2.0f);
    float rightZ = sin(yawRad + PI / 2.0f);

    if (keyW) { camX += dirX * viteza; camY += dirY * viteza; camZ += dirZ * viteza; }
    if (keyS) { camX -= dirX * viteza; camY -= dirY * viteza; camZ -= dirZ * viteza; }
    if (keyA) { camX -= rightX * viteza; camZ -= rightZ * viteza; }
    if (keyD) { camX += rightX * viteza; camZ += rightZ * viteza; }

    // Coliziune simpla cu terenul
    float inaltimeSol = inaltimeTeren(camX, camZ);
    if (camY < inaltimeSol + 2.0f) camY = inaltimeSol + 2.0f;

    float focusX = camX + dirX, focusY = camY + dirY, focusZ = camZ + dirZ;
    gluLookAt(camX, camY, camZ, focusX, focusY, focusZ, 0, 1, 0);
}
// =========================================================================
// === SFARSIT P3: CAMERA ===
// =========================================================================


// =========================================================================
// === INCEPUT P3: RENDER (ILUMINARE SI UMBRE MULTIPLE) ===
// =========================================================================
void renderScene(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); glLoadIdentity();

    actualizeazaCamera();

    // 1. Pozitionarea Sursei Globale de Lumina (LUNA). 
    // Am folosit w=0.0 pentru lumina directionala, astfel incat razele sunt paralele si umbrele sunt realiste
    GLfloat lunaPos[] = { -60.0f, 80.0f, -40.0f, 0.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lunaPos);

    // Am mai adaugat lumini hardware locale punctiforme la baza catorva stalpi (pentru umbre suplimentare pe cladiri/asfalt)
    // Pentru a respecta cerinta umbrelor multiple
    GLfloat pozStalpLumina1[] = { posStalpi[0][0], 6.2f, posStalpi[0][1], 1.0f };
    glEnable(GL_LIGHT1);
    GLfloat luminaCalda[] = { 0.8f, 0.6f, 0.2f, 1.0f }; glLightfv(GL_LIGHT1, GL_DIFFUSE, luminaCalda);
    glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 0.5f);
    glLightfv(GL_LIGHT1, GL_POSITION, pozStalpLumina1);

    GLfloat pozStalpLumina2[] = { posStalpi[3][0], 6.2f, posStalpi[3][1], 1.0f };
    glEnable(GL_LIGHT2);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, luminaCalda);
    glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, 0.5f);
    glLightfv(GL_LIGHT2, GL_POSITION, pozStalpLumina2);

    // Desenarea Scenei in conditii normale
    deseneazaSkybox();
    deseneazaPodeaRelief();
    deseneazaCircuit();
    deseneazaLumeaStatica();

    // Toti stalpii se deseneaza (unii contin lumini hardware pe ei)
    for (int i = 0; i < 8; i++) {
        deseneazaStalp(posStalpi[i][0], posStalpi[i][1]);
    }

    // --- CONSTRUCTIA UMBRELOR MULTIPLE ---
    desenamUmbre = true; // Blochez texturile
    glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D);
    float mat[16];

    // Umbra de la sursa directionala (LUNA) - Groasa, neagra
    glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
    glPushMatrix();
    glTranslatef(0, 0.1f, 0); // Putina inaltare pt a evita Z-fighting cu solul
    calculeazaMatriceUmbra(mat, lunaPos, 0.0f); glMultMatrixf(mat);
    deseneazaLumeaStatica();
    for (int i = 0; i < 8; i++) deseneazaStalp(posStalpi[i][0], posStalpi[i][1]); // Stalpii arunca doar umbra de la luna
    glPopMatrix();

    // Umbre Secundare generate de luminile locale ale stalpilor (Umbre Multiple)
    glColor4f(0.0f, 0.0f, 0.0f, 0.3f); // Opacitate mai mica
    glPushMatrix();
    glTranslatef(0, 0.11f, 0);
    calculeazaMatriceUmbra(mat, pozStalpLumina1, 0.0f); glMultMatrixf(mat);
    deseneazaLumeaStatica();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0, 0.12f, 0);
    calculeazaMatriceUmbra(mat, pozStalpLumina2, 0.0f); glMultMatrixf(mat);
    deseneazaLumeaStatica();
    glPopMatrix();

    desenamUmbre = false;
    glEnable(GL_LIGHTING); glEnable(GL_TEXTURE_2D);

    // Daca apasam ESC, apare meniul de PAUZA (Text 2D ortografic suprapus)
    if (isPaused) {
        glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, 1024, 0, 768);
        glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
        glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);

        glColor4f(0, 0, 0, 0.7f);
        glBegin(GL_QUADS); glVertex2f(0, 0); glVertex2f(1024, 0); glVertex2f(1024, 768); glVertex2f(0, 768); glEnd();

        glColor3f(1, 0.8f, 0); glRasterPos2i(350, 400);
        const char* txt = "PAUZA: Apasa ESC pentru a continua.";
        for (const char* c = txt; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

        glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
        glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
    }

    glutSwapBuffers();
}
// =========================================================================
// === SFARSIT P3 (RENDER) ===
// =========================================================================

// =========================================================================
// === INCEPUT P3: MOUSE LOCK & TASTATURA ===
// =========================================================================
void keyDown(unsigned char key, int x, int y) {
    if (key == 27) { // Am folosit codul ASCII 27 pt ESCAPE
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

// Calculez orientarea camerei bazat pe cat de mult deviaza mouse-ul de la centrul ecranului
void mouseMove(int x, int y) {
    if (isPaused || (x == centruX && y == centruY)) return;

    camYaw += (x - centruX) * 0.15f;
    camPitch -= (y - centruY) * 0.15f;

    if (camPitch > 89.0f) camPitch = 89.0f; if (camPitch < -89.0f) camPitch = -89.0f;

    // Mentin mouse-ul prins pe centrul ferestrei
    glutWarpPointer(centruX, centruY);
}
// =========================================================================
// === SFARSIT P3: INPUT ===
// =========================================================================

void resize(int w, int h) {
    if (h == 0) h = 1; centruX = w / 2; centruY = h / 2;
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glViewport(0, 0, w, h);
    gluPerspective(60.0f, (float)w / h, 0.1f, 400.0f); glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_ALPHA);
    glutInitWindowSize(1024, 768);
    glutCreateWindow("Proiect Grafica 3D");

    initializare();

    glutDisplayFunc(renderScene);
    glutReshapeFunc(resize);
    glutIdleFunc([]() { glutPostRedisplay(); });
    glutKeyboardFunc(keyDown); glutKeyboardUpFunc(keyUp);
    glutPassiveMotionFunc(mouseMove);

    glutMainLoop();
    return 0;
}