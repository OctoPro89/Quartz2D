#version 330 core
in vec4 vColor;
in vec2 vTexCoord;
flat in int vTexIndex;

uniform sampler2D u_Textures[8];

out vec4 FragColor;

void main() {
    if (vTexIndex < 0)
        FragColor = vColor; // Use pure color
    else
        FragColor = texture(u_Textures[vTexIndex], vTexCoord) * vColor;
}