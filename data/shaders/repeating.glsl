#version 330

uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Raylib gives us these
in vec2 fragTexCoord;

out vec4 fragColor;

// We also ask for scale
uniform vec2 scale;

void main() {
    vec2 pos = vec2(fragTexCoord.x * scale.x, fragTexCoord.y * scale.y);
    pos = fract(pos);
    vec4 source = texture(texture0, pos);
    fragColor = source;
    fragColor.a = source.a * colDiffuse.a;
}
