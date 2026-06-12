#version 330 core
out vec4 FragColor;

uniform vec3 objectColor;
uniform vec3 lightDir;
uniform float ambientStrength;

void main() {
    vec3 normal = vec3(0.0, 1.0, 0.0);
    float diff = max(dot(normalize(normal), normalize(-lightDir)), 0.0);
    vec3 color = objectColor * (ambientStrength + diff * 0.45);
    FragColor = vec4(color, 1.0);
}
