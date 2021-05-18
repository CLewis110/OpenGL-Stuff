#version 430 core

layout(location=0) out vec4 out_color;
 
in vec4 vertexColor; // Now interpolated across face
in vec4 interPos;
in vec3 interNormal;
in vec2 interUV;

struct PointLight
{
    vec4 pos;
    vec4 color;
};

uniform PointLight light;
uniform sampler2D diffuseTexture;


void main()
{
    vec3 N = normalize(interNormal);

    //Calculate the light vector L as the NORMALIZED direction vector FROM interPos TO light.pos
    vec3 UL = vec3(light.pos - interPos);
    vec3 L = normalize(UL);

    vec4 texColor = texture(diffuseTexture, interUV);

    //Calculate the diffuse coefficient (float) as the max of 0 and the dot product of N and L
    float diffuse = max(0, dot(N, L));

    // Multiply the diffuse coefficient by the vertexColor and convert to vec3 → vec3 diffColor
    vec3 diffColor = diffuse * vec3(vertexColor);

    diffColor = diffColor * texColor.xyz;

    //Set shininess s to 100
    float s = 100;

    //Get view vector
    vec3 V = normalize(vec3(-interPos));

    //Get the half vector
    vec3 H = normalize(V + L);

    //Compute specular comp
    float spec = max(0, pow(dot(N, H), s));

    //Multiply spec comp by diffuse comp
    spec = spec * diffuse;

    //Spec color
    vec3 specColor = vec3(spec, spec, spec);

    //Set out_color equals to vec4(diffColor, 1.0)
    out_color = vec4(diffColor + specColor, 1.0);
}
