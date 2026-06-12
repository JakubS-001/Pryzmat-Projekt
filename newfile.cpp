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
#include "constants.h"
#include "allmodels.h"
#include "lodepng.h"
#include "shaderprogram.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

using namespace Models;

struct BeamSegment {
	glm::vec3 start;
	glm::vec3 end;
	glm::vec4 color;
	float wavelength;
};

struct BeamRay {
	std::vector<BeamSegment> segments;
	glm::vec4 color;
};

struct Burst {
	glm::vec3 origin;
	glm::vec3 dir;
};

// --- GLOBAL VARIABLES ---
static int windowWidth = 800;
static int windowHeight = 600;
static glm::vec3 cameraPos(0.0f, 1.5f, 9.0f);
static glm::vec3 cameraFront(0.0f, 0.0f, -1.0f);
static glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
static float cameraYaw = -90.0f;
static float cameraPitch = 0.0f;
static float lastX = 400.0f;
static float lastY = 300.0f;
static bool firstMouse = true;
static bool mouseUnlocked = false; // Toggled by Alt key

// Key Tracking
static bool spacePressedLast = false;
static bool shiftPressedLast = false;
static bool tabPressedLast = false;
static bool altPressedLast = false;
static bool keyBracketLast = false;
static bool keyBackslashLast = false;
static bool keySemicolonLast = false;
static bool keyApostropheLast = false;
static bool showLabels = true;

static std::vector<Burst> activeBursts;
static std::vector<BeamRay> firedRays;

// Optics & Simulation Parameters (Controlled via ImGui)
static float prismIndexOfRefraction = 1.52f;
static float prismDispersion = 0.048f;
static float lastFrameTime = 0.0f;

// Lens Object Parameters
static glm::vec3 lensPos(1.2f, -1.9f, -1.0f);
static glm::vec3 lensScale(1.5f, 1.5f, 0.35f); // Flattened on Z to create biconvex lens shape

// --- GLOBAL SCENE MATRICES ---
static glm::mat4 cylinderModel;
static glm::mat4 lensModel;
static std::vector<glm::mat4> mirrors;

void setupSceneObjects() {
	// Cylinder (Horizontal)
	cylinderModel = glm::mat4(1.0f);
	cylinderModel = glm::translate(cylinderModel, glm::vec3(-1.6f, -2.3f, 0.4f)); 
	cylinderModel = glm::rotate(cylinderModel, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)); 
	cylinderModel = glm::rotate(cylinderModel, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f)); 
	cylinderModel = glm::scale(cylinderModel, glm::vec3(0.35f, 2.0f, 0.35f)); 

	// Re-calculate Lens Transformation Matrix
	lensModel = glm::translate(glm::mat4(1.0f), lensPos);
	lensModel = glm::scale(lensModel, lensScale);

	// Mirrors 
	mirrors.clear();
	glm::mat4 m1 = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.8f, -2.0f));
	m1 = glm::rotate(m1, glm::radians(35.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	m1 = glm::scale(m1, glm::vec3(1.4f, 1.0f, 1.0f));
	mirrors.push_back(m1);
}

// --- CALLBACKS ---
void error_callback(int error, const char* description) {
	fputs(description, stderr);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	windowWidth = width;
	windowHeight = height;
	glViewport(0, 0, width, height);
}

void updateCameraFront() {
	glm::vec3 front;
	front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
	front.y = sin(glm::radians(cameraPitch));
	front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
	cameraFront = glm::normalize(front);
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	if (mouseUnlocked) return; // Do not update camera angle if mouse is unlocked for UI sliders

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

// --- RAY INTERSECTION MATH ---
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

bool intersectPrismFace(const glm::vec3& origin, const glm::vec3& dir,
	const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
	float& t, glm::vec3& orientedNormal) {
	glm::vec3 faceNormal;
	if (!rayTriangleIntersect(origin, dir, v0, v1, v2, t, faceNormal)) return false;
	orientedNormal = glm::dot(dir, faceNormal) < 0.0f ? faceNormal : -faceNormal;
	return true;
}

bool intersectPrism(const glm::vec3& origin, const glm::vec3& dir, float& t, glm::vec3& normal) {
	const glm::vec3 localVerts[6] = {
		glm::vec3(-0.8f, 0.0f, -0.5f), glm::vec3(0.8f, 0.0f, -0.5f), glm::vec3(0.0f, 0.0f,  0.8f),
		glm::vec3(-0.8f, 0.7f, -0.5f), glm::vec3(0.8f, 0.7f, -0.5f), glm::vec3(0.0f, 0.7f,  0.8f)
	};
	glm::mat4 prismModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.3f, 0.0f));
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

bool intersectSphere(const glm::vec3& origin, const glm::vec3& dir, float& t, glm::vec3& normal) {
	glm::vec3 center(1.6f, -1.9f, 0.4f);
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

// Biconvex Lens analytical matrix tracer
bool intersectLens(const glm::vec3& origin, const glm::vec3& dir, float& t, glm::vec3& normal) {
	glm::mat4 invM = glm::inverse(lensModel);
	glm::vec3 ro = glm::vec3(invM * glm::vec4(origin, 1.0f));
	glm::vec3 rd = glm::vec3(invM * glm::vec4(dir, 0.0f));
	float scaleFactor = glm::length(rd);
	if (scaleFactor < 1e-6f) return false;
	rd = glm::normalize(rd);

	// Unit Sphere intersection
	float b = 2.0f * glm::dot(rd, ro);
	float c = glm::dot(ro, ro) - 1.0f;
	float disc = b * b - 4.0f * c;
	if (disc < 0.0f) return false;
	
	float sqrtDisc = sqrt(disc);
	float t1 = (-b - sqrtDisc) / 2.0f;
	float t2 = (-b + sqrtDisc) / 2.0f;
	float localT = -1.0f;
	if (t1 > 0.001f) localT = t1;
	else if (t2 > 0.001f) localT = t2;
	else return false;

	t = localT / scaleFactor;
	glm::vec3 localHitPoint = ro + rd * localT;
	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(lensModel)));
	glm::vec3 worldNormal = glm::normalize(normalMatrix * localHitPoint);
	normal = glm::dot(dir, worldNormal) < 0.0f ? worldNormal : -worldNormal;
	return true;
}

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

// --- OPTICS SIMULATION ---
glm::vec4 wavelengthToColor(float wavelength) {
	float R = 0.0f, G = 0.0f, B = 0.0f;
	if (wavelength < 0.44f) { R = 0.6f + 0.4f * (wavelength - 0.38f) / 0.06f; G = 0.0f; B = 1.0f; }
	else if (wavelength < 0.49f) { R = 0.0f; G = (wavelength - 0.44f) / 0.05f; B = 1.0f; }
	else if (wavelength < 0.53f) { R = 0.0f; G = 1.0f; B = 1.0f - (wavelength - 0.49f) / 0.04f; }
	else if (wavelength < 0.59f) { R = (wavelength - 0.53f) / 0.06f; G = 1.0f; B = 0.0f; }
	else { R = 1.0f; G = 1.0f - (wavelength - 0.59f) / 0.1f; B = 0.0f; }
	return glm::vec4(glm::clamp(R, 0.0f, 1.0f), glm::clamp(G, 0.0f, 1.0f), glm::clamp(B, 0.0f, 1.0f), 1.0f);
}

float refractiveIndexForWavelength(float wavelength) {
	return prismIndexOfRefraction + prismDispersion / (wavelength * wavelength);
}

void addBeamSegment(BeamRay& ray, const glm::vec3& start, const glm::vec3& end, float wavelength) {
	ray.segments.push_back({ start, end, ray.color, wavelength });
}

void traceBeam(BeamRay& ray, const glm::vec3& origin, const glm::vec3& dir, bool inside, int depth, float currentIOR, float wavelength) {
	if (depth == 0) return;
	float tRoom = 0.0f, tPrism = 0.0f, tSphere = 0.0f, tCylinder = 0.0f, tLens = 0.0f;
	glm::vec3 nRoom, nPrism, nSphere, nCylinder, nLens;

	bool hitRoom = intersectRoom(origin, dir, tRoom, nRoom);
	bool hitPrism = intersectPrism(origin, dir, tPrism, nPrism);
	bool hitSphere = intersectSphere(origin, dir, tSphere, nSphere);
	bool hitCylinder = intersectCylinder(origin, dir, cylinderModel, tCylinder, nCylinder);
	bool hitLens = intersectLens(origin, dir, tLens, nLens);

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

	if (hitType == 0) {
		addBeamSegment(ray, origin, origin + dir * 80.0f, wavelength);
		return;
	}

	glm::vec3 hitPoint = origin + dir * bestT;
	addBeamSegment(ray, origin, hitPoint, wavelength);

	if (hitType == 1) {
		return; 
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
		traceBeam(ray, origin, direction, false, 8, currentIOR, wavelength);
		firedRays.push_back(ray);
	}
}

// --- DRAWING FUNCTIONS ---
void drawCube(ShaderProgram* sp, const glm::mat4& P, const glm::mat4& V, const glm::mat4& M, const glm::vec4& color) {
	sp->use();
	glUniformMatrix4fv(sp->u("P"), 1, GL_FALSE, glm::value_ptr(P));
	glUniformMatrix4fv(sp->u("V"), 1, GL_FALSE, glm::value_ptr(V));
	glUniformMatrix4fv(sp->u("M"), 1, GL_FALSE, glm::value_ptr(M));
	glUniform4fv(sp->u("color"), 1, glm::value_ptr(color));
	glm::vec4 viewLightDir = V * glm::vec4(glm::normalize(glm::vec3(-0.3f, 1.0f, -0.5f)), 0.0f);
	glUniform4fv(sp->u("lightDir"), 1, glm::value_ptr(viewLightDir));
	Models::cube.drawSolid(true);
}

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

void drawLensObject(const glm::mat4& P, const glm::mat4& V, const glm::mat4& M, const glm::vec4& color) {
	static std::vector<float> verts; static std::vector<float> norms;
	if (verts.empty()) {
		int stacks = 16, slices = 16;
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
	glUniform4fv(spConstant->u("color"), 1, glm::value_ptr(glm::vec4(0.7f, 0.8f, 0.9f, 0.6f)));
	glEnableVertexAttribArray(0); glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, mirrorVertices);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, mirrorNormals);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(0); glDisableVertexAttribArray(1);
}

void drawLabelBackground(const glm::mat4& P, const glm::mat4& V, const glm::vec3& center, float width, float height, const glm::vec3& right, const glm::vec3& up) {
	float hw = width / 2.0f;
	float hh = height / 2.0f;
	glm::vec3 p1 = center - right * hw - up * hh;
	glm::vec3 p2 = center + right * hw - up * hh;
	glm::vec3 p3 = center + right * hw + up * hh;
	glm::vec3 p4 = center - right * hw + up * hh;

	float verts[] = {
		p1.x, p1.y, p1.z, 1.0f,  p2.x, p2.y, p2.z, 1.0f,  p3.x, p3.y, p3.z, 1.0f,
		p1.x, p1.y, p1.z, 1.0f,  p3.x, p3.y, p3.z, 1.0f,  p4.x, p4.y, p4.z, 1.0f
	};
	
	// Light target fix so backgrounds don't wash out to pure white
	float norms[24] = {
		-0.3f, 1.0f, -0.5f, 0.0f, -0.3f, 1.0f, -0.5f, 0.0f, -0.3f, 1.0f, -0.5f, 0.0f,
		-0.3f, 1.0f, -0.5f, 0.0f, -0.3f, 1.0f, -0.5f, 0.0f, -0.3f, 1.0f, -0.5f, 0.0f
	};

	spConstant->use();
	glUniformMatrix4fv(spConstant->u("P"), 1, GL_FALSE, glm::value_ptr(P));
	glUniformMatrix4fv(spConstant->u("V"), 1, GL_FALSE, glm::value_ptr(V));
	glUniformMatrix4fv(spConstant->u("M"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
	glUniform4fv(spConstant->u("color"), 1, glm::value_ptr(glm::vec4(0.12f, 0.12f, 0.14f, 0.85f))); 

	glEnableVertexAttribArray(0); glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, verts);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, norms);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(0); glDisableVertexAttribArray(1);
}

void drawStrokeChar(char c, const glm::vec3& pos, const glm::vec3& right, const glm::vec3& up, float scale, const glm::vec4& color) {
	std::vector<float> lineVerts;
	auto addL = [&](float x1, float y1, float x2, float y2) {
		glm::vec3 p1 = pos + right * (x1 * scale) + up * (y1 * scale);
		glm::vec3 p2 = pos + right * (x2 * scale) + up * (y2 * scale);
		lineVerts.insert(lineVerts.end(), { p1.x, p1.y, p1.z, 1.0f, p2.x, p2.y, p2.z, 1.0f });
	};
	switch (c) {
	case '0': addL(0, 0, 1, 0); addL(1, 0, 1, 1); addL(1, 1, 0, 1); addL(0, 1, 0, 0); addL(0, 0, 1, 1); break;
	case '1': addL(0.5f, 0, 0.5f, 1); addL(0.2f, 0.8f, 0.5f, 1); break;
	case '2': addL(0, 1, 1, 1); addL(1, 1, 1, 0.5f); addL(1, 0.5f, 0, 0.5f); addL(0, 0.5f, 0, 0); addL(0, 0, 1, 0); break;
	case '3': addL(0, 1, 1, 1); addL(1, 1, 1, 0); addL(1, 0, 0, 0); addL(0, 0.5f, 1, 0.5f); break;
	case '4': addL(0, 1, 0, 0.5f); addL(0, 0.5f, 1, 0.5f); addL(1, 1, 1, 0); break;
	case '5': addL(1, 1, 0, 1); addL(0, 1, 0, 0.5f); addL(0, 0.5f, 1, 0.5f); addL(1, 0.5f, 1, 0); addL(1, 0, 0, 0); break;
	case '6': addL(1, 1, 0, 1); addL(0, 1, 0, 0); addL(0, 0, 1, 0); addL(1, 0, 1, 0.5f); addL(1, 0.5f, 0, 0.5f); break;
	case '7': addL(0, 1, 1, 1); addL(1, 1, 0.5f, 0); break;
	case '8': addL(0, 0, 1, 0); addL(1, 0, 1, 1); addL(1, 1, 0, 1); addL(0, 1, 0, 0); addL(0, 0.5f, 1, 0.5f); break;
	case '9': addL(0, 0.5f, 1, 0.5f); addL(0, 0.5f, 0, 1); addL(0, 1, 1, 1); addL(1, 1, 1, 0); addL(1, 0, 0, 0); break;
	case 'n': addL(0, 0, 0, 0.6f); addL(0, 0.6f, 1, 0.6f); addL(1, 0.6f, 1, 0); break;
	case 'm': addL(0, 0, 0, 0.6f); addL(0, 0.6f, 0.5f, 0.6f); addL(0.5f, 0.6f, 0.5f, 0); addL(0.5f, 0.6f, 1, 0.6f); addL(1, 0.6f, 1, 0); break;
	}
	if (lineVerts.empty()) return;

	// CRITICAL FIX: Align vectors to the shader's diffuse light direction (-0.3, 1.0, -0.5)
	// This forces text lines to render with 100% luminous brightness instead of black!
	std::vector<float> lightMatchedNormals(lineVerts.size(), 0.0f);
	for (size_t i = 0; i < lineVerts.size(); i += 4) {
		lightMatchedNormals[i] = -0.3f;
		lightMatchedNormals[i + 1] = 1.0f;
		lightMatchedNormals[i + 2] = -0.5f;
	}

	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, lineVerts.data());
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, lightMatchedNormals.data());
	glLineWidth(2.5f);
	glUniform4fv(spConstant->u("color"), 1, glm::value_ptr(color));
	glDrawArrays(GL_LINES, 0, (GLsizei)(lineVerts.size() / 4));
}

void drawStrokeString(const std::string& str, const glm::vec3& pos, const glm::vec3& right, const glm::vec3& up, float scale, const glm::vec4& color) {
	glm::vec3 curPos = pos - right * ((str.length() * scale * 1.2f) / 2.0f);
	for (char c : str) {
		drawStrokeChar(c, curPos, right, up, scale, color);
		curPos += right * (scale * 1.3f);
	}
}

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
	
	// Additive blending for energetic beam cores
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	for (const BeamRay& ray : firedRays) {
		for (const BeamSegment& seg : ray.segments) {
			float lineVertices[8] = {
				seg.start.x, seg.start.y, seg.start.z, 1.0f,
				seg.end.x, seg.end.y, seg.end.z, 1.0f
			};
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, lineVertices);
			glLineWidth(5.0f);
			glUniform4fv(spConstant->u("color"), 1, glm::value_ptr(seg.color));
			glDrawArrays(GL_LINES, 0, 2);
		}
	}

	// Normal alpha blending for clean UI layout text elements
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (showLabels) {
		// FIX: Calculate true dynamic billboarding coordinates for vertical and horizontal camera tracking
		glm::vec3 camRight = glm::normalize(glm::cross(cameraFront, cameraUp));
		glm::vec3 camUp = glm::normalize(glm::cross(camRight, cameraFront)); 
		float scale = 0.05f;

		// Disable depth buffer test temporarily so text renders layered neatly on top
		glDisable(GL_DEPTH_TEST);

		for (const BeamRay& ray : firedRays) {
			if (ray.segments.empty()) continue;
			
			// GRAB ONLY THE FINAL SEGMENT BEFORE COLLIDING WITH ROOM WALLS
			const BeamSegment& seg = ray.segments.back();

			if (glm::distance(seg.start, seg.end) > 0.4f) {
				glm::vec3 mid = (seg.start + seg.end) * 0.5f;
				char buf[16];
				sprintf_s(buf, "%dnm", (int)(seg.wavelength * 1000.0f));
				
				glm::vec3 textPos = mid + camUp * 0.12f;
				float textWidth = std::string(buf).length() * scale * 1.3f;
				
				glm::vec3 bgCenter = textPos + camUp * (scale / 2.0f);
				drawLabelBackground(P, V, bgCenter, textWidth + 0.06f, scale + 0.06f, camRight, camUp);
				drawStrokeString(buf, textPos, camRight, camUp, scale, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			}
		}
		glEnable(GL_DEPTH_TEST);
	}

	glDisableVertexAttribArray(0); glDisableVertexAttribArray(1);
	glDisable(GL_BLEND);
}

void drawScene(GLFWwindow* window) {
	float currentTime = (float)glfwGetTime();
	float deltaTime = currentTime - lastFrameTime;
	lastFrameTime = currentTime;
	
	// Start ImGui Layout Construction
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// Create Optics Control Room Window Panel
	ImGui::Begin("Optics Laboratory - Configuration Panel");
	ImGui::Text("Press [ALT] to switch between Free Look and UI Slider Mouse focus.");
	ImGui::Separator();
	
	ImGui::Text("Glass Density Attributes:");
	ImGui::SliderFloat("Prism IOR (Base n)", &prismIndexOfRefraction, 1.00f, 2.80f, "%.3f");
	ImGui::SliderFloat("Prism Dispersion Value", &prismDispersion, 0.000f, 0.150f, "%.4f");
	
	ImGui::Separator();
	ImGui::Text("Biconvex Recombination Lens Settings:");
	if (ImGui::DragFloat3("Lens Position (X/Y/Z)", glm::value_ptr(lensPos), 0.05f, -8.0f, 8.0f)) {
		setupSceneObjects();
	}
	if (ImGui::SliderFloat("Lens Diameter", &lensScale.x, 0.5f, 3.5f, "%.2f")) {
		lensScale.y = lensScale.x; // Keep aspect proportional
		setupSceneObjects();
	}
	if (ImGui::SliderFloat("Lens Profile Thickness", &lensScale.z, 0.05f, 1.2f, "%.2f")) {
		setupSceneObjects();
	}

	ImGui::End();

	// Camera Flight Handling
	float cameraSpeed = 5.5f * deltaTime;
	if (!mouseUnlocked) {
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += cameraFront * cameraSpeed;
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= cameraFront * cameraSpeed;
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
		if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) cameraPos -= cameraUp * cameraSpeed;
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) cameraPos += cameraUp * cameraSpeed;
	}
	
	// Fire Beam Handling (Triggered via Keyboard Key Space or backspace alternatives)
	bool spacePressed = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && mouseUnlocked); // Fired via space bar while operating UI
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) spacePressed = true; // Secondary fire key mapping

	if (spacePressed && !spacePressedLast) activeBursts.push_back({cameraPos + cameraFront * 0.2f, cameraFront});
	spacePressedLast = spacePressed;

	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS && !shiftPressedLast) activeBursts.clear();
	shiftPressedLast = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);

	bool tabPressed = (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS);
	if (tabPressed && !tabPressedLast) showLabels = !showLabels;
	tabPressedLast = tabPressed;

	// Mouse Lock Input Toggle Switch (Alt Key Check)
	bool altPressed = (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS);
	if (altPressed && !altPressedLast) {
		mouseUnlocked = !mouseUnlocked;
		if (mouseUnlocked) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		} else {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			firstMouse = true;
		}
	}
	altPressedLast = altPressed;

	// Micro Angles Setup Adjustment Logic for Last Beam Segment Path Tuning
	bool keyBracket = glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS;
	bool keyBackslash = glfwGetKey(window, GLFW_KEY_BACKSLASH) == GLFW_PRESS;
	bool keySemicolon = glfwGetKey(window, GLFW_KEY_SEMICOLON) == GLFW_PRESS;
	bool keyApostrophe = glfwGetKey(window, GLFW_KEY_APOSTROPHE) == GLFW_PRESS;

	if (!activeBursts.empty()) {
		Burst& lastBurst = activeBursts.back();
		float moveSpeed = 1.0f * deltaTime;
		
		// Move Origin seamlessly with up/down arrows
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) lastBurst.origin.y += moveSpeed;
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) lastBurst.origin.y -= moveSpeed;
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) lastBurst.origin.x += moveSpeed;
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) lastBurst.origin.x -= moveSpeed;

		// Rotate pitch & yaw incrementally by precise 1 degree bounds
		if (keyBracket && !keyBracketLast) {
			lastBurst.dir = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(1.0f), glm::vec3(0, 1, 0))) * lastBurst.dir;
		}
		if (keyBackslash && !keyBackslashLast) {
			lastBurst.dir = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(-1.0f), glm::vec3(0, 1, 0))) * lastBurst.dir;
		}
		if (keyApostrophe && !keyApostropheLast) {
			glm::vec3 right = glm::cross(lastBurst.dir, glm::vec3(0, 1, 0));
			right = glm::length(right) < 0.001f ? glm::vec3(1, 0, 0) : glm::normalize(right);
			lastBurst.dir = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(1.0f), right)) * lastBurst.dir;
		}
		if (keySemicolon && !keySemicolonLast) {
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

	// Loop back tracing algorithm checks recursively
	firedRays.clear();
	for (const Burst& b : activeBursts) {
		simulateBurst(b.origin, b.dir);
	}

	char title[128];
	sprintf_s(title, sizeof(title), "OpenGL Optics Room - Base n=%.3f", prismIndexOfRefraction);
	glfwSetWindowTitle(window, title);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 P = glm::perspective(glm::radians(70.0f), (float)windowWidth / (float)windowHeight, 0.1f, 120.0f);
	glm::mat4 V = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	const float roomHalf = 10.0f;
	glm::mat4 model;

	// Environment Room Box Maps
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
	
	// Table Assembly Structures
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -3.2f, 0.0f));
	model = glm::scale(model, glm::vec3(4.5f, 0.2f, 3.0f));
	drawCube(spLambert, P, V, model, glm::vec4(0.40f, 0.24f, 0.12f, 1.0f));
	for (int i = 0; i < 4; ++i) {
		float x = (i < 2) ? -4.0f : 4.0f;
		float z = (i % 2 == 0) ? -2.5f : 2.5f;
		glm::mat4 leg = glm::translate(glm::mat4(1.0f), glm::vec3(x, -4.3f, z));
		leg = glm::scale(leg, glm::vec3(0.3f, 1.8f, 0.3f));
		drawCube(spLambert, P, V, leg, glm::vec4(0.36f, 0.18f, 0.08f, 1.0f));
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);

	// Render Physical Translucent Glass Components 
	glm::mat4 prismModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.3f, 0.0f));
	drawPrism(P, V, prismModel, glm::vec4(0.55f, 0.85f, 1.0f, 0.22f));
	drawSphere(P, V, glm::vec3(1.6f, -1.9f, 0.4f), 0.45f, glm::vec4(0.9f, 0.7f, 1.0f, 0.25f));
	drawCylinder(P, V, cylinderModel, glm::vec4(0.6f, 1.0f, 0.8f, 0.25f));
	
	// Render Adjustable Converging Lens Tool Object
	drawLensObject(P, V, lensModel, glm::vec4(0.7f, 0.95f, 0.95f, 0.3f));
	
	for (const auto& m : mirrors) {
		drawMirrorObject(P, V, m);
	}

	drawBeamLines(P, V);

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	// Render ImGui Buffer context out to OpenGL Canvas context structures
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void initOpenGLProgram(GLFWwindow* window) {
	initShaders();
	setupSceneObjects(); 
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.12f, 0.14f, 0.18f, 1.0f);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	
	// Start with cursor disabled for free mouse look control
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	lastFrameTime = (float)glfwGetTime();

	// Initialize ImGui Systems Frame loops hooks context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");
}

void freeOpenGLProgram(GLFWwindow* window) {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	freeShaders();
}

int main(void) {
	GLFWwindow* window;
	glfwSetErrorCallback(error_callback);
	if (!glfwInit()) {
		fprintf(stderr, "Nie mozna zainicjowac GLFW.\n");
		exit(EXIT_FAILURE);
	}
	window = glfwCreateWindow(windowWidth, windowHeight, "OpenGL Optics Laboratory", NULL, NULL);
	if (!window) {
		fprintf(stderr, "Nie mozna utworzyc okna.\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Nie mozna zainicjowac GLEW.\n");
		exit(EXIT_FAILURE);
	}
	initOpenGLProgram(window);
	while (!glfwWindowShouldClose(window)) {
		drawScene(window);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	freeOpenGLProgram(window);
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}