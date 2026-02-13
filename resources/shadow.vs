#version 330
in vec3 vertexPosition;
in vec2 vertexTexcoord;
in vec3 vertexNormal;
in vec4 vertexColor;

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matLight;

out vec3 fragPosition;
out vec2 fragTexcoord;
out vec4 fragPositionLight;
out vec3 fragNormal;
out vec4 fragColor;

void main() {
    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));
    fragTexcoord = vertexTexcoord;
    fragColor = vertexColor;
    fragNormal = normalize(vec3(matModel * vec4(vertexNormal, 0.0)));
    fragPositionLight = matLight * vec4(fragPosition, 1.0);
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}