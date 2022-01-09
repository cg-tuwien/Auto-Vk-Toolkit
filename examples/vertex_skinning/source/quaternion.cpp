#include "quaternion.hpp"

quaternion::quaternion() {
	mValues = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
}

quaternion::quaternion(float theta, glm::vec3 axis) {
	glm::vec4 norm_axis = glm::vec4(glm::normalize(axis), 1.0f);
	mValues = glm::vec4(glm::sin(theta / 2.0f), glm::sin(theta / 2.0f), glm::sin(theta / 2.0f), glm::cos(theta / 2.0f)) * norm_axis;
}

void quaternion::set_values(glm::vec4 values) {
	mValues = values;
}

glm::vec4 quaternion::get_values() {
	return mValues;
}

quaternion quaternion::add(const quaternion& q1, const quaternion& q2) {
	quaternion ret;
	float w1 = q1.mValues.w;
	float w2 = q2.mValues.w;
	glm::vec3 v1 = glm::vec3(q1.mValues);
	glm::vec3 v2 = glm::vec3(q2.mValues);
	ret.mValues = glm::vec4(v1 + v2, w1 + w2);
	return ret;
}

quaternion quaternion::mult(float s, const quaternion& q) {
	quaternion ret;
	ret.mValues = s * q.mValues;
	return ret;
}

quaternion quaternion::mult(const quaternion& q1, const quaternion& q2) {
	quaternion ret;
	float w1 = q1.mValues.w;
	float w2 = q2.mValues.w;
	glm::vec3 v1 = glm::vec3(q1.mValues);
	glm::vec3 v2 = glm::vec3(q2.mValues);
	ret.mValues = glm::vec4(
		w1 * v2 + w2 * v1 + glm::cross(v1, v2),
		w1 * w2 - glm::dot(v1, v2)
	);
	return ret;
}

quaternion quaternion::conj(const quaternion& q) {
	quaternion ret;
	ret.mValues = glm::vec4(-glm::vec3(q.mValues), q.mValues.w);
	return ret;
}

float quaternion::mag(const quaternion& q) {
	return mult(q, conj(q)).mValues.w;
}

quaternion quaternion::normalize(const quaternion& q) {
	quaternion ret;
	float magnitude = mag(q);
	ret.mValues = q.mValues / magnitude;
	return ret;
}

glm::mat4 quaternion::to_matrix(quaternion q) {
	auto tmp = quaternion::normalize(q);
	glm::mat4 m = glm::identity<glm::mat4>();
	glm::vec4 rv = tmp.mValues;

	// Extract rotational information
	m[0][0] = rv.w * rv.w + rv.x * rv.x - rv.y * rv.y - rv.z * rv.z;
	m[0][1] = 2.0f * rv.x * rv.y + 2.0f * rv.w * rv.z;
	m[0][2] = 2.0f * rv.x * rv.z - 2.0f * rv.w * rv.y;
	m[1][0] = 2.0f * rv.x * rv.y - 2.0f * rv.w * rv.z;
	m[1][1] = rv.w * rv.w + rv.y * rv.y - rv.x * rv.x - rv.z * rv.z;
	m[1][2] = 2.0f * rv.y * rv.z + 2.0f * rv.w * rv.x;
	m[2][0] = 2.0f * rv.x * rv.z + 2.0f * rv.w * rv.y;
	m[2][1] = 2.0f * rv.y * rv.z - 2.0f * rv.w * rv.x;
	m[2][2] = rv.w * rv.w + rv.z * rv.z - rv.x * rv.x - rv.y * rv.y;

	return m;
}

quaternion::quaternion_struct quaternion::to_struct(quaternion q) {
	quaternion_struct qs;
	qs.q = q.mValues;
	return qs;
}