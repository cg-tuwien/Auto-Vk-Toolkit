
struct Plane
{
	float a, b, c, d;
};

// <0 ... pt lies on the negative halfspace
//  0 ... pt lies on the plane
// >0 ... pt lies on the positive halfspace
float ClassifyPoint(Plane plane, vec3 pt)
{
	float d;
	d = plane.a * pt.x + plane.b * pt.y + plane.c * pt.z + plane.d;
	return d;
}


vec4 bone_transform(mat4 BM0, mat4 BM1, mat4 BM2, mat4 BM3, vec4 weights, vec4 positionToTransform)
{
	weights.w = 1.0 - dot(weights.xyz, vec3(1.0, 1.0, 1.0));
	vec4 tr0 = BM0 * positionToTransform;
	vec4 tr1 = BM1 * positionToTransform;
	vec4 tr2 = BM2 * positionToTransform;
	vec4 tr3 = BM3 * positionToTransform;
	return weights[0] * tr0 + weights[1] * tr1 + weights[2] * tr2 + weights[3] * tr3;
}

vec3 bone_transform(mat4 BM0, mat4 BM1, mat4 BM2, mat4 BM3, vec4 weights, vec3 normalToTransform)
{
	weights.w = 1.0 - dot(weights.xyz, vec3(1.0, 1.0, 1.0));
	vec3 tr0 = mat3(BM0) * normalToTransform;
	vec3 tr1 = mat3(BM1) * normalToTransform;
	vec3 tr2 = mat3(BM2) * normalToTransform;
	vec3 tr3 = mat3(BM3) * normalToTransform;
	return weights[0] * tr0 + weights[1] * tr1 + weights[2] * tr2 + weights[3] * tr3;
}


struct bounding_box
{
	vec4 mMin;
	vec4 mMax;
};

vec3[8] bounding_box_corners(bounding_box bb)
{
	vec3 corners[8] = vec3[8](
		vec3(bb.mMin.x, bb.mMin.y, bb.mMin.z),
		vec3(bb.mMin.x, bb.mMin.y, bb.mMax.z),
		vec3(bb.mMin.x, bb.mMax.y, bb.mMin.z),
		vec3(bb.mMin.x, bb.mMax.y, bb.mMax.z),
		vec3(bb.mMax.x, bb.mMin.y, bb.mMin.z),
		vec3(bb.mMax.x, bb.mMin.y, bb.mMax.z),
		vec3(bb.mMax.x, bb.mMax.y, bb.mMin.z),
		vec3(bb.mMax.x, bb.mMax.y, bb.mMax.z)
	);
	return corners;
}

bounding_box transform(bounding_box bbIn, mat4 M)
{
	vec3[8] corners = bounding_box_corners(bbIn);
	// Transform all the corners:
	for (int i = 0; i < 8; ++i) {
		vec4 transformed = (M * vec4(corners[i], 1.0));
		corners[i] = transformed.xyz / transformed.w;
	}

	bounding_box bbOut;
	bbOut.mMin = vec4(corners[0], 0.0);
	bbOut.mMax = vec4(corners[0], 0.0);
	for (int i = 1; i < 8; ++i) {
		bbOut.mMin.xyz = min(bbOut.mMin.xyz, corners[i]);
		bbOut.mMax.xyz = max(bbOut.mMax.xyz, corners[i]);
	}
	return bbOut;
}
