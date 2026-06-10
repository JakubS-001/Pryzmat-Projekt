/**
 * @file drawings.h
 * @brief Moduł do rysowania elementów sceny.
 * @details Udostępnia funkcje do rysowania m.in. geometrii (sześciany, sfery, soczewki, pryzmaty),
 * a także rysuje promienie symulacji optycznej i interfejs celownika/statystyk (HUD).
 * @author Jakub Stasierski
 */

#ifndef DRAWINGS_H
#define DRAWINGS_H

#include <glm/glm.hpp>
#include <vector>

#include "rays.h"
class ShaderProgram;

/**
 * @brief Zewnętrzne zmienne używane podczas rysowania HUD oraz renderowania promieni.
 */
extern std::vector<BeamRay> firedRays;
extern int windowWidth;
extern int windowHeight;
extern glm::vec3 cameraPos;
extern glm::vec3 cameraFront;
extern bool showLabels;

/**
 * @brief Rysuje sześcian na scenie wykorzystując podany shader (np. oświetlenie Lamberta).
 * @param sp Wskaźnik do instancji programu shadera użytego do renderowania.
 * @param P Macierz projekcji (Projection).
 * @param V Macierz widoku (View).
 * @param M Macierz modelu (Model).
 * @param color Kolor bazowy sześcianu w formacie RGBA.
 */
void drawCube(ShaderProgram* sp, const glm::mat4& P, const glm::mat4& V, const glm::mat4& M, const glm::vec4& color);

/**
 * @brief Rysuje półprzezroczysty pryzmat geometryczny na potrzeby symulacji rozszczepienia światła.
 * @param P Macierz projekcji.
 * @param V Macierz widoku.
 * @param M Macierz modelu dla pryzmatu.
 * @param color Kolor i przezroczystość obiektu (RGBA).
 */
void drawPrism(const glm::mat4& P, const glm::mat4& V, const glm::mat4& M, const glm::vec4& color);

/**
 * @brief Generuje i rysuje geometryczną sferę.
 * @param P Macierz projekcji.
 * @param V Macierz widoku.
 * @param center Położenie środka sfery w przestrzeni światowej.
 * @param radius Promień sfery.
 * @param color Kolor renderowanej sfery (RGBA).
 */
void drawSphere(const glm::mat4& P, const glm::mat4& V, const glm::vec3& center, float radius, const glm::vec4& color);

/**
 * @brief Renderuje półprzezroczystą, rozciągniętą elipsoidę działającą jako soczewka.
 * @param P Macierz projekcji.
 * @param V Macierz widoku.
 * @param M Macierz transformacji soczewki.
 * @param color Kolor soczewki.
 */
void drawEllipsoid(const glm::mat4& P, const glm::mat4& V, const glm::mat4& M, const glm::vec4& color);

/**
 * @brief Rysuje półprzezroczysty walec.
 * @param P Macierz projekcji.
 * @param V Macierz widoku.
 * @param M Macierz modelu walca.
 * @param color Kolor obiektu (RGBA).
 */
void drawCylinder(const glm::mat4& P, const glm::mat4& V, const glm::mat4& M, const glm::vec4& color);

/**
 * @brief Rysuje prostokątną płaszczyznę odbijającą (lustro).
 * @param P Macierz projekcji.
 * @param V Macierz widoku.
 * @param M Macierz modelu lustra (translacja i rotacja).
 */
void drawMirrorObject(const glm::mat4& P, const glm::mat4& V, const glm::mat4& M);

/**
 * @brief Wyświetla wszystkie wygenerowane linie dla wyzwolonych i przebiegających promieni świetlnych.
 * @details Renderuje z użyciem mieszania addytywnego (glow effect), tworząc efekt "jarzących się" linii lasera.
 * @param P Macierz projekcji.
 * @param V Macierz widoku kamery.
 */
void drawBeamLines(const glm::mat4& P, const glm::mat4& V);

/**
 * @brief Odpowiada za wyświetlenie statystyk interfejsu 2D za pośrednictwem ImGui oraz rysowanie wskaźnika.
 * @details Rysuje centralny celownik na ekranie i po namierzeniu laserowego promienia pokazuje jego długość fali.
 */
void drawCrosshairHUD();

#endif // DRAWINGS_H