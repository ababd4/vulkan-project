#version 460

#extension GL_GOOGLE_include_directive : require
#include "input.glsl"

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec4 inLightSpacePosition;
layout (location = 4) in vec3 inFragPos;

layout (location = 0) out vec4 outFragColor;

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

    // 3x3 PCF
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

void main() 
{
    // normalize vectors
    vec3 N = normalize(inNormal);
    vec3 L = normalize(-sceneData.sunlightDirection.xyz);

	vec3 color = inColor * texture(colorTex,inUV).xyz;
	vec3 ambient = color *  sceneData.ambientColor.xyz;

    // specular reflection
    vec3 viewDir = normalize(sceneData.cameraPos.xyz - inFragPos);
    vec3 reflectDir = reflect(-L, N);
    float specular = pow(max(dot(viewDir, reflectDir), 0.0), 16);
    float specularStrength = 0.5;

	// shadow mapping
	float shadow = CalculateShadow(inLightSpacePosition, N, L);
    float visibility = 1.0 - shadow;
    // diffuse lighting
    float NdotL = max(dot(N, L), 0.0);

    //outFragColor = vec4(vec3(lighting * visibility), 1.0);
    //outFragColor = vec4(N * 0.5 + 0.5, 1.0);
    //outFragColor = vec4(vec3(NdotL), 1.0);
    //outFragColor = vec4(vec3((d + 1.0) * 0.5), 1.0);

    //outFragColor = vec4(color * lightValue *  sceneData.sunlightColor.w + ambient ,1.0f);
	//outFragColor = vec4(vec3(texture(shadowMap, inUV).r), 1.0);
    //outFragColor = vec4(vec3(1.0 - shadow), 1.0);

    vec3 finalColor = (color * NdotL * visibility) + ambient + (specular * specularStrength * visibility);
    // gamma correction
    finalColor = pow(finalColor, vec3(1.0 / 2.2));
    outFragColor = vec4(finalColor, 1.0);
}

