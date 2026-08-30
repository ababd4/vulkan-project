#version 460

#extension GL_GOOGLE_include_directive : require
#include "input.glsl"

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in mat3 inTBN;
layout (location = 6) in vec4 inLightSpacePosition;
layout (location = 7) in vec3 inFragPos;

layout (location = 0) out vec4 outFragColor;

// pi
const float PI = 3.14159265359;
// alpha constants
const float ALPHA_MODE_OPAQUE = 0.0;
const float ALPHA_MODE_MASK = 1.0;
const float ALPHA_MODE_BLEND = 2.0;

float CalculateShadow(vec4 lightSpacePosition, vec3 normal, vec3 directionToLight)
{
    // light cliped space into NDC
    vec3 projectedCoords = lightSpacePosition.xyz / lightSpacePosition.w;

    // convert NDC xy into texture xy
    projectedCoords.xy = projectedCoords.xy * 0.5 + 0.5;

    // using GLM_FORCE_DEPTH_ZERO_TO_ONE so i don't need to convert 0~1
    float currentDepth = projectedCoords.z;

    // outside of shadow map
    if (projectedCoords.x < 0.0 || projectedCoords.x > 1.0 ||
        projectedCoords.y < 0.0 || projectedCoords.y > 1.0 ||
        currentDepth < 0.0 || currentDepth > 1.0) {
        return 0.0;
    }

    float normalDotLight =
        max(
            dot(normal, directionToLight),
            0.0
        );

    // 傾いている面ほどBiasを増やす
    float bias =
        max(
            0.002 * (1.0 - normalDotLight),
            0.0002
        );

    // uv size that one texel of shadow map
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    float shadowSampleCount = 0.0;

    // 5x5 PCF
    for (int y = -2; y <= 2; ++y) {
        for (int x = -2; x <= 2; ++x) {
            vec2 sampleUV = projectedCoords.xy + vec2(float(x), float(y)) * texelSize;
            float closestDepth = texture(shadowMap, sampleUV).r;

            shadowSampleCount += currentDepth - bias > closestDepth ? 1.0 : 0.0;
        }
    }

    float shadowRatio = shadowSampleCount / 25.0;

    return shadowRatio * 0.7;
}

float DistributionGCX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denominator = NdotH2 * (a2 - 1.0) + 1.0;
    denominator = PI * denominator * denominator;

    return a2 / max(denominator, 0.000001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    float denominator = NdotV * (1.0 - k) + k;

    return NdotV / max(denominator, 0.000001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float geometryView = GeometrySchlickGGX(NdotV, roughness);
    float geometryLight = GeometrySchlickGGX(NdotL, roughness);

    return geometryView * geometryLight;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() 
{
    // normalize vectors
    // vec3 N = normalize(inNormal);
    // use normal map
    vec3 normalTS = texture(normalTex, inUV).xyz;
    normalTS = normalTS * 2.0 - 1.0;
    vec3 N = normalize(inTBN * normalTS);
    vec3 L = normalize(-sceneData.sunlightDirection.xyz);
    vec3 V = normalize(sceneData.cameraPos.xyz - inFragPos);
    vec3 H = normalize(L + V);

    // base color
	vec4 baseColorSample = texture(colorTex, inUV);
    vec3 baseColor = pow(baseColorSample.rgb, vec3(2.2));
    baseColor *= inColor;
    // alpha 
    float alpha = baseColorSample.a * materialData.colorFactors.a;
    float alphaCutoff = materialData.extraData.x;
    float alphaMode = materialData.extraData.y;
    if (alphaMode == ALPHA_MODE_MASK && alpha < alphaCutoff) {
        discard;
    }

	// shadow mapping
	float shadow = CalculateShadow(inLightSpacePosition, N, L);
    float visibility = 1.0 - shadow;
    //float visibility = 1.0;
    
    // Metallic and Roughness
    vec4 mrSample = texture(metalRoughTex, inUV);
    float metallic = clamp(mrSample.b * materialData.metalRoughFactors.x, 0.0, 1.0);
    //float metallic = 0.0;
    float roughness = clamp(mrSample.g * materialData.metalRoughFactors.y, 0.04, 1.0);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, baseColor, metallic);

    // calc lighting
    float NDF = DistributionGCX(N, H, roughness);
    float geometry = GeometrySmith(N, V, L, roughness);
    vec3 fresnel = FresnelSchlick(max(dot(H, V), 0.0), F0);
    vec3 numerator = NDF * geometry * fresnel;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0); 

    // specular reflection
    vec3 specular = numerator / max(denominator, 0.000001);

    // diffuse reflection
    vec3 kS = fresnel;
    vec3 kD = vec3(1.0) - kS;
    // metal has no diffuse reflection
    kD *= 1.0 - metallic;
    float NdotL = max(dot(N, L), 0.0);
    vec3 radiance = sceneData.sunlightColor.rgb * sceneData.sunlightColor.w;
    vec3 directLighting = (kD * baseColor / PI + specular) * radiance * NdotL * visibility;

    // ambient
    vec3 diffuseAmbient = sceneData.ambientColor.rgb * baseColor * (1.0 - metallic);
    vec3 specularAmbient = sceneData.ambientColor.rgb * F0;

    vec3 ambient = diffuseAmbient + specularAmbient;

    //outFragColor = texture(normalTex, inUV);
    //outFragColor = vec4(normalTS * 0.5 + 0.5, 1.0);
    //outFragColor = vec4(vec3(NdotL), 1.0);

    //outFragColor = vec4(vec3(1.0 - shadow), 1.0);

    vec3 finalColor = ambient + directLighting;
    // gamma correction
    finalColor = pow(finalColor, vec3(1.0 / 2.2));
    outFragColor = vec4(finalColor, alpha);
}

