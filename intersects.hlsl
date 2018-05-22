struct Intersection
{
	Triangle closest;
	float closest_t;
};

struct SphereIntersection
{
	Sphere closest;
	float closest_t;
};

FUNC bool IntersectBVH(float3 orig, float3 dir, BVHNode node)
{
    float tmin, tmax, tymin, tymax, tzmin, tzmax; 
 
	float3 invdir = 1 / dir; 
	int sign[3];
	sign[0] = (invdir.x < 0); 
    sign[1] = (invdir.y < 0); 
    sign[2] = (invdir.z < 0); 

    tmin = (node.bbox[sign[0]].x - orig.x) * invdir.x; 
    tmax = (node.bbox[1-sign[0]].x - orig.x) * invdir.x; 
    tymin = (node.bbox[sign[1]].y - orig.y) * invdir.y; 
    tymax = (node.bbox[1-sign[1]].y - orig.y) * invdir.y; 
 
    if ((tmin > tymax) || (tymin > tmax)) 
        return false; 
    if (tymin > tmin) 
        tmin = tymin; 
    if (tymax < tmax) 
        tmax = tymax; 
 
    tzmin = (node.bbox[sign[2]].z - orig.z) * invdir.z; 
    tzmax = (node.bbox[1-sign[2]].z - orig.z) * invdir.z; 
 
    if ((tmin > tzmax) || (tzmin > tmax)) 
        return false; 
    if (tzmin > tmin) 
        tmin = tzmin; 
    if (tzmax < tmax) 
        tmax = tzmax; 
 
    return true; 
}

FUNC float2 IntersectRaySphere(float3 origin, float3 direction, Sphere sphere)
{
	float3 oc = origin - sphere.center;

	float k1 = dot(direction, direction);
	float k2 = 2 * dot(oc, direction);
	float k3 = dot(oc, oc) - sphere.radius * sphere.radius;

	float discriminant = k2 * k2 - 4.f * k1 * k3;
	if (discriminant < 0)
	{
		return float2(inf, inf);
	}

	float t1 = (-k2 + sqrt(discriminant)) / (2.f * k1);
	float t2 = (-k2 - sqrt(discriminant)) / (2.f * k1);

	return float2(t1, t2);
}


FUNC float2 IntersectRayTriangle(float3 orig, float3 dir, Triangle tri) 
{ 
   	const float3 v0v1 = tri.b - tri.a; 
    const float3 v0v2 = tri.c - tri.a; 
    const float3 pvec = cross(dir, v0v2); 
    const float det = dot(v0v1, pvec); 
 
    // ray and triangle are parallel if det is close to 0
    if (abs(det) < epsilon) return float2(inf, inf); 
 
    const float invDet = 1 / det; 
 
    const float3 tvec = orig - tri.a; 
    const float u = dot(tvec, pvec) * invDet; 
    if (u < 0 || u > 1) return float2(inf, inf); 
 
    const float3 qvec = cross(tvec, v0v1); 
    const float v = dot(dir, qvec) * invDet; 
    if (v < 0 || u + v > 1) return float2(inf, inf); 
 
    const float t = dot(v0v2, qvec) * invDet; 
 
    return (t > 0) ? float2(t, t) : float2(inf, inf); 
} 