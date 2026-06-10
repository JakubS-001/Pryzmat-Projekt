/**
 * @file rays.h
 * @brief Moduł z logiką promieni świetlnych.
 * @details Zawiera funkcje sprawdzające przecięcia promieni z siatkami trójkątów, pryzmatami, cylindrami,
 * sferami, soczewkami oraz innymi elementami sceny.
 * @author Jakub Stasierski
 */

#ifndef RAYS_H
#define RAYS_H

#include <glm/glm.hpp>
#include <vector>

/**
 * @brief Struktura reprezentująca pojedynczy odcinek promienia świetlnego (segment).
 */
struct BeamSegment {
	glm::vec3 start;     /**< Punkt początkowy segmentu w przestrzeni. */
	glm::vec3 end;       /**< Punkt końcowy segmentu w przestrzeni. */
	glm::vec4 color;     /**< Kolor segmentu w formacie RGBA. */
	float wavelength;    /**< Długość fali odpowiadająca za kolor i współczynnik załamania. */
};

/**
 * @brief Struktura opisująca cały promień światła, składający się z ciągu segmentów.
 */
struct BeamRay {
	std::vector<BeamSegment> segments; /**< Tablica połączonych segmentów ścieżki światła. */
	glm::vec4 color;                   /**< Uśredniony/bazowy kolor dla tego promienia. */
};

/**
 * @brief Struktura przechowująca parametry początkowe punktu wystrzelenia wiązki (lasera).
 */
struct Burst {
	glm::vec3 origin;    /**< Pozycja startowa (źródło promienia). */
	glm::vec3 dir;       /**< Znormalizowany kierunek strzału wiązki. */
};

/**
 * @brief Zewnętrzna zmienna globalna wymagana do prawidłowego obliczenia położenia pryzmatu w przestrzeni.
 */
extern glm::vec3 prismPos;

/**
 * @brief Sprawdza przecięcie promienia z trójkątem w przestrzeni 3D.
 * @details Wykorzystuje algorytm Möllera-Trumbore'a.
 * @param orig Punkt początkowy promienia.
 * @param dir Znormalizowany wektor kierunku promienia.
 * @param v0 Pierwszy wierzchołek trójkąta.
 * @param v1 Drugi wierzchołek trójkąta.
 * @param v2 Trzeci wierzchołek trójkąta.
 * @param t Referencja na zmienną przechowującą dystans do punktu przecięcia (jeśli wystąpi).
 * @param faceNormal Referencja na wektor normalny powierzchni w punkcie przecięcia.
 * @return true jeśli nastąpiło przecięcie promienia z trójkątem, false w przeciwnym wypadku.
 */
bool rayTriangleIntersect(const glm::vec3& orig, const glm::vec3& dir,
	const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
	float& t, glm::vec3& faceNormal);

/**
 * @brief Sprawdza przecięcie promienia ze stołem.
 * @param origin Punkt początkowy promienia.
 * @param dir Kierunek promienia.
 * @param t Dystans do punktu trafienia.
 * @param normal Znormalizowany wektor normalny trafionej powierzchni stołu.
 * @return true jeśli nastąpiło przecięcie ze stołem.
 */
bool intersectTable(const glm::vec3& origin, const glm::vec3& dir, float& t, glm::vec3& normal);

/**
 * @brief Sprawdza przecięcie promienia z pojedynczą ścianą pryzmatu.
 * @param origin Punkt początkowy promienia.
 * @param dir Kierunek promienia.
 * @param v0 Wierzchołek ściany.
 * @param v1 Wierzchołek ściany.
 * @param v2 Wierzchołek ściany.
 * @param t Dystans do punktu trafienia.
 * @param orientedNormal Zorientowany wektor normalny ułatwiający odbicie lub załamanie.
 * @return true jeśli nastąpiło przecięcie ze ścianą pryzmatu.
 */
bool intersectPrismFace(const glm::vec3& origin, const glm::vec3& dir,
	const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
	float& t, glm::vec3& orientedNormal);

/**
 * @brief Sprawdza przecięcie promienia z całym pryzmatem.
 * @param origin Punkt początkowy promienia.
 * @param dir Kierunek promienia.
 * @param t Zmienna wyjściowa przechowująca dystans do najbliższego przecięcia.
 * @param normal Normalna trafionej ściany pryzmatu.
 * @return true jeśli promień trafia w pryzmat.
 */
bool intersectPrism(const glm::vec3& origin, const glm::vec3& dir, float& t, glm::vec3& normal);

/**
 * @brief Sprawdza przecięcie promienia ze sferą.
 * @param origin Punkt początkowy promienia.
 * @param dir Kierunek promienia.
 * @param t Dystans do punktu trafienia.
 * @param normal Normalna powierzchni sfery w punkcie uderzenia.
 * @return true jeśli promień przecina sferę.
 */
bool intersectSphere(const glm::vec3& origin, const glm::vec3& dir, float& t, glm::vec3& normal);

/**
 * @brief Sprawdza przecięcie promienia z cylindrem.
 * @param origin Punkt początkowy promienia.
 * @param dir Kierunek promienia.
 * @param model Macierz transformacji (model matrix) cylindra.
 * @param t Dystans do najbliższego punktu trafienia.
 * @param normal Normalna powierzchni na cylindrze.
 * @return true jeśli promień przecina cylinder.
 */
bool intersectCylinder(const glm::vec3& origin, const glm::vec3& dir, const glm::mat4& model, float& t, glm::vec3& normal);

/**
 * @brief Sprawdza przecięcie promienia z płaskim lustrem.
 * @param origin Punkt początkowy promienia.
 * @param dir Kierunek promienia.
 * @param mirrorM Macierz transformacji lustra w przestrzeni.
 * @param t Dystans promienia do płaszczyzny lustra.
 * @param normal Wektor normalny powierzchni lustra.
 * @return true jeśli promień uderza w lustro.
 */
bool intersectMirror(const glm::vec3& origin, const glm::vec3& dir, const glm::mat4& mirrorM, float& t, glm::vec3& normal);

/**
 * @brief Sprawdza przecięcie promienia z soczewką.
 * @param origin Punkt początkowy promienia.
 * @param dir Kierunek promienia.
 * @param model Macierz modelu soczewki (dla przekształceń).
 * @param t Zmienna przechowująca parametr T trafienia.
 * @param normal Wektor normalny trafionej płaszczyzny soczewki.
 * @return true w przypadku pomyślnej detekcji kolizji.
 */
bool intersectLens(const glm::vec3& origin, const glm::vec3& dir, const glm::mat4& model, float& t, glm::vec3& normal);

/**
 * @brief Sprawdza przecięcie promienia ze ścianami pokoju/sceny.
 * @param origin Punkt startu.
 * @param dir Kierunek strzału.
 * @param t Dystans odcinek z origin do pokoju.
 * @param normal Wektor prostopadły do wyznaczonej ściany.
 * @return true jeżeli promień przecina "pokój".
 */
bool intersectRoom(const glm::vec3& origin, const glm::vec3& dir, float& t, glm::vec3& normal);

/**
 * @brief Oblicza odległość między promieniem kamery a segmentem promienia światła (np. dla celownika HUD).
 * @param ro Punkt początkowy promienia patrzenia kamery.
 * @param rd Kierunek patrzenia kamery.
 * @param p0 Pierwszy punkt segmentu światła.
 * @param p1 Drugi punkt segmentu światła.
 * @return Najmniejsza odległość przestrzenna między prostopadłymi zrzutami obu promieni.
 */
float intersectRaySegment(glm::vec3 ro, glm::vec3 rd, glm::vec3 p0, glm::vec3 p1);

// --- OPTICS SIMULATION FUNCTIONS ---

// Wymaga dostępu do globalnych zmiennych z main_file
extern std::vector<glm::mat4> mirrors;
extern glm::mat4 cylinderModel;
extern glm::mat4 lensModel;
extern float prismIndexOfRefraction;
extern float prismDispersion;
extern std::vector<BeamRay> firedRays;

/**
 * @brief Konwertuje długość fali na kolor RGBA.
 * @param wavelength Długość fali świata w mikrometrach (0.38 - 0.70).
 * @return Wektor koloru RGBA.
 */
glm::vec4 wavelengthToColor(float wavelength);

/**
 * @brief Oblicza wskaźnik załamania dla zadanej długości fali.
 * @param wavelength Długość fali w mikrometrach.
 * @return Współczynnik załamania światła.
 */
float refractiveIndexForWavelength(float wavelength);

/**
 * @brief Dodaje segment promienia do struktury BeamRay.
 * @param ray Referencja do promienia, do którego dodawany jest segment.
 * @param start Punkt początkowy segmentu.
 * @param end Punkt końcowy segmentu.
 * @param wavelength Długość fali dla tego segmentu.
 */
void addBeamSegment(BeamRay& ray, const glm::vec3& start, const glm::vec3& end, float wavelength);

/**
 * @brief Śledzi przebieg promienia przez scenę z odbiciami i załamaniami.
 * @param ray Promień do narysowania.
 * @param origin Punkt początkowy promienia.
 * @param dir Kierunek promienia.
 * @param inside Czy promień znajduje się wewnątrz materiału.
 * @param depth Głębokość rekurencji śledzonego promienia.
 * @param currentIOR Bieżący współczynnik załamania.
 * @param wavelength Długość fali promienia.
 */
void traceBeam(BeamRay& ray, const glm::vec3& origin, const glm::vec3& dir, bool inside, int depth, float currentIOR, float wavelength);

/**
 * @brief Symuluje wybuch promieni świetlnych z rozdzieleniem na długości fali.
 * @param origin Punkt startu wybuchu.
 * @param direction Kierunek wybuchu.
 */
void simulateBurst(const glm::vec3& origin, const glm::vec3& direction);

#endif // RAYS_H