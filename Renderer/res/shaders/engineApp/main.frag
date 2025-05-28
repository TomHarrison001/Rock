#version 460

layout(location = 0) out vec4 colour;

layout(location = 0) in vec3 fragmentPos;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 v_texCoord;
layout(location = 3) in vec3 vertexColour;

struct directionalLight
{
    vec3 colour;
    vec3 direction;
};

struct pointLight
{
    vec3 colour;
    vec3 position;
    vec3 constants;
};

struct spotLight
{
    vec3 colour;
    vec3 position;
    vec3 direction;
    vec3 constants;
    float innerCutOff;
    float outerCutOff;
};

const int numPointLights = 1;

layout(binding = 1) uniform sampler2D u_texture;

layout(binding = 2) uniform LightUBO {
    directionalLight dLight;
    pointLight pLights[numPointLights];
} u_light;

layout(binding = 3) uniform ViewUBO {
    vec3 viewPos;
} u_view;

vec3 getDirectionalLight();
vec3 FresnelSchlick(float cosTheta, vec3 F0);
float DistributionGGX();
float GeometrySmith();
float GeometrySchlickGGX(float Ndot);
vec3 getPointLight(int index);
vec3 getSpotLight(int index);

// global vars
const float PI = 3.14159265359f;
vec3 posInWS;
vec3 albedo;
float metallic = 1.f;
float roughness = 1.f;
vec3 N;
vec3 V;
float NdotV;
float NdotL;
float NdotH;
vec3 F0;

vec3 normal;
float specularStrength = 0.8f;
vec3 viewDir;
vec4 fragPosClipSpace;

void main()
{
    vec2 uv = v_texCoord;
    albedo = texture(u_texture, uv).rgb;
    N = normalize(vertexNormal);
    V = normalize(u_view.viewPos - fragmentPos);

    NdotV = max(dot(N, V), 0.f);

    F0 = vec3(0.04f);
    F0 = mix(F0, albedo, metallic);

    vec3 result = vec3(0.01f, 0.01f, 0.01f);
    result += getDirectionalLight();

    for (int i = 0; i < numPointLights; i++)
    {
        result += getPointLight(i);
    }

    colour = vec4(result, 1.f);
}

vec3 getDirectionalLight()
{
    vec3 ambient = albedo;
    vec3 L = normalize(-u_light.dLight.direction);
    vec3 H = normalize(L + V);

    NdotL = max(dot(N, L), 0.f);
    NdotH = max(dot(N, H), 0.f);

    // cook torrance
    float D = DistributionGGX();
    float G = GeometrySmith();
    vec3 F = FresnelSchlick(NdotH, F0);
    vec3 numerator = D * G * F;
    float denominator = 4.f * NdotV * NdotL + 0.0000001f;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.f) - kS;
    kD *= 1.f - metallic;
    vec3 diffuse = (kD * albedo / PI) * NdotL;

    return ambient + (diffuse + specular) * u_light.dLight.colour;
}

vec3 getPointLight(int index)
{
    vec3 ambient = albedo;

    vec3 L = normalize(u_light.pLights[index].position - fragmentPos);
    vec3 H = normalize(L + V);
    float distance = length(u_light.pLights[index].position - fragmentPos);
    float attenuation = 1.f / (distance * distance);
    vec3 radiance = u_light.pLights[index].colour * attenuation;

    NdotL = max(dot(N, L), 0.f);
    NdotH = max(dot(N, H), 0.f);

    // cook torrance
    float D = DistributionGGX();
    float G = GeometrySmith();
    vec3 F = FresnelSchlick(NdotH, F0);
    vec3 numerator = D * G * F;
    float denominator = 4.f * NdotV * NdotL + 0.0000001f;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.f) - kS;
    kD *= 1.f - metallic;

    vec3 diffuse = (kD * albedo / PI) * NdotL;

    return ambient + (diffuse + specular) * u_light.pLights[index].colour;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.f - F0) * pow(clamp(1.f - cosTheta, 0.f, 1.f), 5.f);
}

float DistributionGGX()
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;

    float denominator = (NdotH2 * (a2 - 1.f) + 1.f);
    denominator = PI * denominator * denominator;

    return a2 / denominator;
}

float GeometrySmith()
{
    float ggx1 = GeometrySchlickGGX(NdotL);
    float ggx2 = GeometrySchlickGGX(NdotV);

    return ggx1 * ggx2;
}

float GeometrySchlickGGX(float Ndot)
{
    float r = roughness + 1.f;
    float k = (r * r) / 8.f;
    float denominator = Ndot * (1.f - k) + k;

    return Ndot / denominator;
}
