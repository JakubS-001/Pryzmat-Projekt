/**
 * @file drawings.cpp
 * @brief Moduł do rysowania elementów sceny.
 * @details Udostępnia funkcje do rysowania m.in. geometrii (sześciany, sfery, soczewki, pryzmaty),
 * a także rysuje promienie symulacji optycznej i interfejs celownika/statystyk (HUD).
 * @author Jakub Stasierski
 */

#include "drawings.h"
#include "rays.h" // Niezbędne dla intersectRaySegment
#include "shaderprogram.h"
#include "allmodels.h"
#include "imgui.h"
#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdio.h>
#include <cmath>

// Odwołanie do wskaźników shadera zdefiniowanych we fragmencie głównym aplikacji
extern ShaderProgram* spLambert;
extern ShaderProgram* spConstant;

/**
 * @brief Rysuje sześcian na scenie wykorzystując podany shader.
 * @param sp Wskaźnik do instancji programu shadera użytego do renderowania.
 * @param P Macierz projekcji.
 * @param V Macierz widoku.
 * @param M Macierz modelu.
 * @param color Kolor bazowy sześcianu w formacie RGBA.
 */
void drawCube(ShaderProgram* sp, const glm::mat4& P, const glm::mat4& V, const glm::mat4& M, const glm::vec4& color) {
	sp->use();
	glUniformMatrix4fv(sp->u("P"), 1, GL_FALSE, glm::value_ptr(P));
	glUniformMatrix4fv(sp->u("V"), 1, GL_FALSE, glm::value_ptr(V));
	glUniformMatrix4fv(sp->u("M"), 1, GL_FALSE, glm::value_ptr(M));
	glUniform4fv(sp->u("color"), 1, glm::value_ptr(color));
	// Źródło światła punktowego zlokalizowane nad stołem (X=0.0, Y=4.0, Z=0.0)
glm::vec4 lightPos = glm::vec4(0.0f, 2.2f, 0.0f, 1.0f); 

// Transformacja współrzędnych światła do przestrzeni widoku (View Space)
glm::vec4 viewLightPos = V * lightPos;
glUniform4fv(sp->u("lightPos"), 1, glm::value_ptr(viewLightPos));
	Models::cube.drawSolid(true);
}

/**
 * @brief Rysuje półprzezroczysty pryzmat geometryczny.
 * @param P Macierz projekcji.
 * @param V Macierz widoku.
 * @param M Macierz modelu dla pryzmatu.
 * @param color Kolor i przezroczystość obiektu (RGBA).
 */
void drawPrism(const glm::mat4& P, const glm::mat4& V, const glm::mat4& M, const glm::vec4& color) {
	static const float prismVertices[] = {
		-0.8f, 0.0f, -0.5f, 1.0f,  0.8f, 0.0f, -0.5f, 1.0f,  0.0f, 0.0f,  0.8f, 1.0f,
		 0.0f, 0.7f,  0.8f, 1.0f,  0.8f, 0.7f, -0.5f, 1.0f, -0.8f, 0.7f, -0.5f, 1.0f,
		-0.8f, 0.0f, -0.5f, 1.0f,  0.8f, 0.0f, -0.5f, 1.0f,  0.8f, 0.7f, -0.5f, 1.0f,
		-0.8f, 0.0f, -0.5f, 1.0f,  0.8f, 0.7f, -0.5f, 1.0f, -0.8f, 0.7f, -0.5f, 1.0f,
		 0.8f, 0.0f, -0.5f, 1.0f,  0.0f, 0.0f,  0.8f, 1.0f,  0.0f, 0.7f, 0.8f, 1.0f,
		 0.8f, 0.0f, -0.5f, 1.0f,  0.0f, 0.7f,  0.8f, 1.0f,  0.8f, 0.7f, -0.5f, 1.0f,
		 0.0f, 0.0f,  0.8f, 1.0f, -0.8f, 0.0f, -0.5f, 1.0f, -0.8f, 0.7f, -0.5f, 1.0f,
		 0.0f, 0.0f,  0.8f, 1.0f, -0.8f, 0.7f, -0.5f, 1.0f,  0.0f, 0.7f,  0.8f, 1.0f
	};
	static const float prismNormals[] = {
		 0.0f, -1.0f,  0.0f, 0.0f, 0.0f, -1.0f,  0.0f, 0.0f,  0.4f, -0.4f,  0.8f, 0.0f,
		 0.0f,  1.0f,  0.0f, 0.0f, 0.0f,  0.0f, -1.0f, 0.0f, -0.4f, 0.2f, -0.9f, 0.0f,
		 0.0f, -1.0f,  0.0f, 0.0f, 0.0f,  0.0f, -1.0f, 0.0f,  0.0f,  0.0f, -1.0f, 0.0f,
		 0.0f,  0.0f, -1.0f, 0.0f, 0.0f,  0.0f, -1.0f, 0.0f,  0.0f,  0.0f, -1.0f, 0.0f,
		-0.5f, -0.4f,  0.8f, 0.0f, 0.4f, -0.4f,  0.8f, 0.0f,  0.0f,  0.0f,  1.0f, 0.0f,
		-0.5f, -0.4f,  0.8f, 0.0f,  0.0f,  0.0f,  1.0f, 0.0f,  0.4f, -0.4f,  0.8f, 0.0f,
		 0.0f, -1.0f,  0.0f, 0.0f, 0.0f, -0.4f, -0.9f, 0.0f, -0.5f,  0.7f, -0.5f, 0.0f,
		 0.0f, -1.0f,  0.0f, 0.0f, 0.0f, -0.4f, -0.9f, 0.0f,  0.4f,  0.2f, -0.9f, 0.0f
	};
	spConstant->use();
	glUniformMatrix4fv(spConstant->u("P"), 1, GL_FALSE, glm::value_ptr(P));
	glUniformMatrix4fv(spConstant->u("V"), 1, GL_FALSE, glm::value_ptr(V));
	glUniformMatrix4fv(spConstant->u("M"), 1, GL_FALSE, glm::value_ptr(M));
	glUniform4fv(spConstant->u("color"), 1, glm::value_ptr(color));
	glEnableVertexAttribArray(0); glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, prismVertices);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, prismNormals);
	glDrawArrays(GL_TRIANGLES, 0, 24);
	glDisableVertexAttribArray(0); glDisableVertexAttribArray(1);
}

/**
 * @brief Generuje i rysuje geometryczną sferę.
 * @param P Macierz projekcji.
 * @param V Macierz widoku.
 * @param center Położenie środka sfery.
 * @param radius Promień sfery.
 * @param color Kolor (RGBA).
 */
void drawSphere(const glm::mat4& P, const glm::mat4& V, const glm::vec3& center, float radius, const glm::vec4& color) {
	static std::vector<float> verts; static std::vector<float> norms;
	if (verts.empty()) {
		int stacks = 12, slices = 12;
		for (int i = 0; i < stacks; ++i) {
			float phi1 = 3.14159265f * (float)i / stacks;
			float phi2 = 3.14159265f * (float)(i + 1) / stacks;
			for (int j = 0; j < slices; ++j) {
				float theta1 = 2.0f * 3.14159265f * (float)j / slices;
				float theta2 = 2.0f * 3.14159265f * (float)(j + 1) / slices;
				float x1 = sin(phi1) * cos(theta1), y1 = cos(phi1), z1 = sin(phi1) * sin(theta1);
				float x2 = sin(phi2) * cos(theta1), y2 = cos(phi2), z2 = sin(phi2) * sin(theta1);
				float x3 = sin(phi2) * cos(theta2), y3 = cos(phi2), z3 = sin(phi2) * sin(theta2);
				float x4 = sin(phi1) * cos(theta2), y4 = cos(phi1), z4 = sin(phi1) * sin(theta2);
				verts.insert(verts.end(), { x1, y1, z1, 1.0f, x2, y2, z2, 1.0f, x3, y3, z3, 1.0f });
				norms.insert(norms.end(), { x1, y1, z1, 0.0f, x2, y2, z2, 0.0f, x3, y3, z3, 0.0f });
				verts.insert(verts.end(), { x1, y1, z1, 1.0f, x3, y3, z3, 1.0f, x4, y4, z4, 1.0f });
				norms.insert(norms.end(), { x1, y1, z1, 0.0f, x3, y3, z3, 0.0f, x4, y4, z4, 0.0f });
			}
		}
	}
	spConstant->use();
	glm::mat4 M = glm::translate(glm::mat4(1.0f), center); M = glm::scale(M, glm::vec3(radius));
	glUniformMatrix4fv(spConstant->u("P"), 1, GL_FALSE, glm::value_ptr(P));
	glUniformMatrix4fv(spConstant->u("V"), 1, GL_FALSE, glm::value_ptr(V));
	glUniformMatrix4fv(spConstant->u("M"), 1, GL_FALSE, glm::value_ptr(M));
	glUniform4fv(spConstant->u("color"), 1, glm::value_ptr(color));
	glEnableVertexAttribArray(0); glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, verts.data());
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, norms.data());
	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(verts.size() / 4));
	glDisableVertexAttribArray(0); glDisableVertexAttribArray(1);
}

/**
 * @brief Renderuje półprzezroczystą, rozciągniętą elipsoidę działającą jako soczewka.
 * @param P Macierz projekcji.
 * @param V Macierz widoku.
 * @param M Macierz transformacji soczewki.
 * @param color Kolor soczewki.
 */
void drawEllipsoid(const glm::mat4& P, const glm::mat4& V, const glm::mat4& M, const glm::vec4& color) {
	static std::vector<float> verts; static std::vector<float> norms;
	if (verts.empty()) {
		int stacks = 18, slices = 18;
		for (int i = 0; i < stacks; ++i) {
			float phi1 = 3.14159265f * (float)i / stacks;
			float phi2 = 3.14159265f * (float)(i + 1) / stacks;
			for (int j = 0; j < slices; ++j) {
				float theta1 = 2.0f * 3.14159265f * (float)j / slices;
				float theta2 = 2.0f * 3.14159265f * (float)(j + 1) / slices;
				float x1 = sin(phi1) * cos(theta1), y1 = cos(phi1), z1 = sin(phi1) * sin(theta1);
				float x2 = sin(phi2) * cos(theta1), y2 = cos(phi2), z2 = sin(phi2) * sin(theta1);
				float x3 = sin(phi2) * cos(theta2), y3 = cos(phi2), z3 = sin(phi2) * sin(theta2);
				float x4 = sin(phi1) * cos(theta2), y4 = cos(phi1), z4 = sin(phi1) * sin(theta2);
				verts.insert(verts.end(), { x1, y1, z1, 1.0f, x2, y2, z2, 1.0f, x3, y3, z3, 1.0f });
				norms.insert(norms.end(), { x1, y1, z1, 0.0f, x2, y2, z2, 0.0f, x3, y3, z3, 0.0f });
				verts.insert(verts.end(), { x1, y1, z1, 1.0f, x3, y3, z3, 1.0f, x4, y4, z4, 1.0f });
				norms.insert(norms.end(), { x1, y1, z1, 0.0f, x3, y3, z3, 0.0f, x4, y4, z4, 0.0f });
			}
		}
	}
	spConstant->use();
	glUniformMatrix4fv(spConstant->u("P"), 1, GL_FALSE, glm::value_ptr(P));
	glUniformMatrix4fv(spConstant->u("V"), 1, GL_FALSE, glm::value_ptr(V));
	glUniformMatrix4fv(spConstant->u("M"), 1, GL_FALSE, glm::value_ptr(M));
	glUniform4fv(spConstant->u("color"), 1, glm::value_ptr(color));
	glEnableVertexAttribArray(0); glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, verts.data());
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, norms.data());
	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(verts.size() / 4));
	glDisableVertexAttribArray(0); glDisableVertexAttribArray(1);
}

/**
 * @brief Rysuje półprzezroczysty walec.
 * @param P Macierz projekcji.
 * @param V Macierz widoku.
 * @param M Macierz modelu walca.
 * @param color Kolor obiektu.
 */
void drawCylinder(const glm::mat4& P, const glm::mat4& V, const glm::mat4& M, const glm::vec4& color) {
	static std::vector<float> verts; static std::vector<float> norms;
	if (verts.empty()) {
		int slices = 16;
		for (int i = 0; i < slices; ++i) {
			float t1 = 2.0f * 3.14159265f * (float)i / slices;
			float t2 = 2.0f * 3.14159265f * (float)(i + 1) / slices;
			float x1 = cos(t1); float z1 = sin(t1);
			float x2 = cos(t2); float z2 = sin(t2);

			verts.insert(verts.end(), { x1, 0.0f, z1, 1.0f, x1, 1.0f, z1, 1.0f, x2, 1.0f, z2, 1.0f });
			norms.insert(norms.end(), { x1, 0.0f, z1, 0.0f, x1, 0.0f, z1, 0.0f, x2, 0.0f, z2, 0.0f });
			verts.insert(verts.end(), { x1, 0.0f, z1, 1.0f, x2, 1.0f, z2, 1.0f, x2, 0.0f, z2, 1.0f });
			norms.insert(norms.end(), { x1, 0.0f, z1, 0.0f, x2, 0.0f, z2, 0.0f, x2, 0.0f, z2, 0.0f });

			verts.insert(verts.end(), { 0.0f, 0.0f, 0.0f, 1.0f, x2, 0.0f, z2, 1.0f, x1, 0.0f, z1, 1.0f });
			norms.insert(norms.end(), { 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f });

			verts.insert(verts.end(), { 0.0f, 1.0f, 0.0f, 1.0f, x1, 1.0f, z1, 1.0f, x2, 1.0f, z2, 1.0f });
			norms.insert(norms.end(), { 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f });
		}
	}
	spConstant->use();
	glUniformMatrix4fv(spConstant->u("P"), 1, GL_FALSE, glm::value_ptr(P));
	glUniformMatrix4fv(spConstant->u("V"), 1, GL_FALSE, glm::value_ptr(V));
	glUniformMatrix4fv(spConstant->u("M"), 1, GL_FALSE, glm::value_ptr(M));
	glUniform4fv(spConstant->u("color"), 1, glm::value_ptr(color));
	glEnableVertexAttribArray(0); glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, verts.data());
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, norms.data());
	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(verts.size() / 4));
	glDisableVertexAttribArray(0); glDisableVertexAttribArray(1);
}

/**
 * @brief Rysuje prostokątną płaszczyznę odbijającą (lustro).
 * @param P Macierz projekcji.
 * @param V Macierz widoku.
 * @param M Macierz modelu lustra.
 */
void drawMirrorObject(const glm::mat4& P, const glm::mat4& V, const glm::mat4& M) {
	static const float mirrorVertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f,  0.5f, -0.5f, 0.0f, 1.0f,  0.5f,  0.5f, 0.0f, 1.0f,
		-0.5f, -0.5f, 0.0f, 1.0f,  0.5f,  0.5f, 0.0f, 1.0f, -0.5f,  0.5f, 0.0f, 1.0f
	};
	static const float mirrorNormals[] = {
		0.0f, 0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 1.0f, 0.0f
	};
	spConstant->use();
	glUniformMatrix4fv(spConstant->u("P"), 1, GL_FALSE, glm::value_ptr(P));
	glUniformMatrix4fv(spConstant->u("V"), 1, GL_FALSE, glm::value_ptr(V));
	glUniformMatrix4fv(spConstant->u("M"), 1, GL_FALSE, glm::value_ptr(M));
	glm::vec4 mirrorSurfaceColor = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f);
	glUniform4fv(spConstant->u("color"), 1, glm::value_ptr(mirrorSurfaceColor));
	glEnableVertexAttribArray(0); glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, mirrorVertices);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, mirrorNormals);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(0); glDisableVertexAttribArray(1);
}

/**
 * @brief Wyświetla wszystkie wygenerowane linie dla wyzwolonych promieni świetlnych.
 * @details Używa mieszania addytywnego do stworzenia efektu poświaty dla promieni laserowych.
 * @param P Macierz projekcji.
 * @param V Macierz widoku kamery.
 */
void drawBeamLines(const glm::mat4& P, const glm::mat4& V) {
	if (firedRays.empty()) return;
	spConstant->use();
	glUniformMatrix4fv(spConstant->u("P"), 1, GL_FALSE, glm::value_ptr(P));
	glUniformMatrix4fv(spConstant->u("V"), 1, GL_FALSE, glm::value_ptr(V));
	glUniformMatrix4fv(spConstant->u("M"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
	static const float zeroNormals[8] = { 0.0f };
	glEnableVertexAttribArray(0); glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, zeroNormals);
	glEnable(GL_BLEND);

	// Włącz mieszanie addytywne - poświata promieni
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	for (const BeamRay& ray : firedRays) {
		for (const BeamSegment& seg : ray.segments) {
			float lineVertices[8] = {
				seg.start.x, seg.start.y, seg.start.z, 1.0f,
				seg.end.x, seg.end.y, seg.end.z, 1.0f
			};
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, lineVertices);
			glLineWidth(6.0f);
			glUniform4fv(spConstant->u("color"), 1, glm::value_ptr(seg.color));
			glDrawArrays(GL_LINES, 0, 2);
		}
	}

	// Przywróć podstawowy sposób łączenia, żeby uniknąć defektów tła/HUD
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisableVertexAttribArray(0); glDisableVertexAttribArray(1);
	glDisable(GL_BLEND);
}

/**
 * @brief Odpowiada za wyświetlenie interfejsu ImGui i rysowanie wskaźnika.
 * @details Aktualizuje HUD (FPS, liczba promieni) oraz pozwala namierzać konkretny promień i podglądać jego długość fali.
 */
void drawCrosshairHUD() {
	ImGuiIO& io = ImGui::GetIO();

	// --- 1. OKNO STATYSTYK OVERLAY ---
	ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.35f);

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs;

	if (ImGui::Begin("HUD_Stats", nullptr, window_flags)) {
		ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "FPS: %.1f", io.Framerate);

		size_t totalSegments = 0;
		for (const auto& ray : firedRays) {
			totalSegments += ray.segments.size();
		}

		ImGui::Text("Ilosc promieni: %zu", firedRays.size());
		ImGui::Text("Ilosc segmentow promieni: %zu", totalSegments);

		ImGui::Separator();
		ImGui::Text("Nacisnij [TAB] zeby wlaczyc celownik do odczytu koloru");
	}
	ImGui::End();

	// --- 2. INTERAKTYWNY CELOWNIK ---
	if (!showLabels) return;

	bool foundRay = false;
	glm::vec4 targetColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	float targetWavelength = 0.0f;
	float detectionRadius = 0.12f;

	// Wyszukanie w promieniach celu, w który jest skierowana kamera
	for (const auto& ray : firedRays) {
		for (const auto& seg : ray.segments) {
			float dist = intersectRaySegment(cameraPos, cameraFront, seg.start, seg.end);
			if (dist < detectionRadius) {
				detectionRadius = dist;
				targetColor = ray.color;
				targetWavelength = seg.wavelength;
				foundRay = true;
			}
		}
	}

	ImDrawList* drawList = ImGui::GetForegroundDrawList();
	ImVec2 center = ImVec2(windowWidth * 0.5f, windowHeight * 0.5f);
	ImU32 crosshairColor = IM_COL32((int)(targetColor.r * 255), (int)(targetColor.g * 255), (int)(targetColor.b * 255), 255);

	// Rysowanie "krzyżyka"
	float length = 12.0f;
	float thickness = 2.0f;
	drawList->AddLine(ImVec2(center.x - length, center.y), ImVec2(center.x + length, center.y), crosshairColor, thickness);
	drawList->AddLine(ImVec2(center.x, center.y - length), ImVec2(center.x, center.y + length), crosshairColor, thickness);

	// Tło i wartość liczbowa przy namierzeniu
	if (foundRay) {
		char textBuf[64];
		float displayWavelength = (targetWavelength < 1.0f) ? (targetWavelength * 1000.0f) : targetWavelength;
		snprintf(textBuf, sizeof(textBuf), "%.0f nm", displayWavelength);

		ImVec2 textSize = ImGui::CalcTextSize(textBuf);
		ImVec2 textPos = ImVec2((windowWidth - textSize.x) * 0.5f, center.y - 40.0f);

		drawList->AddRectFilled(ImVec2(textPos.x - 8, textPos.y - 4), ImVec2(textPos.x + textSize.x + 8, textPos.y + textSize.y + 4), IM_COL32(15, 15, 15, 200), 4.0f);
		drawList->AddText(textPos, crosshairColor, textBuf);
	}
}