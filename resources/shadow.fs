#version 330
in vec3 fragPosition;
in vec2 fragTexcoord;
in vec4 fragPositionLight;
in vec3 fragNormal;
in vec4 fragColor;

uniform sampler2D texture0;
uniform sampler2D shadowMap;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec4 colDiffuse;

out vec4 finalColor;

void main() {
    vec3 shadowCoord = (fragPositionLight.xyz / fragPositionLight.w) * 0.5 + 0.5;
    float shadow = 0.0;
    if (shadowCoord.z <= 1.0) {
        float closestDepth = texture(shadowMap, shadowCoord.xy).r;
        float currentDepth = shadowCoord.z;
        float bias = 0.005; 
        shadow = (currentDepth - bias > closestDepth) ? 1.0 : 0.0;
    }
   vec4 texelColor = texture(texture0, fragTexcoord);
    vec3 sunDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(fragNormal, sunDir), 0.0);
    vec3 ambient = vec3(0.5);
    vec3 lighting = (ambient + (1.0 - shadow) * diff) * lightColor;
    finalColor = vec4(lighting, 1.0) * texelColor * fragColor * colDiffuse;
}