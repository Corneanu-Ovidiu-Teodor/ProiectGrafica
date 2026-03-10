#include <GL/freeglut.h>
#include <iostream>
#include <math.h>

// Asta include biblioteca stb_image direct in cod, fara fisiere .lib externe!
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// --- VARIABILELE MELE GLOBALE ---
// La P3 o sa folosesc variabilele astea ca sa ma plimb pe circuit cu camera. 
// Momentan doar se invarte in cerc ca sa vad P1.
float unghiCamera = 0.0f;
float razaCamera = 60.0f;

// Aici salvez ID-urile pentru pozele incarcate in placa video
GLuint texturaIarba;
GLuint texturaSkybox[6]; // Vector pentru cele 6 fete ale cerului (cubului)

// Aici tin minte datele despre poza alb-negru din care generez dealurile
unsigned char* heightmapData = nullptr;
int hmLatime, hmInaltime, hmCanale;

// --- FUNCTIA CARE IMI INCARCA O POZA DE PE DISK IN OPENGL ---
// 'repeta' e true la iarba (ca sa fie gresie la infinit) si false la cer (ca sa intind marginile)
GLuint incarcaTextura(const char* numeFisier, bool repeta = true) {
    GLuint texturaID;
    glGenTextures(1, &texturaID);
    glBindTexture(GL_TEXTURE_2D, texturaID);

    int width, height, nrChannels;
    // Citesc poza efectiv
    unsigned char* data = stbi_load(numeFisier, &width, &height, &nrChannels, 0);

    if (data) {
        // Imi dau seama daca poza are sau nu transparenta (Alpha)
        GLenum format = GL_RGB;
        if (nrChannels == 4) format = GL_RGBA;
        if (nrChannels == 1) format = GL_LUMINANCE; // Pentru poze alb-negru

        // Trimit poza in placa video
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

        // Filtrare: blureaza finut pixelii ca sa nu se vada patratos cand ma apropii
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (repeta) {
            // Repet poza (GL_REPEAT)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
        else {
            // Intind poza spre margini (GL_CLAMP_TO_EDGE) - Definit manual in caz ca crapa pe PC-uri vechi
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
    }
    else {
        // Daca am uitat sa pun poza in folder sau am gresit numele, imi da eroare in consola
        std::cout << "Eroare: Nu am gasit fisierul -> " << numeFisier << std::endl;
    }
    stbi_image_free(data); // Eliberez memoria RAM ca sa nu crape PC-ul
    return texturaID;
}

// --- AICI INCARC TOATE ASSET-URILE LA INCEPUT DE PROGRAM ---
void initializare() {
    glEnable(GL_DEPTH_TEST); // Activez 3D-ul (ca obiectele din fata sa le acopere pe alea din spate)
    glEnable(GL_TEXTURE_2D); // Ii zic la OpenGL ca vreau sa folosesc poze

    // Iarba trebuie intoarsa ca s-o citeasca corect OpenGL-ul
    stbi_set_flip_vertically_on_load(true);
    texturaIarba = incarcaTextura("iarba.jpg", true); // Bifez 'true' ca sa se repete la infinit

    // Incarc cerul. Astea 6 NU trebuie intoarse, altfel arata ciudat
    stbi_set_flip_vertically_on_load(false);
    // Atentie: Daca ecranul e negru, inseamna ca am gresit vreo litera in numele astea:
    texturaSkybox[0] = incarcaTextura("Daylight Box_Front.bmp", false);
    texturaSkybox[1] = incarcaTextura("Daylight Box_Back.bmp", false);
    texturaSkybox[2] = incarcaTextura("Daylight Box_Left.bmp", false);
    texturaSkybox[3] = incarcaTextura("Daylight Box_Right.bmp", false);
    texturaSkybox[4] = incarcaTextura("Daylight Box_Top.bmp", false);
    texturaSkybox[5] = incarcaTextura("Daylight Box_Bottom.bmp", false);

    // Incarc poza alb-negru pentru relief
    stbi_set_flip_vertically_on_load(true);
    // Fortez 1 canal ca sa o citeasca fix alb-negru
    heightmapData = stbi_load("Rolling Hills Height Map.png", &hmLatime, &hmInaltime, &hmCanale, 1);
    if (!heightmapData) {
        std::cout << "Eroare: Nu am gasit 'Rolling Hills Height Map.png'!" << std::endl;
    }
}

// --- SMECHERIA PENTRU RELIEF ---
float obtineInaltimeDinHarta(float x, float z) {
    // Daca n-a gasit poza de relief, fac terenul perfect plat la inaltimea 0
    if (!heightmapData) return 0.0f;

    // Transform coordonatele mele din joc (ex: -50..50) in coordonate de pe poza (ex: pixelul 20, 30)
    float u = (x + 50.0f) / 100.0f;
    float v = (z + 50.0f) / 100.0f;

    int px = u * (hmLatime - 1);
    int pz = v * (hmInaltime - 1);

    // Ma asigur ca nu citesc pe langa poza ca imi iau crash instant
    if (px < 0) px = 0; if (px >= hmLatime) px = hmLatime - 1;
    if (pz < 0) pz = 0; if (pz >= hmInaltime) pz = hmInaltime - 1;

    // Aflu cat de "alb" e pixelul. 0 e negru complet (vale), 255 e alb complet (munte)
    unsigned char culoarePixel = heightmapData[pz * hmLatime + px];

    // Convertesc pixelul in inaltime reala (max 15 metri in joc)
    return (culoarePixel / 255.0f) * 15.0f;
}

// --- DESENEZ CUBUL IN CARE STAU (SKYBOX) ---
void deseneazaSkybox() {
    float d = 100.0f; // Cat de mare e cubul. Sa fie destul cat sa incapa harta in el.
    glColor3f(1.0f, 1.0f, 1.0f); // IMPORTANT: Culoarea trebuie sa fie alba ca pozele sa ramana la culorile lor!

    // Lipesc pozele pe fiecare perete in parte. coordonata (0,0) din poza pusa pe colturile patratului.
    glBindTexture(GL_TEXTURE_2D, texturaSkybox[1]); // Spate
    glBegin(GL_QUADS);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-d, -d, -d);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(d, -d, -d);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(d, d, -d);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-d, d, -d);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, texturaSkybox[0]); // Fata
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-d, -d, d);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(d, -d, d);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(d, d, d);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-d, d, d);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, texturaSkybox[2]); // Stanga
    glBegin(GL_QUADS);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-d, -d, d);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-d, -d, -d);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-d, d, -d);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-d, d, d);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, texturaSkybox[3]); // Dreapta
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(d, -d, d);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(d, -d, -d);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(d, d, -d);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(d, d, d);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, texturaSkybox[4]); // Sus (Cerul vizibil efectiv)
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-d, d, -d);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(d, d, -d);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(d, d, d);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-d, d, d);
    glEnd();
}

// --- DESENEZ PODEAUA (RELIEFUL) ---
void deseneazaRelief() {
    glBindTexture(GL_TEXTURE_2D, texturaIarba);
    glColor3f(1.0f, 1.0f, 1.0f);

    // Fac un gratar (grid) urias din 2 in 2 metri (de la -50 la +50)
    glBegin(GL_QUADS);
    for (float x = -50.0f; x < 50.0f; x += 2.0f) {
        for (float z = -50.0f; z < 50.0f; z += 2.0f) {

            // Intreb functia mea inalta cat de sus trebuie pus fiecare din cele 4 colturi ale patratelului
            float y1 = obtineInaltimeDinHarta(x, z);
            float y2 = obtineInaltimeDinHarta(x + 2.0f, z);
            float y3 = obtineInaltimeDinHarta(x + 2.0f, z + 2.0f);
            float y4 = obtineInaltimeDinHarta(x, z + 2.0f);

            // Inmultesc coordonatele cu 10. Asta face iarba sa para marunta si detaliata. 
            // Daca o lasam simpla, se vedea o poza uriasa blurata pe tot muntele.
            float texScale = 10.0f / 100.0f;

            glTexCoord2f((x + 50) * texScale, (z + 50) * texScale); glVertex3f(x, y1, z);
            glTexCoord2f((x + 52) * texScale, (z + 50) * texScale); glVertex3f(x + 2.0f, y2, z);
            glTexCoord2f((x + 52) * texScale, (z + 52) * texScale); glVertex3f(x + 2.0f, y3, z + 2.0f);
            glTexCoord2f((x + 50) * texScale, (z + 52) * texScale); glVertex3f(x, y4, z + 2.0f);
        }
    }
    glEnd();
}

// --- BUCLE DE RANDARE ---
void renderScene(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Sterg cadrul vechi
    glLoadIdentity(); // Resetez camera

    // La P1 doar ma invart deasupra hartii in cerc
    float camX = sin(unghiCamera) * razaCamera;
    float camZ = cos(unghiCamera) * razaCamera;
    float camY = 25.0f; // Ma uit putin de sus

    // Ochii mei virtuali
    gluLookAt(
        camX, camY, camZ,       // De unde ma uit
        0.0f, 5.0f, 0.0f,       // La ce ma uit (in centrul ecranului)
        0.0f, 1.0f, 0.0f        // Unde e cerul (axa Y)
    );

    // Pun obiectele in scena
    deseneazaSkybox();
    deseneazaRelief();

    glutSwapBuffers(); // Arata poza pe monitor
}

// Functie necesara ca sa nu se turteasca imaginea cand fac fereastra mai mica/mare
void changeSize(int w, int h) {
    if (h == 0) h = 1;
    float ratio = w * 1.0 / h;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, w, h);
    // 250 e cat de departe pot sa vad. Daca era mai mic (ex: 100), se taiau muntii din departare.
    gluPerspective(45.0f, ratio, 0.1f, 250.0f);
    glMatrixMode(GL_MODELVIEW);
}

// Aici mut camera la fiecare "tic" ca sa se invarta singura
void animate() {
    unghiCamera += 0.002f; // Daca ma misc prea repede sau incet, modific aici!
    glutPostRedisplay(); // Zic placii video sa mai randeze un cadru
}

int main(int argc, char** argv) {
    // Setari default freeglut
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(1024, 768);
    glutCreateWindow("Proiect Grafica - P1 (Relief si Skybox)");

    initializare(); // Incarc pozele inainte sa incep sa desenez

    // Asociez functiile mele cu freeglut
    glutDisplayFunc(renderScene);
    glutReshapeFunc(changeSize);
    glutIdleFunc(animate); // Cand PC-ul n-are ce face, sa invarta camera

    glutMainLoop(); // Porneste efectiv aplicatia
    return 0;
}