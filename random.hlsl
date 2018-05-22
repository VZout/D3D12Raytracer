FUNC float random( float2 p )
{
    const float2 K1 = float2(
        23.14069263277926, // e^pi (Gelfond's constant)
         2.665144142690225 // 2^sqrt(2) (Gelfondâ€“Schneider constant)
    );
    return frac( cos( dot(p,K1) ) * 12345.6789 );
}

FUNC float3 RandomSpherePoint(Sphere sphere, float u, float v){
	const float theta = 2 * PI * u;
	const float phi = acos(2 * v - 1);
	const float x = sphere.center.x + (sphere.radius * sin(phi) * cos(theta));
	const float y = sphere.center.y + (sphere.radius * sin(phi) * sin(theta));
	const float z = sphere.center.z + (sphere.radius * cos(phi));
	return float3(x, y, z);	
}