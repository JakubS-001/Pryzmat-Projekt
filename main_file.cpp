/**
 * @file main_file.cpp
 * @brief Plik główny aplikacji do symulacji promieni światła.
 * @details Zawiera punkt wejścia programu, inicjalizację bibliotek OpenGL, GLFW oraz ImGui,
 * a także główną pętlę renderowania, obsługę wejścia użytkownika (klawiatura, mysz)
 * oraz rysowanie sceny 3D z obiektami optycznymi (pryzmat, sfera, cylinder, soczewka, lustra).
 * @author Jakub Stasierski
 */

#define GLM_FORCE_RADIANS
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <cmath>
#include <string>
#include "rays.h"
#include "drawings.h"
#include "constants.h"
#include "allmodels.h"
#include "lodepng.h"
#include "shaderprogram.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

using namespace Models;

// --- GLOBAL VARIABLES ---
 int windowWidth = 1280;
 int windowHeight = 720;
 glm::vec3 cameraPos(0.0f, 1.5f, 9.0f);
 glm::vec3 cameraFront(0.0f, 0.0f, -1.0f);
 glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
 float cameraYaw = -90.0f;
 float cameraPitch = 0.0f;
 float lastX = 640.0f;
 float lastY = 360.0f;
 bool firstMouse = true;
 bool mouseUnlocked = false;
 bool altPressedLast = false;

// Key Tracking
 bool spacePressedLast = false;
 bool shiftPressedLast = false;
 bool tabPressedLast = false;
 bool keyBracketLast = false;
 bool keyBackslashLast = false;
 bool keySemicolonLast = false;
 bool keyApostropheLast = false;
 bool showLabels = true;

 std::vector<Burst> activeBursts;
 std::vector<BeamRay> firedRays;

 float prismIndexOfRefraction = 1.52f;
 float prismDispersion = 0.048f;
 float lastFrameTime = 0.0f;

// --- GLOBAL SCENE MATRICES ---
 glm::mat4 cylinderModel;
 std::vector<glm::mat4> mirrors;
 glm::mat4 lensModel;

 glm::vec3 lensPos = glm::vec3(1.5f, -2.38f, -1.8f);
 glm::vec3 lensScale = glm::vec3(0.1f, 0.5f, 0.5f);
 glm::vec3 cylinderPos = glm::vec3(-1.6f, -2.38f, 0.4f);
 glm::vec3 cylinderScale = glm::vec3(0.35f, 2.0f, 0.35f);
 glm::vec3 prismPos = glm::vec3(0.0f, -2.7f, 0.0f);

 glm::vec3 mirror1Pos = glm::vec3(0.0f, -2.3f, -2.0f);
 float mirror1Yaw = 35.0f;
 glm::vec3 mirror2Pos = glm::vec3(-3.0f, -2.3f, -1.0f);
 float mirror2Yaw = -55.0f;

/**
 * @brief Inicjalizuje i aktualizuje macierze modelu wszystkich obiektów sceny.
 * @details Buduje macierze transformacji dla cylindra (z obrotem poziomym i kątem 45°),
 * dwóch luster (z zadanymi pozycjami i kątami obrotu wokół osi Y) oraz soczewki.
 * Funkcja jest wywoływana przy starcie aplikacji oraz każdorazowo po zmianie
 * parametrów obiektów przez użytkownika za pośrednictwem panelu ImGui.
 */
void setupSceneObjects() {
	cylinderModel = glm::mat4(1.0f);
	cylinderModel = glm::translate(cylinderModel, cylinderPos);
	cylinderModel = glm::rotate(cylinderModel, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	cylinderModel = glm::rotate(cylinderModel, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	cylinderModel = glm::scale(cylinderModel, cylinderScale);

	mirrors.clear();

	glm::mat4 m1 = glm::translate(glm::mat4(1.0f), mirror1Pos);
	m1 = glm::rotate(m1, glm::radians(mirror1Yaw), glm::vec3(0.0f, 1.0f, 0.0f));
	m1 = glm::scale(m1, glm::vec3(1.4f, 1.0f, 1.0f));
	mirrors.push_back(m1);

	glm::mat4 m2 = glm::translate(glm::mat4(1.0f), mirror2Pos);
	m2 = glm::rotate(m2, glm::radians(mirror2Yaw), glm::vec3(0.0f, 1.0f, 0.0f));
	m2 = glm::scale(m2, glm::vec3(1.4f, 1.0f, 1.0f));
	mirrors.push_back(m2);

	lensModel = glm::mat4(1.0f);
	lensModel = glm::translate(lensModel, lensPos);
	lensModel = glm::rotate(lensModel, glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	lensModel = glm::scale(lensModel, lensScale)
}

// --- CALLBACKS ---
/**
 * @brief Callback wywoływany przez bibliotekę GLFW w przypadku wystąpienia błędu.
 * @details Wypisuje opis błędu na standardowe wyjście błędów (stderr).
 * Rejestrowany przy starcie aplikacji za pomocą glfwSetErrorCallback().
 * @param error Kod numeryczny błędu GLFW.
 * @param description Łańcuch znaków z opisem zaistniałego błędu.
 */
void error_callback(int error, const char* description) {
	fputs(description, stderr);
}

/**
 * @brief Callback wywoływany przez GLFW przy zmianie rozmiaru okna lub framebuffera.
 * @details Aktualizuje globalne zmienne szerokości i wysokości okna oraz wywołuje
 * glViewport(), aby dopasować obszar renderowania do nowych wymiarów.
 * @param window Wskaźnik na okno GLFW, którego rozmiar uległ zmianie.
 * @param width Nowa szerokość framebuffera w pikselach.
 * @param height Nowa wysokość framebuffera w pikselach.
 */
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	windowWidth = width;
	windowHeight = height;
	glViewport(0, 0, width, height);
}

/**
 * @brief Przelicza i aktualizuje wektor kierunku kamery na podstawie bieżących kątów obrotu.
 * @details Na podstawie globalnych zmiennych cameraYaw (obrót poziomy) oraz cameraPitch
 * (obrót pionowy) wyznacza znormalizowany wektor cameraFront, który wskazuje
 * kierunek patrzenia kamery w przestrzeni świata.
 */
void updateCameraFront() {
	glm::vec3 front;
	front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
	front.y = sin(glm::radians(cameraPitch));
	front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
	cameraFront = glm::normalize(front);
}

/**
 * @brief Callback wywoływany przez GLFW przy każdym ruchu kursora myszy.
 * @details Oblicza różnicę pozycji kursora między kolejnymi klatkami i przelicza ją
 * na zmianę kątów yaw oraz pitch kamery (z ograniczeniem pitch do ±89°).
 * Ruch kamery jest wstrzymywany, gdy mysz jest odblokowana (mouseUnlocked == true),
 * co pozwala na swobodną obsługę paneli ImGui.
 * @param window Wskaźnik na okno GLFW, w którym wykryto ruch kursora.
 * @param xpos Bieżąca pozioma pozycja kursora w pikselach.
 * @param ypos Bieżąca pionowa pozycja kursora w pikselach.
 */
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	if (mouseUnlocked) return;

	if (firstMouse) {
		lastX = (float)xpos;
		lastY = (float)ypos;
		firstMouse = false;
	}
	float xoffset = (float)xpos - lastX;
	float yoffset = lastY - (float)ypos;
	lastX = (float)xpos;
	lastY = (float)ypos;
	const float sensitivity = 0.12f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;
	cameraYaw += xoffset;
	cameraPitch += yoffset;
	if (cameraPitch > 89.0f) cameraPitch = 89.0f;
	if (cameraPitch < -89.0f) cameraPitch = -89.0f;
	updateCameraFront();
}


/**
 * @brief Główna funkcja rysująca scenę 3D oraz obsługująca wejście użytkownika w każdej klatce.
 * @details Wykonuje następujące operacje w ramach jednej klatki:
 * - Wyświetla panel ImGui z suwakami do sterowania parametrami optycznymi i pozycjami obiektów.
 * - Przetwarza ruch kamery (WASD, Ctrl) z uwzględnieniem kolizji ze ścianami pokoju i blatem stołu.
 * - Obsługuje przełączanie blokady myszy klawiszem Alt.
 * - Rejestruje i modyfikuje wybuchy promieni (SPACJA, SHIFT, strzałki, nawiasy, średnik, apostrof).
 * - Przelicza symulację optyczną dla wszystkich aktywnych wiązek i renderuje scenę:
 *   ściany pokoju, stół z nogami, lustra, półprzezroczyste obiekty szklane oraz linie promieni.
 * @param window Wskaźnik na aktywne okno GLFW używane do odczytu stanu klawiatury.
 */
void drawScene(GLFWwindow* window) {
	if (windowWidth <= 0 || windowHeight <= 0)
		return;
	float currentTime = (float)glfwGetTime();
	float deltaTime = currentTime - lastFrameTime;
	lastFrameTime = currentTime;

	bool sceneChanged = false;
	ImGui::SetNextWindowPos(ImVec2(10.0f, 120.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Sterowanie")) {
		if (ImGui::SliderFloat("Wsp. zalamania", &prismIndexOfRefraction, 1.00f, 2.80f, "%.3f")) sceneChanged = true;
		if (ImGui::SliderFloat("Dyspersja", &prismDispersion, 0.0f, 0.15f, "%.4f")) sceneChanged = true;

		ImGui::Separator();
		ImGui::Text("Cylinder");
		if (ImGui::SliderFloat3("Pozycja cylindra", glm::value_ptr(cylinderPos), -8.0f, 8.0f)) sceneChanged = true;
		
		float cyl_xz = (cylinderScale.x + cylinderScale.z) * 0.5f;
		if (ImGui::SliderFloat("Skala XZ", &cyl_xz, 0.01f, 2.0f)) { cylinderScale.x = cyl_xz; cylinderScale.z = cyl_xz; sceneChanged = true; }
		if (ImGui::SliderFloat("Skala Y", &cylinderScale.y, 0.01f, 8.0f)) sceneChanged = true;

		ImGui::Separator();
		ImGui::Text("Soczewka");
		if (ImGui::SliderFloat3("Pozycja soczewki", glm::value_ptr(lensPos), -8.0f, 8.0f)) sceneChanged = true;
		
		float lens_yz = (lensScale.y + lensScale.z) * 0.5f;
		if (ImGui::SliderFloat("Skala X", &lensScale.x, 0.01f, 2.0f)) sceneChanged = true;
		if (ImGui::SliderFloat("Skala YZ", &lens_yz, 0.01f, 2.0f)) { lensScale.y = lens_yz; lensScale.z = lens_yz; sceneChanged = true; }

		ImGui::Separator();
		ImGui::Text("Lustro 1");
		if (ImGui::SliderFloat3("Pozycja 1", glm::value_ptr(mirror1Pos), -8.0f, 8.0f)) sceneChanged = true;
		if (ImGui::SliderFloat("Obrot 1", &mirror1Yaw, 0.0f, 360.0f)) sceneChanged = true;
		ImGui::Text("Lustro 2");
		if (ImGui::SliderFloat3("Pozycja 2", glm::value_ptr(mirror2Pos), -8.0f, 8.0f)) sceneChanged = true;
		if (ImGui::SliderFloat("Obrot 2", &mirror2Yaw, 0.0f, 360.0f)) sceneChanged = true;

		ImGui::TextWrapped("Nacisnij [ALT] zeby zablokowac / odblokowac mysz");
	}
	ImGui::End();

	if (sceneChanged) setupSceneObjects();

	// Camera Movement
	float cameraSpeed = 5.5f * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += cameraFront * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= cameraFront * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) cameraPos -= cameraUp * cameraSpeed;


	{
		const float roomHalf = 10.0f;
		const float wallThickness = 0.2f; 
		const float halfThickness = wallThickness * 0.5f;
		const float margin = 0.5f;

		float xMin = -roomHalf + halfThickness + margin;
		float xMax = roomHalf - halfThickness - margin;
		float zMin = -roomHalf + halfThickness + margin;
		float zMax = roomHalf - halfThickness - margin;

		const float floorCenterY = -5.0f; const float ceilCenterY = 5.0f;
		const float floorHalfH = 0.1f; const float ceilHalfH = 0.1f;
		float yMin = floorCenterY + floorHalfH + margin; 
		float yMax = ceilCenterY - ceilHalfH - margin; 

		cameraPos.x = glm::clamp(cameraPos.x, xMin, xMax);
		cameraPos.z = glm::clamp(cameraPos.z, zMin, zMax);
		cameraPos.y = glm::clamp(cameraPos.y, yMin, yMax);

		const float tableCenterY = -3.2f;
		const float tableHalfX = 4.5f;
		const float tableHalfZ = 3.0f;
		const float tableHalfY = 0.2f * 0.5f;
		const float tableTopY = tableCenterY + tableHalfY;
		const float eyeClearance = 0.3f; 
		if (cameraPos.x >= -tableHalfX && cameraPos.x <= tableHalfX && cameraPos.z >= -tableHalfZ && cameraPos.z <= tableHalfZ) {
			if (cameraPos.y < tableTopY + eyeClearance) cameraPos.y = tableTopY + eyeClearance;
		}
	}

	bool altPressed = (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS);
	if (altPressed && !altPressedLast) {
		mouseUnlocked = !mouseUnlocked;
		glfwSetInputMode(window, GLFW_CURSOR, mouseUnlocked ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
		firstMouse = true;
	}
	altPressedLast = altPressed;

	bool spacePressed = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);
	if (spacePressed && !spacePressedLast) activeBursts.push_back({ cameraPos, cameraFront });
	spacePressedLast = spacePressed;

	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS && !shiftPressedLast) activeBursts.clear();
	shiftPressedLast = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);

	bool tabPressed = (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS);
	if (tabPressed && !tabPressedLast) showLabels = !showLabels;
	tabPressedLast = tabPressed;

	bool keyBracket = glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS;
	bool keyBackslash = glfwGetKey(window, GLFW_KEY_BACKSLASH) == GLFW_PRESS;
	bool keySemicolon = glfwGetKey(window, GLFW_KEY_SEMICOLON) == GLFW_PRESS;
	bool keyApostrophe = glfwGetKey(window, GLFW_KEY_APOSTROPHE) == GLFW_PRESS;

	if (!activeBursts.empty()) {
		Burst& lastBurst = activeBursts.back();
		float moveSpeed = 1.0f * deltaTime;

		
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) lastBurst.origin.y += moveSpeed;
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) lastBurst.origin.y -= moveSpeed;
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) lastBurst.origin.x += moveSpeed;
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) lastBurst.origin.x -= moveSpeed;
		if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS) lastBurst.origin.z += moveSpeed;
		if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) lastBurst.origin.z -= moveSpeed;

		if (keySemicolon) {
			lastBurst.dir = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(1.0f), glm::vec3(0, 1, 0))) * lastBurst.dir;
		}
		if (keyBackslash) {
			lastBurst.dir = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(-1.0f), glm::vec3(0, 1, 0))) * lastBurst.dir;
		}
		if (keyBracket) {
			glm::vec3 right = glm::cross(lastBurst.dir, glm::vec3(0, 1, 0));
			right = glm::length(right) < 0.001f ? glm::vec3(1, 0, 0) : glm::normalize(right);
			lastBurst.dir = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(1.0f), right)) * lastBurst.dir;
		}
		if (keyApostrophe) {
			glm::vec3 right = glm::cross(lastBurst.dir, glm::vec3(0, 1, 0));
			right = glm::length(right) < 0.001f ? glm::vec3(1, 0, 0) : glm::normalize(right);
			lastBurst.dir = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(-1.0f), right)) * lastBurst.dir;
		}
		lastBurst.dir = glm::normalize(lastBurst.dir);
	}

	keyBracketLast = keyBracket;
	keyBackslashLast = keyBackslash;
	keyApostropheLast = keyApostrophe;
	keySemicolonLast = keySemicolon;


	firedRays.clear();
	for (const Burst& b : activeBursts) {
		simulateBurst(b.origin, b.dir);
	}

	char title[128];
	sprintf_s(title, sizeof(title), "Grafika Projekt Jakub Stasierski 163992");
	glfwSetWindowTitle(window, title);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 P = glm::perspective(glm::radians(70.0f), (float)windowWidth / (float)windowHeight, 0.1f, 120.0f);
	glm::mat4 V = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	const float roomHalf = 10.0f;
	glm::mat4 model;


	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -5.0f, 0.0f));
	model = glm::scale(model, glm::vec3(roomHalf * 2.0f, 0.2f, roomHalf * 2.0f));
	drawCube(spLambert, P, V, model, glm::vec4(0.48f, 0.45f, 0.4f, 1.0f));
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 5.0f, 0.0f));
	model = glm::scale(model, glm::vec3(roomHalf * 2.0f, 0.2f, roomHalf * 2.0f));
	drawCube(spLambert, P, V, model, glm::vec4(0.24f, 0.26f, 0.29f, 1.0f));
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -roomHalf));
	model = glm::scale(model, glm::vec3(roomHalf * 2.0f, roomHalf * 2.0f, 0.2f));
	drawCube(spLambert, P, V, model, glm::vec4(0.43f, 0.48f, 0.53f, 1.0f));
	model = glm::translate(glm::mat4(1.0f), glm::vec3(-roomHalf, 0.0f, 0.0f));
	model = glm::scale(model, glm::vec3(0.2f, roomHalf * 2.0f, roomHalf * 2.0f));
	drawCube(spLambert, P, V, model, glm::vec4(0.45f, 0.42f, 0.38f, 1.0f));
	model = glm::translate(glm::mat4(1.0f), glm::vec3(roomHalf, 0.0f, 0.0f));
	model = glm::scale(model, glm::vec3(0.2f, roomHalf * 2.0f, roomHalf * 2.0f));
	drawCube(spLambert, P, V, model, glm::vec4(0.51f, 0.47f, 0.42f, 1.0f));
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, roomHalf));
	model = glm::scale(model, glm::vec3(roomHalf * 2.0f, roomHalf * 2.0f, 0.2f));
	drawCube(spLambert, P, V, model, glm::vec4(0.42f, 0.40f, 0.43f, 1.0f));

	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -3.2f, 0.0f));
	model = glm::scale(model, glm::vec3(4.5f, 0.2f, 3.0f));
	drawCube(spLambert, P, V, model, glm::vec4(0.40f, 0.24f, 0.12f, 1.0f));

	for (int i = 0; i < 4; ++i) {
		float x = (i < 2) ? -4.0f : 4.0f;
		float z = (i % 2 == 0) ? -2.5f : 2.5f;
		glm::mat4 leg = glm::translate(glm::mat4(1.0f), glm::vec3(x, -5.0f, z));
		leg = glm::scale(leg, glm::vec3(0.3f, 1.8f, 0.3f));
		drawCube(spLambert, P, V, leg, glm::vec4(0.36f, 0.18f, 0.08f, 1.0f));
	}

	for (const auto& m : mirrors) {
		drawMirrorObject(P, V, m);
	}


	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);

	// Render Glass Objects
	glm::mat4 prismModel = glm::translate(glm::mat4(1.0f), prismPos);
	drawPrism(P, V, prismModel, glm::vec4(0.6f, 0.9f, 1.0f, 0.20f));
	drawSphere(P, V, glm::vec3(2.0f, -2.3f, 0.9f), 0.45f, glm::vec4(0.9f, 0.7f, 1.0f, 0.25f));
	drawCylinder(P, V, cylinderModel, glm::vec4(0.6f, 1.0f, 0.8f, 0.25f));
	drawEllipsoid(P, V, lensModel, glm::vec4(0.8f, 0.9f, 1.0f, 0.25f));

	drawBeamLines(P, V);

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}

/**
 * @brief Inicjalizuje wszystkie zasoby OpenGL, shadery oraz bibliotekę ImGui.
 * @details Ładuje shadery, wywołuje setupSceneObjects(), konfiguruje stan OpenGL
 * (test głębokości, mieszanie alfa, kolor czyszczenia), rejestruje callbacki GLFW
 * (ruch myszy, zmiana rozmiaru okna) oraz inicjalizuje kontekst ImGui z backendem
 * OpenGL3 i GLFW. Kursor myszy jest na starcie ukrywany i przechwytywany.
 * @param window Wskaźnik na okno GLFW, z którym zostanie powiązany kontekst renderowania.
 */
void initOpenGLProgram(GLFWwindow* window) {
	initShaders();
	setupSceneObjects();
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.20f, 0.22f, 0.26f, 1.0f);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	lastFrameTime = (float)glfwGetTime();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");

	io.Fonts->Build();
}

/**
 * @brief Zwalnia zasoby OpenGL i czyści pamięć po zakończeniu działania aplikacji.
 * @details Wywołuje freeShaders() w celu usunięcia skompilowanych programów shaderów.
 * Powinna być wywoływana tuż przed zniszczeniem okna GLFW.
 * @param window Wskaźnik na okno GLFW.
 */
void freeOpenGLProgram(GLFWwindow* window) {
	freeShaders();
}

/**
 * @brief Punkt wejścia aplikacji symulatora promieni światła.
 * @details Inicjalizuje bibliotekę GLFW, tworzy okno OpenGL o wymiarach 1280×720,
 * inicjalizuje GLEW oraz wszystkie zasoby programu (initOpenGLProgram()).
 * Następnie uruchamia główną pętlę renderowania, w której kolejno:
 * - budowana jest nowa ramka ImGui,
 * - rysowana jest scena 3D (drawScene()),
 * - rysowany jest nakładkowy HUD z celownikiem (drawCrosshairHUD()),
 * - ImGui jest renderowane na ekran, a bufory wymieniane.
 * Po zamknięciu okna zasoby są zwalniane i program kończy działanie kodem EXIT_SUCCESS.
 * @return Kod zakończenia procesu (EXIT_SUCCESS lub EXIT_FAILURE przy błędzie inicjalizacji).
 */
int main(void) {
	GLFWwindow* window;
	glfwSetErrorCallback(error_callback);
	if (!glfwInit()) {
		fprintf(stderr, "Nie mozna zainicjowac GLFW.\n");
		exit(EXIT_FAILURE);
	}
	window = glfwCreateWindow(windowWidth, windowHeight, "OpenGL Prism Lab", NULL, NULL);
	if (!window) {
		fprintf(stderr, "Nie mozna utworzyc okna.\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Nie mozna zainicjowac GLEW.\n");
		exit(EXIT_FAILURE);
	}
	initOpenGLProgram(window);
	while (!glfwWindowShouldClose(window)) {

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		drawScene(window);

		drawCrosshairHUD();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	freeOpenGLProgram(window);
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}