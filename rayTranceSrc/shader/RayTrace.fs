#version 330 core 

#define MAXDISTANCE 100000
#define PI 3.1415926535

#define LAMBERTIAN 0
#define METALLIC 1
#define DIELECTRIC 2

// 随机数生成
uint m_u = uint(521288629);
uint m_v = uint(362436069);

uint GetUintCore(inout uint u, inout uint v){
	v = uint(36969) * (v & uint(65535)) + (v >> 16);
	u = uint(18000) * (u & uint(65535)) + (u >> 16);
	return (v << 16) + u;
}

float GetUniformCore(inout uint u, inout uint v){
	uint z = GetUintCore(u, v);
	
	return float(z) / uint(4294967295);
}

float GetUniform(){
	return GetUniformCore(m_u, m_v);
}

uint GetUint(){
	return GetUintCore(m_u, m_v);
}

float rand(){
	return GetUniform();
}

vec2 rand2(){
	return vec2(rand(), rand());
}

vec3 rand3(){
	return vec3(rand(), rand(), rand());
}

vec4 rand4(){
	return vec4(rand(), rand(), rand(), rand());
}

vec3 randomCirclePoint()
{
	float phi = rand() * PI;
	float theta = rand() * 2 * PI;
	float x = sin(phi) * cos(theta);
	float z = sin(phi) * sin(theta);
	float y = cos(phi);
	return vec3(x, y, z);
}

vec3 reflect(in vec3 v, in vec3 n){
	return v - 2 * dot(n, v) * n;
}



struct Camera
{
	vec3 lowerLeftCore;
	vec3 width;
	vec3 height;
	vec3 origin;
};

struct Material
{
	vec3 albedo;
	float fuzz;
};

in vec2 screenCoord;    // 映射系数[-1, 1] -> [-2, 2]
out vec4 FragColor;

uniform Camera camera;
uniform samplerCube environmentMap;


struct HitInfo
{
	float t;
	bool isHit;
	vec3 hitPos;
	Material material;
	vec3 hitNormal;
	int hitMaterial;
};

struct Ray
{
	vec3 dir;
	vec3 origin;
	vec3 color;
	HitInfo hitInfo;
};

struct Sphere
{
	vec3 origin;
	float radius;
	int type;
	Material material;
};

struct World
{
	int objectCount;
	Sphere objectSet[5];
};

vec3 Getlocation(Ray ray, float t)
{
	return ray.origin + ray.dir * t;
}

float interaction(Sphere sphere, Ray ray)
{
	vec3 ray2sphere = ray.origin - sphere.origin;

	float a = dot(ray.dir, ray.dir);
	float b = 2 * dot(ray2sphere, ray.dir);
	float c = dot(ray2sphere, ray2sphere) - sphere.radius * sphere.radius;
	float delta = b * b - 4 * a * c;
	 
	if(delta < 0)
		return -1;
	else
	{
		float temp1 = (-b - sqrt(delta)) / (2.0 * a);
		float temp2 = (-b + sqrt(delta)) / (2.0 * a);
		if(temp1 < 10000 && temp1 > 0.0001)
			return temp1;
		if(temp2 < 10000 && temp2 > 0.0001)
			return temp2;
		return -1;
	}
} 

World CreateWorld()
{
	World world;
	world.objectSet[0] = Sphere(vec3(0, 0, -1.0), 0.5, LAMBERTIAN, Material(vec3(0.1, 0.7, 0.7), 1.0));
	world.objectSet[1] = Sphere(vec3(1.0, 0.0, -1.0), 0.5, METALLIC, Material(vec3(0.8, 0.8, 0.8), 0.0));

	world.objectCount = 2;
	return world;
}

Ray CreateRay(vec3 origin, vec3 dir)
{
	Ray ray;
	ray.origin = origin;
	ray.dir = dir;
	return ray;
}

vec3 GetBackgroundColor(Ray ray)
{
	// vec3 normalizeDir = normalize(ray.dir);
    // float k = (normalizeDir.y + 1.0) * 0.5;
    // return (1.0 - k) * vec3(1.0, 1.0, 1.0) + k * vec3(0.5, 0.7, 1.0);
	vec3 backgroundColor = texture(environmentMap, normalize(ray.dir)).rgb;
	return backgroundColor;
}

vec3 RayTrace(Ray ray, World world, float depth)
{
	ray.color = vec3(1.0, 1.0, 1.0);
	while(depth > 0)
	{
		depth--;
		ray.hitInfo.t = MAXDISTANCE;
		ray.hitInfo.isHit = false;
		for(int i = 0; i < world.objectCount; ++i)
		{
			Sphere sphere = world.objectSet[i];
			float t = interaction(sphere, ray);
			if(t != -1)
			{
				if(!ray.hitInfo.isHit)
					ray.hitInfo.isHit = true;
				
				if(t < ray.hitInfo.t)
				{
					ray.hitInfo.t = t;
					ray.hitInfo.hitPos = Getlocation(ray, t);
					ray.hitInfo.material = sphere.material;
					ray.hitInfo.hitNormal = (ray.hitInfo.hitPos - sphere.origin) / sphere.radius;
					ray.hitInfo.hitMaterial = sphere.type;
				}
			}
		}

		if(!ray.hitInfo.isHit)
		{
			return ray.color * GetBackgroundColor(ray);
		}
		else
		{
			if(ray.hitInfo.hitMaterial == LAMBERTIAN)
				ray.dir = ray.hitInfo.hitNormal + randomCirclePoint();
			if(ray.hitInfo.hitMaterial == METALLIC)
				ray.dir = reflect(ray.dir, ray.hitInfo.hitNormal) + ray.hitInfo.material.fuzz * randomCirclePoint();
			ray.origin = ray.hitInfo.hitPos;
			ray.color = ray.color * ray.hitInfo.material.albedo;
		}
	}

	return ray.color;
}

void main()
{
	int sampleCount = 200;
	World world = CreateWorld();
	vec3 totalColor = vec3(0.0, 0.0, 0.0);

	for(int i = 0; i < sampleCount; ++i)
	{
		vec2 screenCoordRand = screenCoord + rand2() / vec2(1280, 640);
		Ray ray = CreateRay(camera.origin, camera.lowerLeftCore + screenCoordRand.x * camera.width + screenCoordRand.y * camera.height - camera.origin);
		totalColor += RayTrace(ray, world, 40);
	}
	totalColor = totalColor / sampleCount;
	totalColor = pow(totalColor, vec3(1.0 / 2.2));

	FragColor = vec4(totalColor, 1.0);
}