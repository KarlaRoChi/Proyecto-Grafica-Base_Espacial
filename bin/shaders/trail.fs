#version 330 core

out vec4 FragColor;

uniform vec4 particleColor;

void main()
{
    vec2 p = gl_PointCoord - vec2(0.5);

    float d = length(p);

    // círculo más grande
    float alpha =
        smoothstep(0.55, 0.08, d);

    // núcleo suave
    float core =
        smoothstep(0.10, 0.0, d);

    // MENOS blanco
    vec3 col =
        particleColor.rgb * (1.0 + 0.35 * core);

    FragColor =
        vec4(col, alpha * particleColor.a);
}