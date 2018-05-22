// Load three 16 bit indices from a byte addressed buffer.
FUNC uint3 Load3x16BitIndices(uint offsetBytes)
{
    uint3 retval;

    // ByteAdressBuffer loads must be aligned at a 4 byte boundary.
    // Since we need to read three 16 bit indices: { 0, 1, 2 } 
    // aligned at a 4 byte boundary as: { 0 1 } { 2 0 } { 1 2 } { 0 1 } ...
    // we will load 8 bytes (~ 4 indices { a b | c d }) to handle two possible index triplet layouts,
    // based on first index's offsetBytes being aligned at the 4 byte boundary or not:
    //  Aligned:     { 0 1 | 2 - }
    //  Not aligned: { - 0 | 1 2 }
    const uint dwordAlignedOffset = offsetBytes & ~3;    
    const uint2 four16BitIndices = indices.Load2(dwordAlignedOffset);
 
    // Aligned: { 0 1 | 2 - } => retrieve first three 16bit indices
    if (dwordAlignedOffset == offsetBytes)
    {
        retval.x = four16BitIndices.x & 0xffff;
        retval.y = (four16BitIndices.x >> 16) & 0xffff;
        retval.z = four16BitIndices.y & 0xffff;
    }
    else // Not aligned: { - 0 | 1 2 } => retrieve last three 16bit indices
    {
        retval.x = (four16BitIndices.x >> 16) & 0xffff;
        retval.y = four16BitIndices.y & 0xffff;
        retval.z = (four16BitIndices.y >> 16) & 0xffff;
    }

    return retval;
}

FUNC Material GetMaterial(int idx)
{
	return materials[idx];
}

FUNC float3 ReflectRay(float3 v1, float3 v2)
{
	return (v2 * ((2.f * dot(v1, v2))) - v1);
}

FUNC float3 CanvasToViewport(float2 pos)
{
	return float3(pos.x * viewport_size / canvas_size.x,
		pos.y * viewport_size / canvas_size.y,
		z_near);
}