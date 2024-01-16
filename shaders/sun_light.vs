#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

// Input uniform values
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Output vertex attributes (to fragment shader)
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;

// NOTE: Add here your custom variables

void main()
{
    // Send vertex attributes to fragment shader
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    vec4 vertPos4 = vec4(view * model * vec4(vertexPosition, 1.0));
    fragPosition = vec3(vertPos4) / vertPos4.w;
    fragNormal = normalize(mat3(transpose(inverse(view * model))) * vertexNormal);


    // Calculate final vertex position
    gl_Position = projection * view * model * vec4(vertexPosition, 1.0);
}