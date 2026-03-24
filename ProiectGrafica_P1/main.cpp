#include <GL/freeglut.h>
#include <iostream>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define PI 3.14159265f

// --- VARIABILELE GLOBALE ---
float unghiCamera = 0.0f;
float razaCamera = 60.0f;

// Texturi
GLuint texturaIarba;
GLuint texturaSkybox[6];
GLuint texturaAsfalt;
GLuint texturaCladire;

// Heightmap
unsigned char* heightmapData = nullptr;
int hmLatime, hmInaltime, hmCanale;

// --- INCARCAREA TEXTURILOR (Varianta ultra-sigura pentru memorie) ---
GLuint incarcaTextura(const char* numeFisier, bool repeta = true) {
    GLuint texturaID;
    glGenTextures(1, &texturaID);
    glBindTexture(GL_TEXTURE_2D, texturaID);

    int width, height, nrChannels;

    // Fortam 4 canale (RGBA) pentru a evita orice eroare de memorie cu poze ciudate
    unsigned char* data = stbi_load(numeFisier, &width, &height, &nrChannels, 4);

    if (data) {
        // PREVINE CRASH-UL (Eroarea 0xc0000005)
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (repeta) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
        else {
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
    }
    else {
        std::cout << "Eroare: Nu am gasit fisierul -> " << numeFisier << std::endl;
    }

    stbi_image_free(data);
    return texturaID;
}

void initializare() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    std::cout << "-> Incarc textura iarba..." << std::endl;
    stbi_set_flip_vertically_on_load(true);
    texturaIarba = incarcaTextura("iarba.jpg", true);

    std::cout << "-> Incarc texturile noi (asfalt, cladire)..." << std::endl;
    texturaAsfalt = incarcaTextura("asfalt.jpg", true);
    texturaCladire = incarcaTextura("cladire.jpg", true);

    std::cout << "-> Incarc Skybox-ul..." << std::endl;
    stbi_set_flip_vertically_on_load(false);
    texturaSkybox[0] = incarcaTextura("Daylight Box_Front.bmp", false);
    texturaSkybox[1] = incarcaTextura("Daylight Box_Back.bmp", false);
    texturaSkybox[2] = incarcaTextura("Daylight Box_Left.bmp", false);
    texturaSkybox[3] = incarcaTextura("Daylight Box_Right.bmp", false);
    texturaSkybox[4] = incarcaTextura("Daylight Box_Top.bmp", false);
    texturaSkybox[5] = incarcaTextura("Daylight Box_Bottom.bmp", false);

    std::cout << "-> Incarc harta de relief..." << std::endl;
    stbi_set_flip_vertically_on_load(true);
    heightmapData = stbi_load("Rolling Hills Height Map.png", &hmLatime, &hmInaltime, &hmCanale, 1);
    if (!heightmapData) {
        std::cout << "Eroare: Nu am gasit 'Rolling Hills Height Map.png'!" << std::endl;
    }
}

// --- FUNCTIA CARE IMI DA INALTIMEA TERENULUI ---
float obtineInaltimeDinHarta(float x, float z) {
    if (!heightmapData) return 0.0f;

    float u = (x + 50.0f) / 100.0f;
    float v = (z + 50.0f) / 100.0f;

    int px = u * (hmLatime - 1);
    int pz = v * (hmInaltime - 1);

    if (px < 0) px = 0; if (px >= hmLatime) px = hmLatime - 1;
    if (pz < 0) pz = 0; if (pz >= hmInaltime) pz = hmInaltime - 1;

    unsigned char culoarePixel = heightmapData[pz * hmLatime + px];
    return (culoarePixel / 255.0f) * 15.0f;
}

// ==========================================
// P2: NOILE COMPONENTE (CIRCUIT SI OBIECTE)
// ==========================================

void deseneazaCircuit() {
    glBindTexture(GL_TEXTURE_2D, texturaAsfalt);
    glColor3f(1.0f, 1.0f, 1.0f);

    float razaMica = 25.0f;
    float razaMare = 32.0f;
    int segmente = 100;

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segmente; i++) {
        float unghi = (2.0f * PI * i) / segmente;

        float xIn = cos(unghi) * razaMica;
        float zIn = sin(unghi) * razaMica;
        float xOut = cos(unghi) * razaMare;
        float zOut = sin(unghi) * razaMare;

        // Ridicam circuitul mai sus (0.8f) ca sa evitam intrepatrunderea ierbii cu asfaltul
        float yIn = obtineInaltimeDinHarta(xIn, zIn) + 0.8f;
        float yOut = obtineInaltimeDinHarta(xOut, zOut) + 0.8f;

        glTexCoord2f(i * 0.2f, 0.0f); glVertex3f(xIn, yIn, zIn);
        glTexCoord2f(i * 0.2f, 1.0f); glVertex3f(xOut, yOut, zOut);
    }
    glEnd();
}

void deseneazaCopac(float x, float z) {
    float y = obtineInaltimeDinHarta(x, z);

    glPushMatrix();
    glTranslatef(x, y, z);

    glDisable(GL_TEXTURE_2D);

    // Trunchiul copacului
    glColor3f(0.4f, 0.2f, 0.0f);
    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    glutSolidCone(0.5, 2.0, 10, 2);
    glPopMatrix();

    // Coroana bradului
    glColor3f(0.1f, 0.5f, 0.1f);
    glTranslatef(0.0f, 1.5f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    glutSolidCone(2.0, 4.0, 10, 2);

    glEnable(GL_TEXTURE_2D);
    glPopMatrix();
}

void deseneazaCladire(float x, float z, float latime, float inaltime, float adancime) {
    float y = obtineInaltimeDinHarta(x, z);

    glBindTexture(GL_TEXTURE_2D, texturaCladire);
    glColor3f(1.0f, 1.0f, 1.0f);

    glPushMatrix();
    glTranslatef(x, y, z);

    // Peretii cladirii
    glBegin(GL_QUADS);
    // Fata
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-latime, 0, adancime);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(latime, 0, adancime);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(latime, inaltime, adancime);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-latime, inaltime, adancime);
    // Spate
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-latime, 0, -adancime);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(latime, 0, -adancime);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(latime, inaltime, -adancime);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-latime, inaltime, -adancime);
    // Stanga
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-latime, 0, -adancime);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-latime, 0, adancime);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-latime, inaltime, adancime);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-latime, inaltime, -adancime);
    // Dreapta
    glTexCoord2f(0.0f, 0.0f); glVertex3f(latime, 0, -adancime);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(latime, 0, adancime);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(latime, inaltime, adancime);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(latime, inaltime, -adancime);
    glEnd();

    // Acoperisul 
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.2f, 0.2f, 0.2f); // Culoare gri inchis
    glBegin(GL_QUADS);
    glVertex3f(-latime, inaltime, -adancime);
    glVertex3f(latime, inaltime, -adancime);
    glVertex3f(latime, inaltime, adancime);
    glVertex3f(-latime, inaltime, adancime);
    glEnd();

    // Pornim pozele la loc pentru restul lumii si resetam culoarea
    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);

    glPopMatrix();
}

void deseneazaLumeaStatica() {
    // Cladiri
    deseneazaCladire(0.0f, 0.0f, 3.0f, 10.0f, 3.0f);
    deseneazaCladire(15.0f, 5.0f, 2.0f, 6.0f, 2.0f);
    deseneazaCladire(-10.0f, 15.0f, 4.0f, 5.0f, 4.0f);
    deseneazaCladire(-38.0f, -38.0f, 2.5f, 8.0f, 2.5f);
    deseneazaCladire(40.0f, -10.0f, 2.0f, 7.0f, 3.0f);

    // Copaci
    deseneazaCopac(5.0f, -10.0f);
    deseneazaCopac(8.0f, -12.0f);
    deseneazaCopac(6.0f, -14.0f);

    deseneazaCopac(-20.0f, -5.0f);
    deseneazaCopac(-40.0f, 20.0f);
    deseneazaCopac(35.0f, 35.0f);
    deseneazaCopac(-5.0f, 40.0f);
}

// ==========================================
// P1: BAZA VECHE (SKYBOX SI TEREN)
// ==========================================

void deseneazaSkybox() {
    float d = 100.0f;
    glColor3f(1.0f, 1.0f, 1.0f);

    glBindTexture(GL_TEXTURE_2D, texturaSkybox[1]);
    glBegin(GL_QUADS);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-d, -d, -d);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(d, -d, -d);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(d, d, -d);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-d, d, -d);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, texturaSkybox[0]);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-d, -d, d);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(d, -d, d);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(d, d, d);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-d, d, d);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, texturaSkybox[2]);
    glBegin(GL_QUADS);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-d, -d, d);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-d, -d, -d);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-d, d, -d);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-d, d, d);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, texturaSkybox[3]);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(d, -d, d);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(d, -d, -d);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(d, d, -d);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(d, d, d);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, texturaSkybox[4]);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-d, d, -d);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(d, d, -d);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(d, d, d);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-d, d, d);
    glEnd();
}

void deseneazaRelief() {
    glBindTexture(GL_TEXTURE_2D, texturaIarba);
    glColor3f(1.0f, 1.0f, 1.0f);

    glBegin(GL_QUADS);
    for (float x = -50.0f; x < 50.0f; x += 2.0f) {
        for (float z = -50.0f; z < 50.0f; z += 2.0f) {

            float y1 = obtineInaltimeDinHarta(x, z);
            float y2 = obtineInaltimeDinHarta(x + 2.0f, z);
            float y3 = obtineInaltimeDinHarta(x + 2.0f, z + 2.0f);
            float y4 = obtineInaltimeDinHarta(x, z + 2.0f);

            float texScale = 10.0f / 100.0f;

            glTexCoord2f((x + 50) * texScale, (z + 50) * texScale); glVertex3f(x, y1, z);
            glTexCoord2f((x + 52) * texScale, (z + 50) * texScale); glVertex3f(x + 2.0f, y2, z);
            glTexCoord2f((x + 52) * texScale, (z + 52) * texScale); glVertex3f(x + 2.0f, y3, z + 2.0f);
            glTexCoord2f((x + 50) * texScale, (z + 52) * texScale); glVertex3f(x, y4, z + 2.0f);
        }
    }
    glEnd();
}

// --- FUNCTIA CENTRALA DE DESENARE A CADRULUI ---
void renderScene(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    float camX = sin(unghiCamera) * razaCamera;
    float camZ = cos(unghiCamera) * razaCamera;
    float camY = 35.0f;

    gluLookAt(
        camX, camY, camZ,
        0.0f, 5.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    );

    deseneazaSkybox();
    deseneazaRelief();
    deseneazaCircuit();
    deseneazaLumeaStatica();

    glutSwapBuffers();
}

void changeSize(int w, int h) {
    if (h == 0) h = 1;
    float ratio = w * 1.0 / h;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, w, h);
    gluPerspective(45.0f, ratio, 0.1f, 250.0f);
    glMatrixMode(GL_MODELVIEW);
}

void animate() {
    unghiCamera += 0.002f;
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    std::cout << "[1] Programul a pornit cu succes!" << std::endl;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(1024, 768);
    glutCreateWindow("Proiect Grafica - P1 + P2");

    std::cout << "[2] Fereastra creata. Incep incarcarea pozelor..." << std::endl;

    initializare();

    std::cout << "[3] Toate pozele incarcate. Pornesc bucla 3D!" << std::endl;

    glutDisplayFunc(renderScene);
    glutReshapeFunc(changeSize);
    glutIdleFunc(animate);

    glutMainLoop();
    return 0;
}