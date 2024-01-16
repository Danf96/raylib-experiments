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

uniform vec3 sunDir;
uniform vec3 ambient;
uniform vec3 viewPos;

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables

const vec3 lightColor = vec3(1.0, 1.0, 1.0);
const vec3 specColor = vec3(1.0, 1.0, 1.0);
const float shininess = 32.0;
const float screenGamma = 2.2;

float when_gt(float x, float y) {
  return max(sign(x - y), 0.0);
}

void main()
{
    // Texel color fetching from texture sampler
    vec4 texelColor = texture(texture0, fragTexCoord);

    vec3 normal = normalize(fragNormal);
    float lambertian = max(dot(sunDir, normal), 0.0);
    float specular = 0.0;

    vec3 viewDir = normalize(-fragPosition);
    vec3 halfDir = normalize(sunDir + viewDir);
    float specAngle = max(dot(halfDir, normal), 0.0);
    specular = pow(specAngle, shininess) * when_gt(lambertian, 0.0);

    vec3 colorLinear = texelColor.xyz * ambient + 
                       texelColor.xyz * colDiffuse.xyz * lambertian * lightColor +
                       specColor * specular * lightColor;
    vec3 colorGammaCorrected = pow(colorLinear, vec3(1.0 / screenGamma));

    gl_FragColor = vec4(colorGammaCorrected, 1.0);

}

