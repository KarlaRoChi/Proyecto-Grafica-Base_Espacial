#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

// Variables de cámara y texturas
uniform vec3 eye; 
uniform sampler2D texture_diffuse1;

// Propiedades del material
uniform vec4 MaterialAmbientColor;
uniform vec4 MaterialDiffuseColor;
uniform vec4 MaterialSpecularColor;
uniform int alphaIndex; 
uniform float transparency;

// Luz base global enviada desde C++
uniform vec4 globalAmbient; 

#define MAX_LIGHTS 20
uniform int numLights;

struct Light {
    vec3  Position;
    vec3  Direction;   
    vec4  Color;
    vec4  Power;       
    int   alphaIndex;  
    float distance;    
    int   type;        // 0: Punto, 1: Foco, 2: Direccional
};

uniform Light allLights[MAX_LIGHTS];

vec4 ApplyLight(Light light, vec3 N, vec3 V) {
    vec3 L;
    float attenuation = 1.0;

    // --- 1. DIRECCIÓN Y ATENUACIÓN SEGÚN TIPO ---
    if (light.type == 2) {
        // LUZ DIRECCIONAL
        L = normalize(-light.Direction);
        attenuation = light.Power.x * 0.01; // Ajuste de escala para luz direccional
    } 
    else {
        // LUZ POSICIONAL (PUNTO O FOCO)
        L = normalize(light.Position - FragPos);
        float d = length(light.Position - FragPos);
        
        // Atenuación controlada que usa tanto el Power como el Distance de tu C++
        // Evita divisiones por 0 y atenúa suavemente.
        float distFactor = max(light.distance, 1.0);
        attenuation = light.Power.x / (1.0 + (d * d) / (distFactor * 0.5));
    }

    // --- 2. MODELO BLINN-PHONG ---
    // Componente Ambiental local de la luz
    vec4 K_a = MaterialAmbientColor * light.Color * 0.05;

    // Componente Difusa
    float cosTheta = max(dot(N, L), 0.0);
    vec4 K_d = MaterialDiffuseColor * light.Color * cosTheta;

    // Componente Especular (Usando Half-Vector de Blinn-Phong para evitar defectos visuales)
    vec3 H = normalize(L + V);
    float cosAlpha = max(dot(N, H), 0.0);
    vec4 K_s = MaterialSpecularColor * light.Color * pow(cosAlpha, max(float(alphaIndex), 1.0));

    return (K_a + K_d + K_s) * attenuation;
}

void main() {    
    vec3 N = normalize(Normal);
    vec3 V = normalize(eye - FragPos);

    vec4 totalLighting = globalAmbient * MaterialAmbientColor * 0.3;
    for(int i = 0; i < numLights; ++i) {
        totalLighting += ApplyLight(allLights[i], N, V);
    }
           
    vec4 texel = texture(texture_diffuse1, TexCoords);

    // 1) Linearizamos la textura antes de mezclarla con la luz
    vec3 albedo = pow(texel.rgb, vec3(2.2));

    vec3 hdrColor = albedo * totalLighting.rgb;

    // 2) Tone mapping ACES filmic: conserva saturación mucho mejor que Reinhard
    vec3 a = hdrColor * (2.51 * hdrColor + 0.03);
    vec3 b = hdrColor * (2.43 * hdrColor + 0.59) + 0.14;
    vec3 mapped = clamp(a / b, 0.0, 1.0);

    // 3) Corrección gamma final (ahora sí es correcta porque venimos de espacio lineal)
    mapped = pow(mapped, vec3(1.0/2.2));

    FragColor = vec4(mapped, transparency);
}