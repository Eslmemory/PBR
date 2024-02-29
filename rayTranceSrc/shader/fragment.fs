#version 330 core

in vec3 WorldPos;
in vec2 TexCoords;
in vec3 Normal;
out vec4 FragColor;

uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;
uniform vec3 lightPosition[1];
uniform vec3 lightColor[1];
uniform vec3 camPos;
uniform float roughness;
uniform float metallic;

const float PI = 3.1415926;

vec3 GetNormalFromMap()
{
	vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;
	
	vec3 Q1 = dFdx(WorldPos);
	vec3 Q2 = dFdy(WorldPos); 
	vec2 st1 = dFdx(TexCoords);
	vec2 st2 = dFdy(TexCoords);

	vec3 N = normalize(Normal);
	vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);
	
	return normalize(TBN * tangentNormal);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a2 = roughness * roughness;
	float NdotH = max(dot(N, H), 0.0); 
	float term = NdotH * NdotH * (a2 - 1.0) + 1.0;
	return a2 / (PI * (term * term));
}

float GeometrySchlickGGX(float NdotX, float roughness)
{
	float k = (1.0 + roughness) * (1.0 + roughness) / 8.0;
	return NdotX / (NdotX * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx1 = GeometrySchlickGGX(NdotV, roughness);
	float ggx2 = GeometrySchlickGGX(NdotL, roughness);
	return ggx1 * ggx2;
}

vec3 fresneSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1 - F0) * pow(clamp(1 - cosTheta, 0.0, 1.0), 5.0);
}


void main()
{
	// vec3 albedo = pow(texture(albedoMap, TexCoords).rgb, vec3(2.2));
	// float metallic = texture(metallicMap, TexCoords).r;
	// float roughness = texture(roughnessMap, TexCoords).r;
	// float ao = texture(aoMap, TexCoords).r;
	// vec3 N = GetNormalFromMap();
	// vec3 V = normalize(camPos - WorldPos);

	vec3 albedo = vec3(0.5, 0.0, 0.0);
	// float metallic = 0.0;
	// float roughness = 0.3;
	float ao = 1.0;
	vec3 N = normalize(Normal);
	vec3 V = normalize(camPos - WorldPos);
	
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);
	vec3 finalColor = vec3(0.0);
	
	for(int i = 0; i < 1; ++i)
	{
		float distance = length(WorldPos - lightPosition[i]);
		vec3 radiance = lightColor[i] / (distance * distance);

		vec3 L = normalize(lightPosition[i] - WorldPos);
		vec3 H = normalize(L + V);
		float D = DistributionGGX(N, H, roughness);
		float G = GeometrySmith(N, V, L, roughness);
		vec3 F = fresneSchlick(max(dot(H, V), 0.0), F0);

		float denominator = 4.0 * max(dot(V, N), 0.0) * max(dot(L, N), 0.0) + 0.0001;
		vec3 specular = (D * G * F) / denominator;

		vec3 kd = vec3(1.0)- F;
		kd = kd * (1.0 - metallic);

		finalColor += (kd * albedo / PI + specular) * radiance * max(dot(N, L), 0.0);
	}

	vec3 ambient = vec3(0.015) * albedo * ao;
	vec3 color = ambient + finalColor;

	// HDR tonemapping
	color = color / (color + vec3(1.0));

	// gamma correct
	color = pow(color, vec3(1.0/2.2));

	FragColor = vec4(color, 1.0f);
}