#version 330 core

#define   INF            100000
#define   PI             3.1415926
#define   TRIANGLE_SIZE  12
#define   NODE_SIZE      4

uniform int frameCounter;
uniform int width;
uniform int height;

in vec2 screenCoord;
out vec4 FragColor;

uint seed = uint(
    uint((screenCoord.x * 0.5 + 0.5) * width)  * uint(1973) + 
    uint((screenCoord.y * 0.5 + 0.5) * height) * uint(9277) + 
    uint(frameCounter) * uint(26699)) | uint(1);

uint wang_hash(inout uint seed) {
    seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
    seed *= uint(9);
    seed = seed ^ (seed >> 4);
    seed *= uint(0x27d4eb2d);
    seed = seed ^ (seed >> 15);
    return seed;
}
 
float rand() {
    return float(wang_hash(seed)) / 4294967296.0;
}

// 半球均匀分布
vec3 SampleHemisphere()
{
    float z = rand();
    float r = max(0, sqrt(1.0 - z * z));
    float phi = 2.0 * PI * rand();
    return vec3(r * cos(phi), r * sin(phi), z);
}

// 将向量 v 投影到 N 的法向半球
vec3 toNormalHemisphere(vec3 v, vec3 N) {
    vec3 helper = vec3(1, 0, 0);
    if(abs(N.x)>0.999) helper = vec3(0, 0, 1);
    vec3 tangent = normalize(cross(N, helper));
    vec3 bitangent = normalize(cross(N, tangent));
    return v.x * tangent + v.y * bitangent + v.z * N;
}

// 球面图采样
vec2 SampleFromEquirec(vec3 pos)
{
	vec2 uv = vec2(atan(pos.z, pos.x), asin(pos.y));
	uv = uv / vec2(2 * PI, PI) + 0.5;
	return uv;
}


uniform samplerBuffer triangleTexture;
uniform samplerBuffer BVHTexture;
uniform sampler2D lastFrame;
uniform sampler2D hdrMap;
uniform int triangleNum;
uniform int NodeNum;

struct Material{
	vec3 emissive;
    vec3 baseColor;
    float subsurface;
    float metallic;
    float specular;
    float specularTint;
    float roughness;
    float anisotropic;
    float sheen;
    float sheenTint;
    float clearcoat;
    float clearcoatGloss;
    float IOR;
    float transmission;
};

struct Triangle{
	vec3 p1, p2, p3;  
    vec3 n1, n2, n3;  
    Material material;
};

struct Camera{
    vec3 bottomLeft;
    vec3 width;
    vec3 height;
    vec3 origin;
};

struct Ray{
    vec3 origin;
    vec3 dir;
};

struct HitInfo{
    bool isHit;             // 是否命中
    bool isInside;          // 是否从内部命中
    float distance;         // 与交点的距离
    vec3 hitPoint;          // 光线命中点
    vec3 normal;            // 命中点法线
    vec3 viewDir;           // 击中该点的光线的方向
    Material material;      // 命中点的表面材质
};

struct BVHNode{
    int left, right;
    int index, n;
    vec3 AA, BB;
};

BVHNode GetNode(int i)
{
    BVHNode node;
    int offset = i * NODE_SIZE;
    node.left = int(texelFetch(BVHTexture, offset + 0).x);
    node.right = int(texelFetch(BVHTexture, offset + 0).y);

    node.index = int(texelFetch(BVHTexture, offset + 1).x);
    node.n = int(texelFetch(BVHTexture, offset + 1).y);
    
    node.AA= texelFetch(BVHTexture, offset + 2).xyz;
    node.BB = texelFetch(BVHTexture, offset + 3).xyz;
    return node;
}

Triangle GetTriangle(int i)
{
    Triangle triangle;
    vec3 param1, param2, param3, param4;
    int offset = i * TRIANGLE_SIZE;

    triangle.p1 = texelFetch(triangleTexture, offset + 0).xyz;
    triangle.p2 = texelFetch(triangleTexture, offset + 1).xyz;
    triangle.p3 = texelFetch(triangleTexture, offset + 2).xyz;

    triangle.n1 = texelFetch(triangleTexture, offset + 3).xyz;
    triangle.n2 = texelFetch(triangleTexture, offset + 4).xyz;
    triangle.n3 = texelFetch(triangleTexture, offset + 5).xyz;

    triangle.material.emissive = texelFetch(triangleTexture, offset + 6).xyz;
    triangle.material.baseColor = texelFetch(triangleTexture, offset + 7).xyz;

    param1 = texelFetch(triangleTexture, offset + 8).xyz;
    param2 = texelFetch(triangleTexture, offset + 9).xyz;
    param3 = texelFetch(triangleTexture, offset + 10).xyz;
    param4 = texelFetch(triangleTexture, offset + 11).xyz;

    triangle.material.subsurface = param1.x;
    triangle.material.metallic = param1.y;
    triangle.material.specular = param1.z;

    triangle.material.specularTint = param2.x;
    triangle.material.roughness = param2.y;
    triangle.material.anisotropic = param2.z;

    triangle.material.sheen = param3.x;
    triangle.material.sheenTint = param3.y;
    triangle.material.clearcoat = param3.z;

    triangle.material.clearcoatGloss = param4.x;
    triangle.material.IOR = param4.y;
    triangle.material.transmission = param4.z;

    return triangle;
}

HitInfo hitTriangle(Ray ray, Triangle triangle)
{
    HitInfo hitInfo;
    hitInfo.isHit = false;
    hitInfo.isInside = false;
    hitInfo.distance = INF;

    vec3 p1 = triangle.p1;
    vec3 p2 = triangle.p2;
    vec3 p3 = triangle.p3;

    vec3 N = normalize(cross(p2 - p1, p3 - p2));
    if(dot(N, ray.dir) > 0.0)
    {
        N = -N;
        hitInfo.isInside = true;
    }

    float t = dot(p1 - ray.origin, N) / dot(ray.dir, N);
    
    if(t < 0.000001 || abs(dot(N, ray.dir)) < 0.000001)
        return hitInfo;
    
    vec3 hitPoint = ray.origin + t * ray.dir;
    
    float c1 = dot(cross(p2 - p1, hitPoint - p1), N);
    float c2 = dot(cross(p3 - p2, hitPoint - p2), N);
    float c3 = dot(cross(p1 - p3, hitPoint - p3), N);
    if((c1 > 0 && c2 > 0 && c3 > 0) || (c1 < 0 && c2 < 0 && c3 < 0))
    {
        hitInfo.isHit = true;
        hitInfo.distance = t;
        hitInfo.hitPoint = hitPoint;
        hitInfo.viewDir = ray.dir;
        hitInfo.material = triangle.material;

        float alpha = (-(hitPoint.x-p2.x)*(p3.y-p2.y) + (hitPoint.y-p2.y)*(p3.x-p2.x)) / (-(p1.x-p2.x-0.00005)*(p3.y-p2.y+0.00005) + (p1.y-p2.y+0.00005)*(p3.x-p2.x+0.00005));
        float beta  = (-(hitPoint.x-p3.x)*(p1.y-p3.y) + (hitPoint.y-p3.y)*(p1.x-p3.x)) / (-(p2.x-p3.x-0.00005)*(p1.y-p3.y+0.00005) + (p2.y-p3.y+0.00005)*(p1.x-p3.x+0.00005));
        float gama  = 1.0 - alpha - beta;
        vec3 Nsmooth = alpha * triangle.n1 + beta * triangle.n2 + gama * triangle.n3;
        Nsmooth = normalize(Nsmooth);
        hitInfo.normal = hitInfo.isInside ? (-Nsmooth) : Nsmooth;
    }

    return hitInfo;
}

HitInfo hitTriangleArray(Ray ray, int left, int right)
{
    HitInfo hitInfo;
    hitInfo.isHit = false;
    hitInfo.distance = INF;
    for(int i = left; i <= right; ++i)
    {
        HitInfo tempInfo = hitTriangle(ray, GetTriangle(i));
        if(tempInfo.isHit && tempInfo.distance < hitInfo.distance)
            hitInfo = tempInfo;
    }

    return hitInfo;
}

float hitAABBbox(Ray ray, vec3 AA, vec3 BB)
{
    vec3 near = (AA - ray.origin) / ray.dir;
    vec3 far = (BB - ray.origin) / ray.dir;

    vec3 mint = min(near, far);
    vec3 maxt = max(near, far);

    float t0 = max(mint.x, max(mint.y, mint.z));
    float t1 = min(maxt.x, min(maxt.y, maxt.z));

    return t0 <= t1 ? (t0 > 0.0 ? t0 : t1) : -1;
}

HitInfo hitBVH(Ray ray)
{
    HitInfo hitInfo;
    int stack[256];
    int sp = 0;

    hitInfo.isHit = false;
    hitInfo.distance = INF;

    stack[sp++] = 0;
    while(sp > 0)
    {
        int top = stack[--sp];
        BVHNode node = GetNode(top);

        if(node.n > 0)
        {
            HitInfo temp = hitTriangleArray(ray, node.index, node.index + node.n - 1);
            if(temp.isHit && temp.distance < hitInfo.distance)
                hitInfo = temp;
            continue;
        }

        float d1 = -1, d2 = -1;
        if(node.left > 0)
        {
            BVHNode leftNode = GetNode(node.left);
            d1 = hitAABBbox(ray, leftNode.AA, leftNode.BB);
        }
        if(node.right > 0)
        {
            BVHNode rightNode = GetNode(node.right);
            d2 = hitAABBbox(ray, node.AA, node.BB);
        }

        if(d1 > 0.0 && d2 > 0.0)
        {
            if(d1 < d2)
            {
                stack[sp++] = node.right;
                stack[sp++] = node.left;
            }
            else
            {
                stack[sp++] = node.left;
                stack[sp++] = node.right;
            }
        }
        else if(d1 > 0.0)
            stack[sp++] = node.left;
        else if(d2 > 0.0)
            stack[sp++] = node.right;
    }

    return hitInfo;

}

vec3 pathTracing(HitInfo hit, int maxDepth)
{
    vec3 Lo = vec3(0.0);     
    vec3 history = vec3(1.0);

    for(int i = 0; i < maxDepth; ++i)
    {
        vec3 wi = toNormalHemisphere(SampleHemisphere(), hit.normal);

        Ray newRay = Ray(hit.hitPoint, wi);
        HitInfo newHit = hitBVH(newRay);
        
        float pdf = 1.0 / 2.0 * PI;
        float cosine_o = max(0.0, dot(-hit.viewDir, hit.normal));
        float cosine_i = max(0.0, dot(newRay.dir, hit.normal));
        vec3 fr = hit.material.baseColor / PI;

        if(!newHit.isHit)
        {
            float Intensity = 6.0f;
            vec2 uv = SampleFromEquirec(normalize(newRay.dir));
            vec3 backgroundLe = Intensity * texture2D(hdrMap, uv).rgb;
            // backgroundLe = min(backgroundLe, vec3(10));
            Lo += history * backgroundLe * fr * cosine_i / pdf;
            break;
        }

        vec3 Le = newHit.material.emissive;
        Lo += history * Le * fr * cosine_i / pdf;

        hit = newHit;
        history *= (fr * cosine_i / pdf);
    }

    return Lo;
}

uniform Camera camera;
uniform mat4 cameraRotation;

void main()
{
    vec3 color = vec3(0.0, 0.0, 0.0);
    int sampleNum = 1;

    for(int i = 0; i < sampleNum; ++i)
    {
        vec2 AA = vec2((rand()-0.5)/float(width), (rand()-0.5)/float(height));
        vec2 screenCoordRand = screenCoord + AA;
        vec3 pixel = camera.bottomLeft + screenCoordRand.x * camera.width + screenCoordRand.y * camera.height;

        vec3 rayDir = vec3(cameraRotation * vec4(normalize(pixel - camera.origin), 1.0));
        Ray firstRay = Ray(camera.origin, rayDir);

        HitInfo firstHit = hitBVH(firstRay);

        if(!firstHit.isHit)
        {
            vec2 uv = SampleFromEquirec(firstRay.dir);
            color = texture2D(hdrMap, uv).rgb;
            // color = min(color, vec3(10));
        }
        else{
            vec3 Le = firstHit.material.emissive;
            vec3 Li = pathTracing(firstHit, 3);
            color = Le + Li;
        }
    }
    
    vec3 lastColor = texture2D(lastFrame, screenCoord).rgb;
    color = mix(lastColor, color, 1.0 / float(frameCounter));

    // HitInfo hitInfo = hitBVH(ray);
    // HitInfo hitInfo = hitTriangleArray(ray, 0, triangleNum);
    
    gl_FragData[0] = vec4(color, 1.0);
}