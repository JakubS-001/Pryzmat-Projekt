/**
 * @file rays.cpp
 * @brief Moduł z logiką promieni świetlnych.
 * @details Zawiera funkcje sprawdzające przecięcia promieni z siatkami trójkątów, pryzmatami, cylindrami,
 * sferami, soczewkami oraz innymi elementami sceny.
 * @author Jakub Stasierski
 */

#include "rays.h"
#include <cmath>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/geometric.hpp>

/**
 * @brief Sprawdza przecięcie promienia z trójkątem w przestrzeni 3D.
 * @details Używa algorytmu Möllera-Trumbore'a dla szybkiej weryfikacji punktu trafienia.
 * @param orig Punkt początkowy promienia.
 * @param dir Znormalizowany wektor kierunku promienia.
 * @param v0 Pierwszy wierzchołek trójkąta.
 * @param v1 Drugi wierzchołek trójkąta.
 * @param v2 Trzeci wierzchołek trójkąta.
 * @param t Referencja na zmienną przechowującą dystans do przecięcia.
 * @param faceNormal Referencja na wektor normalny powierzchni.
 * @return true jeśli promień uderzył w trójkąt.
 */
bool rayTriangleIntersect(const glm::vec3& orig, const glm::vec3& dir,
	const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
	float& t, glm::vec3& faceNormal) {
	const float EPSILON = 0.000001f;
	glm::vec3 edge1 = v1 - v0;
	glm::vec3 edge2 = v2 - v0;
	glm::vec3 pvec = glm::cross(dir, edge2);
	float det = glm::dot(edge1, pvec);
	if (fabs(det) < EPSILON) return false;
	float invDet = 1.0f / det;
	glm::vec3 tvec = orig - v0;
	float u = glm::dot(tvec, pvec) * invDet;
	if (u < 0.0f || u > 1.0f) return false;
	glm::vec3 qvec = glm::cross(tvec, edge1);
	float v = glm::dot(dir, qvec) * invDet;
	if (v < 0.0f || u + v > 1.0f) return false;
	t = glm::dot(edge2, qvec) * invDet;
	if (t <= 0.001f) return false;
	faceNormal = glm::normalize(glm::cross(edge1, edge2));
	return true;
}

/**
 * @brief Sprawdza przecięcie promienia ze stołem.
 * @param origin Punkt początkowy promienia.
 * @param dir Kierunek promienia.
 * @param t Dystans do punktu trafienia.
 * @param normal Znormalizowany wektor normalny trafionej powierzchni stołu.
 * @return true jeśli nastąpiło przecięcie ze stołem.
 */
bool intersectTable(const glm::vec3& origin, const glm::vec3& dir, float& t, glm::vec3& normal) {
	const float tableCenterY = -3.2f;
	const float tableHeight = 0.2f;
	const float tableHalfY = tableHeight * 0.5f;
	const float tableHalfX = 4.5f;
	const float tableHalfZ = 3.0f;
	const float tableTop = tableCenterY + tableHalfY;
	const float tableBottom = tableCenterY - tableHalfY;

	if (fabs(dir.y) < 1e-6f) return false;

	// Sprawdzenie górnej płaszczyzny stołu
	float tTop = (tableTop - origin.y) / dir.y;
	if (tTop > 0.001f) {
		glm::vec3 p = origin + dir * tTop;
		if (p.x >= -tableHalfX && p.x <= tableHalfX && p.z >= -tableHalfZ && p.z <= tableHalfZ) {
			t = tTop; normal = glm::vec3(0.0f, 1.0f, 0.0f); return true;
		}
	}

	// Sprawdzenie dolnej płaszczyzny stołu
	float tBottom = (tableBottom - origin.y) / dir.y;
	if (tBottom > 0.001f) {
		glm::vec3 p = origin + dir * tBottom;
		if (p.x >= -tableHalfX && p.x <= tableHalfX && p.z >= -tableHalfZ && p.z <= tableHalfZ) {
			t = tBottom; normal = glm::vec3(0.0f, -1.0f, 0.0f); return true;
		}
	}

	return false;
}

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
	float& t, glm::vec3& orientedNormal) {
	glm::vec3 faceNormal;
	if (!rayTriangleIntersect(origin, dir, v0, v1, v2, t, faceNormal)) return false;
	orientedNormal = glm::dot(dir, faceNormal) < 0.0f ? faceNormal : -faceNormal;
	return true;
}

/**
 * @brief Sprawdza przecięcie promienia z całym pryzmatem.
 * @param origin Punkt początkowy promienia.
 * @param dir Kierunek promienia.
 * @param t Zmienna wyjściowa przechowująca dystans do najbliższego przecięcia.
 * @param normal Normalna trafionej ściany pryzmatu.
 * @return true jeśli promień trafia w pryzmat.
 */
bool intersectPrism(const glm::vec3& origin, const glm::vec3& dir, float& t, glm::vec3& normal) {
	const glm::vec3 localVerts[6] = {
		glm::vec3(-0.8f, 0.0f, -0.5f), glm::vec3(0.8f, 0.0f, -0.5f), glm::vec3(0.0f, 0.0f,  0.8f),
		glm::vec3(-0.8f, 0.7f, -0.5f), glm::vec3(0.8f, 0.7f, -0.5f), glm::vec3(0.0f, 0.7f,  0.8f)
	};
	glm::mat4 prismModel = glm::translate(glm::mat4(1.0f), prismPos);
	glm::vec3 worldVerts[6];
	for (int i = 0; i < 6; ++i) {
		glm::vec4 worldPos = prismModel * glm::vec4(localVerts[i], 1.0f);
		worldVerts[i] = glm::vec3(worldPos);
	}
	const int triangles[8][3] = {
		{0, 1, 2}, {3, 5, 4}, {0, 1, 4}, {0, 4, 3},
		{1, 2, 5}, {1, 5, 4}, {2, 0, 3}, {2, 3, 5}
	};
	float bestT = 1e8f;
	glm::vec3 hitNormal;
	bool hit = false;
	for (int i = 0; i < 8; ++i) {
		float localT; glm::vec3 localNormal;
		if (intersectPrismFace(origin, dir, worldVerts[triangles[i][0]], worldVerts[triangles[i][1]], worldVerts[triangles[i][2]], localT, localNormal)) {
			if (localT < bestT) { bestT = localT; hitNormal = localNormal; hit = true; }
		}
	}
	if (hit) { t = bestT; normal = hitNormal; }
	return hit;
}

/**
 * @brief Sprawdza przecięcie promienia ze sferą.
 * @param origin Punkt początkowy promienia.
 * @param dir Kierunek promienia.
 * @param t Dystans do punktu trafienia.
 * @param normal Normalna powierzchni sfery w punkcie uderzenia.
 * @return true jeśli promień przecina sferę.
 */
bool intersectSphere(const glm::vec3& origin, const glm::vec3& dir, float& t, glm::vec3& normal) {
	glm::vec3 center(2.0f, -2.3f, 0.9f);
	float radius = 0.45f;
	glm::vec3 v = origin - center;
	float b = 2.0f * glm::dot(dir, v);
	float c = glm::dot(v, v) - radius * radius;
	float disc = b * b - 4.0f * c;
	if (disc < 0.0f) return false;
	float sqrtDisc = sqrt(disc);
	float t1 = (-b - sqrtDisc) / 2.0f;
	float t2 = (-b + sqrtDisc) / 2.0f;
	float localT = -1.0f;
	if (t1 > 0.001f) localT = t1;
	else if (t2 > 0.001f) localT = t2;
	else return false;
	t = localT;
	glm::vec3 hitPoint = origin + dir * t;
	glm::vec3 rawNormal = glm::normalize(hitPoint - center);
	normal = glm::dot(dir, rawNormal) < 0.0f ? rawNormal : -rawNormal;
	return true;
}

/**
 * @brief Sprawdza przecięcie promienia z cylindrem.
 * @param origin Punkt początkowy promienia.
 * @param dir Kierunek promienia.
 * @param model Macierz transformacji (model matrix) cylindra.
 * @param t Dystans do najbliższego punktu trafienia.
 * @param normal Normalna powierzchni na cylindrze.
 * @return true jeśli promień przecina cylinder.
 */
bool intersectCylinder(const glm::vec3& origin, const glm::vec3& dir, const glm::mat4& model, float& t, glm::vec3& normal) {
	glm::mat4 invM = glm::inverse(model);
	glm::vec3 ro = glm::vec3(invM * glm::vec4(origin, 1.0f));
	glm::vec3 rd = glm::vec3(invM * glm::vec4(dir, 0.0f));

	float a = rd.x * rd.x + rd.z * rd.z;
	float b = 2.0f * (rd.x * ro.x + rd.z * ro.z);
	float c = ro.x * ro.x + ro.z * ro.z - 1.0f;

	float bestT = 1e8f; glm::vec3 bestNormal; bool hit = false;

	if (fabs(a) > 1e-6f) {
		float disc = b * b - 4.0f * a * c;
		if (disc >= 0.0f) {
			float sqrtDisc = sqrt(disc);
			float t_arr[2] = { (-b - sqrtDisc) / (2.0f * a), (-b + sqrtDisc) / (2.0f * a) };
			for (int i = 0; i < 2; ++i) {
				if (t_arr[i] > 0.001f && t_arr[i] < bestT) {
					float yHit = ro.y + rd.y * t_arr[i];
					if (yHit >= 0.0f && yHit <= 1.0f) {
						bestT = t_arr[i];
						bestNormal = glm::vec3(ro.x + rd.x * bestT, 0.0f, ro.z + rd.z * bestT);
						hit = true;
					}
				}
			}
		}
	}
	if (fabs(rd.y) > 1e-6f) {
		float t_caps[2] = { (0.0f - ro.y) / rd.y, (1.0f - ro.y) / rd.y };
		for (int i = 0; i < 2; ++i) {
			if (t_caps[i] > 0.001f && t_caps[i] < bestT) {
				glm::vec3 hitPt = ro + rd * t_caps[i];
				if (hitPt.x * hitPt.x + hitPt.z * hitPt.z <= 1.0f) {
					bestT = t_caps[i];
					bestNormal = i == 0 ? glm::vec3(0.0f, -1.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
					hit = true;
				}
			}
		}
	}
	if (hit) {
		t = bestT;
		glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
		glm::vec3 worldNormal = glm::normalize(normalMatrix * bestNormal);
		normal = glm::dot(dir, worldNormal) < 0.0f ? worldNormal : -worldNormal;
	}
	return hit;
}

/**
 * @brief Sprawdza przecięcie promienia z płaskim lustrem.
 * @param origin Punkt początkowy promienia.
 * @param dir Kierunek promienia.
 * @param mirrorM Macierz transformacji lustra w przestrzeni.
 * @param t Dystans promienia do płaszczyzny lustra.
 * @param normal Wektor normalny powierzchni lustra.
 * @return true jeśli promień uderza w lustro.
 */
bool intersectMirror(const glm::vec3& origin, const glm::vec3& dir, const glm::mat4& mirrorM, float& t, glm::vec3& normal) {
	glm::vec3 v0 = glm::vec3(mirrorM * glm::vec4(-0.5f, -0.5f, 0.0f, 1.0f));
	glm::vec3 v1 = glm::vec3(mirrorM * glm::vec4(0.5f, -0.5f, 0.0f, 1.0f));
	glm::vec3 v2 = glm::vec3(mirrorM * glm::vec4(0.5f, 0.5f, 0.0f, 1.0f));
	glm::vec3 v3 = glm::vec3(mirrorM * glm::vec4(-0.5f, 0.5f, 0.0f, 1.0f));

	float bestT = 1e8f; glm::vec3 hitNormal; bool hit = false;
	float localT; glm::vec3 localNormal;

	if (rayTriangleIntersect(origin, dir, v0, v1, v2, localT, localNormal)) {
		if (localT < bestT) { bestT = localT; hitNormal = localNormal; hit = true; }
	}
	if (rayTriangleIntersect(origin, dir, v0, v2, v3, localT, localNormal)) {
		if (localT < bestT) { bestT = localT; hitNormal = localNormal; hit = true; }
	}
	if (hit) {
		t = bestT;
		hitNormal = glm::normalize(hitNormal);
		normal = glm::dot(dir, hitNormal) < 0.0f ? hitNormal : -hitNormal;
	}
	return hit;
}

/**
 * @brief Sprawdza przecięcie promienia z soczewką.
 * @param origin Punkt początkowy promienia.
 * @param dir Kierunek promienia.
 * @param model Macierz modelu soczewki (dla przekształceń).
 * @param t Zmienna przechowująca parametr T trafienia.
 * @param normal Wektor normalny trafionej płaszczyzny soczewki.
 * @return true w przypadku pomyślnej detekcji kolizji.
 */
bool intersectLens(const glm::vec3& origin, const glm::vec3& dir, const glm::mat4& model, float& t, glm::vec3& normal) {
	glm::mat4 invM = glm::inverse(model);
	glm::vec3 ro = glm::vec3(invM * glm::vec4(origin, 1.0f));
	glm::vec3 rd = glm::vec3(invM * glm::vec4(dir, 0.0f));

	float a = glm::dot(rd, rd);
	float b = 2.0f * glm::dot(ro, rd);
	float c = glm::dot(ro, ro) - 1.0f;

	float disc = b * b - 4.0f * a * c;
	if (disc < 0.0f) return false;

	float sqrtDisc = sqrt(disc);
	float t1 = (-b - sqrtDisc) / (2.0f * a);
	float t2 = (-b + sqrtDisc) / (2.0f * a);

	float localT = -1.0f;
	if (t1 > 0.001f) localT = t1;
	else if (t2 > 0.001f) localT = t2;
	else return false;

	t = localT;
	glm::vec3 localHit = ro + rd * t;
	glm::vec3 localNormal = glm::normalize(localHit);

	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
	glm::vec3 worldNormal = glm::normalize(normalMatrix * localNormal);
	normal = glm::dot(dir, worldNormal) < 0.0f ? worldNormal : -worldNormal;
	return true;
}

/**
 * @brief Sprawdza przecięcie promienia ze ścianami pokoju/sceny.
 * @param origin Punkt startu.
 * @param dir Kierunek strzału.
 * @param t Dystans odcinek z origin do pokoju.
 * @param normal Wektor prostopadły do wyznaczonej ściany.
 * @return true jeżeli promień przecina pokój.
 */
bool intersectRoom(const glm::vec3& origin, const glm::vec3& dir, float& t, glm::vec3& normal) {
	struct Wall { glm::vec3 normal; float d; glm::vec3 min; glm::vec3 max; };
	const float roomHalf = 10.0f;
	const Wall walls[6] = {
		{glm::vec3(0.0f,  1.0f,  0.0f), 5.0f, glm::vec3(-roomHalf, -roomHalf, -roomHalf), glm::vec3(roomHalf, roomHalf, roomHalf)},
		{glm::vec3(0.0f, -1.0f,  0.0f), 5.0f, glm::vec3(-roomHalf, -roomHalf, -roomHalf), glm::vec3(roomHalf, roomHalf, roomHalf)},
		{glm::vec3(1.0f,  0.0f,  0.0f), 10.0f, glm::vec3(-roomHalf, -roomHalf, -roomHalf), glm::vec3(roomHalf, roomHalf, roomHalf)},
		{glm::vec3(-1.0f,  0.0f,  0.0f), 10.0f, glm::vec3(-roomHalf, -roomHalf, -roomHalf), glm::vec3(roomHalf, roomHalf, roomHalf)},
		{glm::vec3(0.0f,  0.0f,  1.0f), 10.0f, glm::vec3(-roomHalf, -roomHalf, -roomHalf), glm::vec3(roomHalf, roomHalf, roomHalf)},
		{glm::vec3(0.0f,  0.0f, -1.0f), 10.0f, glm::vec3(-roomHalf, -roomHalf, -roomHalf), glm::vec3(roomHalf, roomHalf, roomHalf)}
	};
	float bestT = 1e8f; glm::vec3 bestNormal;
	for (int i = 0; i < 6; ++i) {
		float denom = glm::dot(dir, walls[i].normal);
		if (fabs(denom) < 0.0001f) continue;
		float localT = -(glm::dot(origin, walls[i].normal) + walls[i].d) / denom;
		if (localT <= 0.001f || localT >= bestT) continue;
		glm::vec3 hitPoint = origin + dir * localT;
		if (hitPoint.x < walls[i].min.x - 0.1f || hitPoint.x > walls[i].max.x + 0.1f) continue;
		if (hitPoint.y < walls[i].min.y - 0.1f || hitPoint.y > walls[i].max.y + 0.1f) continue;
		if (hitPoint.z < walls[i].min.z - 0.1f || hitPoint.z > walls[i].max.z + 0.1f) continue;
		bestT = localT; bestNormal = walls[i].normal;
	}
	if (bestT < 1e8f) { t = bestT; normal = bestNormal; return true; }
	return false;
}

/**
 * @brief Oblicza odległość między promieniem kamery a segmentem promienia światła.
 * @param ro Punkt początkowy promienia patrzenia kamery.
 * @param rd Kierunek patrzenia kamery.
 * @param p0 Pierwszy punkt segmentu światła.
 * @param p1 Drugi punkt segmentu światła.
 * @return Najmniejsza odległość przestrzenna między prostopadłymi zrzutami obu promieni.
 */
float intersectRaySegment(glm::vec3 ro, glm::vec3 rd, glm::vec3 p0, glm::vec3 p1) {
	glm::vec3 v = p1 - p0;
	glm::vec3 w = ro - p0;
	float b = glm::dot(rd, v);
	float c = glm::dot(v, v);
	float d = glm::dot(rd, w);
	float e = glm::dot(v, w);
	float D = 1.0f * c - b * b;
	float sc, tc;

	if (D < 1e-5f) {
		sc = 0.0f;
		tc = (b > c ? d / b : e / c);
	}
	else {
		sc = (b * e - c * d) / D;
		tc = (1.0f * e - b * d) / D;
	}

	// Przycięcie do brzegów segmentu
	if (tc < 0.0f) tc = 0.0f;
	else if (tc > 1.0f) tc = 1.0f;

	sc = glm::dot(p0 + tc * v - ro, rd);
	if (sc < 0.0f) return 1e10f; // Ignorowanie przypadku znajdującego się za kamerą

	glm::vec3 closest_ray = ro + sc * rd;
	glm::vec3 closest_seg = p0 + tc * v;
	return glm::length(closest_ray - closest_seg);
}

// --- OPTICS SIMULATION FUNCTIONS ---

/**
 * @brief Konwertuje długość fali na kolor RGBA.
 * @param wavelength Długość fali świata w mikrometrach (0.38 - 0.70).
 * @return Wektor koloru RGBA.
 */
glm::vec4 wavelengthToColor(float wavelength) {
	float R = 0.0f, G = 0.0f, B = 0.0f;
	if (wavelength < 0.44f) { R = 0.6f + 0.4f * (wavelength - 0.38f) / 0.06f; G = 0.0f; B = 1.0f; }
	else if (wavelength < 0.49f) { R = 0.0f; G = (wavelength - 0.44f) / 0.05f; B = 1.0f; }
	else if (wavelength < 0.53f) { R = 0.0f; G = 1.0f; B = 1.0f - (wavelength - 0.49f) / 0.04f; }
	else if (wavelength < 0.59f) { R = (wavelength - 0.53f) / 0.06f; G = 1.0f; B = 0.0f; }
	else { R = 1.0f; G = 1.0f - (wavelength - 0.59f) / 0.1f; B = 0.0f; }
	return glm::vec4(glm::clamp(R, 0.0f, 1.0f), glm::clamp(G, 0.0f, 1.0f), glm::clamp(B, 0.0f, 1.0f), 1.0f);
}

/**
 * @brief Oblicza wskaźnik załamania dla zadanej długości fali.
 * @param wavelength Długość fali w mikrometrach.
 * @return Współczynnik załamania światła.
 */
float refractiveIndexForWavelength(float wavelength) {
	return prismIndexOfRefraction + prismDispersion / (wavelength * wavelength);
}

/**
 * @brief Dodaje segment promienia do struktury BeamRay.
 * @param ray Referencja do promienia, do którego dodawany jest segment.
 * @param start Punkt początkowy segmentu.
 * @param end Punkt końcowy segmentu.
 * @param wavelength Długość fali dla tego segmentu.
 */
void addBeamSegment(BeamRay& ray, const glm::vec3& start, const glm::vec3& end, float wavelength) {
	ray.segments.push_back({ start, end, ray.color, wavelength });
}

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
void traceBeam(BeamRay& ray, const glm::vec3& origin, const glm::vec3& dir, bool inside, int depth, float currentIOR, float wavelength) {
	if (depth == 0) return;
	float tRoom = 0.0f, tPrism = 0.0f, tSphere = 0.0f, tCylinder = 0.0f;
	glm::vec3 nRoom, nPrism, nSphere, nCylinder;

	bool hitRoom = intersectRoom(origin, dir, tRoom, nRoom);
	bool hitPrism = intersectPrism(origin, dir, tPrism, nPrism);
	bool hitSphere = intersectSphere(origin, dir, tSphere, nSphere);
	bool hitCylinder = intersectCylinder(origin, dir, cylinderModel, tCylinder, nCylinder);

	float tLens = 0.0f; glm::vec3 nLens;
	bool hitLens = intersectLens(origin, dir, lensModel, tLens, nLens);

	float bestMirrorT = 1e8f; glm::vec3 bestMirrorNormal; bool hitMirror = false;
	for (const auto& m : mirrors) {
		float tTemp; glm::vec3 nTemp;
		if (intersectMirror(origin, dir, m, tTemp, nTemp)) {
			if (tTemp < bestMirrorT) { bestMirrorT = tTemp; bestMirrorNormal = nTemp; hitMirror = true; }
		}
	}

	float bestT = 1e8f; int hitType = 0; glm::vec3 normal;
	if (hitRoom && tRoom < bestT) { bestT = tRoom; hitType = 1; normal = nRoom; }
	if (hitPrism && tPrism < bestT) { bestT = tPrism; hitType = 2; normal = nPrism; }
	if (hitSphere && tSphere < bestT) { bestT = tSphere; hitType = 3; normal = nSphere; }
	if (hitCylinder && tCylinder < bestT) { bestT = tCylinder; hitType = 4; normal = nCylinder; }
	if (hitMirror && bestMirrorT < bestT) { bestT = bestMirrorT; hitType = 5; normal = bestMirrorNormal; }
	if (hitLens && tLens < bestT) { bestT = tLens; hitType = 6; normal = nLens; }

	// Check table intersection and treat as opaque surface (stop beams)
	{
		float tTable = 0.0f; glm::vec3 nTable;
		bool hitTable = intersectTable(origin, dir, tTable, nTable);
		if (hitTable && tTable < bestT) { bestT = tTable; hitType = 7; normal = nTable; }
	}

	if (hitType == 0) {
		addBeamSegment(ray, origin, origin + dir * 80.0f, wavelength);
		return;
	}

	glm::vec3 hitPoint = origin + dir * bestT;
	addBeamSegment(ray, origin, hitPoint, wavelength);

	if (hitType == 1 || hitType == 7) {
		return; // Stop at walls or table
	}
	else if (hitType == 5) {
		glm::vec3 reflected = glm::reflect(dir, normal);
		traceBeam(ray, hitPoint + reflected * 0.002f, reflected, inside, depth - 1, currentIOR, wavelength);
	}
	else {
		float n1 = inside ? currentIOR : 1.0f;
		float n2 = inside ? 1.0f : currentIOR;
		glm::vec3 refracted = glm::refract(dir, normal, n1 / n2);
		if (glm::dot(refracted, refracted) < 1e-6f) {
			glm::vec3 reflected = glm::reflect(dir, normal);
			traceBeam(ray, hitPoint + reflected * 0.002f, reflected, inside, depth - 1, currentIOR, wavelength);
		}
		else {
			traceBeam(ray, hitPoint + refracted * 0.002f, refracted, !inside, depth - 1, currentIOR, wavelength);
		}
	}
}

/**
 * @brief Symuluje wybuch promieni świetlnych z rozdzieleniem na długości fali.
 * @param origin Punkt startu wybuchu.
 * @param direction Kierunek wybuchu.
 */
void simulateBurst(const glm::vec3& origin, const glm::vec3& direction) {
	const int colorsCount = 12;
	const float startWavelength = 0.70f;
	const float endWavelength = 0.45f;
	for (int i = 0; i < colorsCount; ++i) {
		float wavelength = startWavelength + (endWavelength - startWavelength) * ((float)i / (colorsCount - 1));
		glm::vec4 rayColor = wavelengthToColor(wavelength);
		if (i > 0) rayColor = glm::mix(rayColor, wavelengthToColor(startWavelength), 0.2f);

		BeamRay ray;
		ray.color = glm::vec4(glm::vec3(rayColor), 1.0f);
		float currentIOR = refractiveIndexForWavelength(wavelength);
		traceBeam(ray, origin, direction, false, 12, currentIOR, wavelength);
		firedRays.push_back(ray);
	}
}