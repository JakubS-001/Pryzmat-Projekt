#version 330

uniform vec4 color;
uniform vec4 lightPos;      // Pozycja światła w przestrzeni widoku
uniform sampler2D shadowMap; // Tekstura z mapą głębokości (cieni)

in vec3 f_normal;
in vec3 f_viewPos;
in vec4 f_lightSpacePos;

out vec4 pixelColor;

// Funkcja sprawdzająca, czy dany piksel jest zasłonięty (w cieniu)
float CalculateShadow(vec4 lightSpacePos, vec3 normal, vec3 lightDir) {
    // Korekcja perspektywiczna
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    // Mapowanie współrzędnych z [-1, 1] na zakres [0, 1] dla tekstury
    projCoords = projCoords * 0.5 + 0.5;
    
    if(projCoords.z > 1.0) return 0.0;
    
    // Odczytanie najbliższej odległości ze shadowmapy
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // Aktualna odległość tego piksela od światła
    float currentDepth = projCoords.z;
    
    // Bias zapobiega efektowi "shadow acne" (paskom na płaskich powierzchniach)
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    
    // Test cienia: 1.0 oznacza pełny cień, 0.0 brak cienia
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    return shadow;
}

void main(void) {
    vec3 n = normalize(f_normal);
    vec3 l = normalize(lightPos.xyz - f_viewPos);

    // Światło otoczenia (ambient)
    float ambient = 0.35;

    // Światło rozproszone (diffuse)
    float nl = clamp(dot(n, l), 0.0, 1.0);
    
    // Obliczenie cienia
    float shadow = CalculateShadow(f_lightSpacePos, n, l);
    
    // Jeśli piksel jest w cieniu (shadow = 1), diffuse zostaje wyzerowane
    vec3 diffuse = nl * vec3(0.65) * (1.0 - shadow);

    pixelColor = vec4(color.rgb * (diffuse + ambient), color.a);
}