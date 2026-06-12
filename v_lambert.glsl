#version 330

// Zmienne jednorodne
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform mat4 lightSpaceMatrix; // Macierz widoku i projekcji z perspektywy światła

// Atrybuty
layout (location=0) in vec4 vertex; 
layout (location=1) in vec4 normal; 

// Przekazujemy dane do fragment shadera do obliczeń per-piksel
out vec3 f_normal;
out vec3 f_viewPos;
out vec4 f_lightSpacePos;

void main(void) {
    gl_Position = P * V * M * vertex;

    // Pozycja wierzchołka w przestrzeni widoku kamery
    f_viewPos = vec3(V * M * vertex);
    
    // Transformacja normalnych do przestrzeni widoku
    mat3 G = inverse(transpose(mat3(V * M)));
    f_normal = G * normal.xyz;
    
    // Pozycja wierzchołka z perspektywy źródła światła (do mapy cieni)
    f_lightSpacePos = lightSpaceMatrix * M * vertex;
}