#version 330

precision mediump float;

// based on wikipedia's blinn-phong implementation: https://en.wikipedia.org/wiki/Blinn%E2%80%93Phong_reflection_model

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragPosition;
in vec3 fragNormal;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec3 viewPos; // camera position
uniform vec3 sunDir;
uniform vec3 ambient;

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables

const vec3 lightColor = vec3(1.0, 1.0, 1.0);
const vec3 specColor = vec3(1.0, 1.0, 1.0);
const float shininess = 22.0;
const float screenGamma = 2.2;


float when_gt(float x, float y) {
  return max(sign(x - y), 0.0);
}

void main()
{
    // Texel color fetching from texture sampler
    vec4 texelColor = texture(texture0, fragTexCoord);
    vec3 lightDot = vec3(0.0);
    vec3 normal = normalize(fragNormal);
    vec3 viewD = normalize(viewPos - fragPosition);
    vec3 specular = vec3(0.0);

    vec3 light = vec3(0.0);


    light = -normalize(sunDir);

    float NdotL = max(dot(normal, light), 0.0);
    lightDot += lightColor*NdotL;

    float specCo = 0.0;
    specCo = pow(max(0.0, dot(viewD, reflect(-(light), normal))), shininess) * when_gt(NdotL, 0.0);
    specular += specCo;

    finalColor = (texelColor*((colDiffuse + vec4(specular, 1.0))*vec4(lightDot, 1.0)));
    finalColor += texelColor*vec4(ambient/10.0, 1.0)*colDiffuse;

    // Gamma correction
    finalColor = pow(finalColor, vec4(1.0/2.2));

}

