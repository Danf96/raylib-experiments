#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;
in vec4 fragPositionLightSpace;
in vec3 fragPosition;
in vec3 fragNormal;

// Input uniform values
uniform sampler2D texture0; //diffuse
uniform sampler2D texture1; // shadow map (baked)
uniform sampler2D texture2; // shadow map (dynamic)

uniform vec3 viewPos; // camera position
uniform vec3 sunDir;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;


float when_gt(float x, float y) {
  return max(sign(x - y), 0.0);
}

float ShadowCalculation(vec4 fragPosLightSpace)
{
    //perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to ndc
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth val from light perspective
    float closestDepth = texture(texture2, projCoords.xy).r;
    // get depth of current fragment from light perspective
    float currentDepth = projCoords.z;
    // check if frag in shadow
    float shadow = currentDepth > closestDepth  ? 1.0 : 0.0; 
    return shadow;
}

void main()
{
    // Texel color fetching from texture sampler
    vec3 texelColor = texture(texture0, fragTexCoord).rgb;
    vec3 shadowColor = texture(texture1, fragTexCoord).rgb;
    shadowColor = vec3(1.0);
    

    //const float shadow = 1.0;
    vec3 normal = normalize(fragNormal);
    vec3 lightColor = vec3(0.9);
    // ambient
    vec3 ambient = 0.5 * lightColor;
    // diffuse
    vec3 lightDir = normalize(sunDir);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    // specular
    vec3 viewDir = normalize(viewPos - fragPosition);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.1;
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor;    
    // calculate shadow
    float shadow = ShadowCalculation(fragPositionLightSpace);                     
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * texelColor * shadowColor;
    
    finalColor = vec4(lighting, 1.0);
}

