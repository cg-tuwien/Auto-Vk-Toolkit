#include "dual_quaternion.hpp"

dual_quaternion::dual_quaternion() {
	mReal.set_values(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
	mDual.set_values(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
}

dual_quaternion::dual_quaternion(quaternion r, glm::vec3 t) {
	mReal = quaternion::normalize(r);
	mDual.set_values(glm::vec4(t, 0.0f));
	mDual = quaternion::mult(mReal, mDual);
	mDual = quaternion::mult(0.5f, mDual);
}

void dual_quaternion::set_real(quaternion r) {
	mReal = r;
}

void dual_quaternion::set_dual(quaternion d) {
	mDual = d;
}

quaternion dual_quaternion::get_real() {
	return mReal;
}

quaternion dual_quaternion::get_dual() {
	return mDual;
}

dual_quaternion dual_quaternion::add(const dual_quaternion& q1, const dual_quaternion& q2) {
	dual_quaternion ret;
	ret.mReal = quaternion::add(q1.mReal, q2.mReal);
	ret.mDual = quaternion::add(q1.mDual, q2.mDual);
	return ret;
}

dual_quaternion dual_quaternion::mult(float s, const dual_quaternion& q) {
	dual_quaternion ret;
	ret.mReal = quaternion::mult(s, q.mReal);
	ret.mDual = quaternion::mult(s, q.mDual);
	return ret;
}

dual_quaternion dual_quaternion::mult(const dual_quaternion& q1, const dual_quaternion& q2) {
	dual_quaternion ret;
	ret.mReal = quaternion::mult(q2.mReal, q1.mReal);
	ret.mDual = quaternion::add(quaternion::mult(q2.mReal, q1.mDual), quaternion::mult(q2.mDual, q1.mReal));
	return ret;
}

dual_quaternion dual_quaternion::conj(const dual_quaternion& q) {
	dual_quaternion ret;
	ret.mReal = quaternion::conj(q.mReal);
	ret.mDual = quaternion::conj(q.mDual);
	return ret;
}

float dual_quaternion::mag(const dual_quaternion& q) {
	dual_quaternion ret = mult(q, conj(q));
	return ret.mReal.get_values().w;
}

dual_quaternion dual_quaternion::normalize(dual_quaternion q) {
	float magnitude = mag(q);
	dual_quaternion ret;
	ret.mReal.set_values(q.mReal.get_values() / magnitude);
	ret.mDual.set_values(q.mDual.get_values() / magnitude);
	return ret;
}

quaternion dual_quaternion::get_rotation(dual_quaternion q) {
	return q.mReal;
}

glm::vec3 dual_quaternion::get_translation(dual_quaternion q) {
	quaternion t = quaternion::mult(quaternion::mult(2.0f, q.mDual), quaternion::conj(q.mReal));
	glm::vec3 trans = glm::vec3(t.get_values());
	return trans;
}

glm::mat4 dual_quaternion::to_matrix(dual_quaternion q) {
	auto tmp = dual_quaternion::normalize(q);
	glm::mat4 m = glm::identity<glm::mat4>();

	glm::vec4 rv = tmp.mReal.get_values();

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

	// Extract translation information
	auto tv = get_translation(tmp);
	m[3][0] = tv.x;
	m[3][1] = tv.y;
	m[3][2] = tv.z;

	return m;
}

dual_quaternion dual_quaternion::from_rot_and_trans(float theta, glm::vec3 axis, glm::vec3 t) {
	quaternion r = quaternion(theta, axis);
	return dual_quaternion(r, t);
}

dual_quaternion dual_quaternion::from_mat4(glm::mat4 m) {
	dual_quaternion ret;
	glm::mat3x4 m34 = glm::mat3x4(glm::transpose(m));
	auto q = glm::dualquat_cast(m34);
	quaternion real;
	quaternion dual;
	real.set_values(glm::vec4(q.real.x, q.real.y, q.real.z, q.real.w));
	real = quaternion::normalize(real);
	dual.set_values(glm::vec4(q.dual.x, q.dual.y, q.dual.z, q.dual.w));
	ret.set_real(real);
	ret.set_dual(dual);
	return ret;
}

dual_quaternion::dual_quaternion_struct dual_quaternion::to_struct(dual_quaternion dq) {
	dual_quaternion_struct dqs;
	dqs.r = dq.mReal.get_values();
	dqs.d = dq.mDual.get_values();
	return dqs;
}