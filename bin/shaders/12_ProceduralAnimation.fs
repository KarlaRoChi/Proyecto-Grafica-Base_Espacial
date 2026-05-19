#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform vec4 uTint = vec4(1.0);

void main()
{
    vec4 texel = texture(texture_diffuse1, TexCoords);
    // Mezcla suave: el tinte sustituye parcialmente al color base, asi que
    // la textura del meteoro sigue legible pero adquiere claramente el
    // matiz de la linea espectral seleccionada en la barra.
    vec3 tinted = mix(texel.rgb, texel.rgb * uTint.rgb, uTint.a);
    FragColor = vec4(tinted, texel.a);
}
