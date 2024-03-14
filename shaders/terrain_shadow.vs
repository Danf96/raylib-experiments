#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

// Input uniform values
uniform mat4 matProjection;
uniform mat4 matView;
uniform mat4 matModel;
uniform mat4 matNormal;
uniform mat4 lightSpaceMatrix;


// Output vertex attributes (to fragment shader)
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec4 fragPositionLightSpace;
out vec3 fragNormal;

void main()
{
    // Send vertex attributes to fragment shader
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));
    fragPositionLightSpace = lightSpaceMatrix * vec4(fragPosition, 1.0);
    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 1.0)));



    // Calculate final vertex position
    gl_Position = matProjection * matView * matModel * vec4(vertexPosition, 1.0);
}