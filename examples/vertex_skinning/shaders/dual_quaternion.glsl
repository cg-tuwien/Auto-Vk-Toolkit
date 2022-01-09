// #version 460
#ifndef dual_quaternion_glsl
#define dual_quaternion_glsl
#extension GL_GOOGLE_include_directive : require
#include "quaternion.glsl"

struct DualQuaternion {
    vec4 r;
    vec4 d;
};

DualQuaternion dualquat() {
    return DualQuaternion(vec4(0.0, 0.0, 0.0, 1.0), vec4(0.0, 0.0, 0.0, 0.0));
}

DualQuaternion dualquat(Quaternion rot, vec3 trans) {
    Quaternion real = quat_normalize(rot);
    Quaternion dual = Quaternion(vec4(trans, 0.0f));
    dual = quat_mult(real, dual);
    dual = quat_mult(0.5, dual);
    return DualQuaternion(real.q, dual.q);
}

DualQuaternion dualquat_from_rot_and_trans(float theta, vec3 axis, vec3 t) {
    Quaternion rot = quat(theta, axis);
    return dualquat(rot, t);
}

DualQuaternion dualquat_add(DualQuaternion q1, DualQuaternion q2) {
    Quaternion real = quat_add(Quaternion(q1.r), Quaternion(q2.r));
    Quaternion dual = quat_add(Quaternion(q1.d), Quaternion(q2.d));
    return DualQuaternion(real.q, dual.q);
}

DualQuaternion dualquat_mult(float s, DualQuaternion q) {
    return DualQuaternion(s * q.r, s * q.d);
}

DualQuaternion dualquat_mult(DualQuaternion q1, DualQuaternion q2) {
    Quaternion real = quat_mult(q2.r, q1.r);
    Quaternion dual = quat_add(quat_mult(q2.r, q1.d), quat_mult(q2.d, q1.r));
    return DualQuaternion(real.q, dual.q);
}

DualQuaternion dualquat_conj(DualQuaternion q) {
    Quaternion real = quat_conj(Quaternion(q.r));
    Quaternion dual = quat_conj(Quaternion(q.d));
    return DualQuaternion(real.q, dual.q);
}

float dualquat_mag(DualQuaternion q) {
    DualQuaternion tmp = dualquat_mult(q, dualquat_conj(q));
    return tmp.r.w;
}

DualQuaternion dualquat_normalize(DualQuaternion q) {
    float mag = dualquat_mag(q);
    vec4 real = q.r / mag;
    vec4 dual = q.d / mag;
    return DualQuaternion(real, dual);
}

Quaternion dualquat_get_rotation(DualQuaternion q) {
    vec4 rot = q.r;
    return Quaternion(rot);
}

vec3 dualquat_get_translation(DualQuaternion q) {
    Quaternion t = quat_mult(quat_mult(2.0, Quaternion(q.d)), quat_conj(Quaternion(q.r)));
    return t.q.xyz;
}

mat4 dualquat_to_matrix(DualQuaternion q) {
    q = dualquat_normalize(q);
    mat4 m  = mat4(1.0);
    vec4 rv = q.r;

    // rotation
    m[0][0] = rv.w * rv.w + rv.x * rv.x - rv.y * rv.y - rv.z * rv.z;
    m[0][1] = 2.0  * rv.x * rv.y + 2.0  * rv.w * rv.z;
    m[0][2] = 2.0  * rv.x * rv.z - 2.0  * rv.w * rv.y;
    m[1][0] = 2.0  * rv.x * rv.y - 2.0  * rv.w * rv.z;
    m[1][1] = rv.w * rv.w + rv.y * rv.y - rv.x * rv.x - rv.z * rv.z;
    m[1][2] = 2.0  * rv.y * rv.z + 2.0  * rv.w * rv.x;
    m[2][0] = 2.0  * rv.x * rv.z + 2.0  * rv.w * rv.y;
    m[2][1] = 2.0  * rv.y * rv.z - 2.0  * rv.w * rv.x;
    m[2][2] = rv.w * rv.w + rv.z * rv.z - rv.x * rv.x - rv.y * rv.y;

    // translation
    vec3 tv = dualquat_get_translation(q);
    m[3][0] = tv.x;
    m[3][1] = tv.y;
    m[3][2] = tv.z;

    return m;
}

#endif