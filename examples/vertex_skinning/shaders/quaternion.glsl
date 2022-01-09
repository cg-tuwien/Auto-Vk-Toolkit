// #version 460
#ifndef quaternion_glsl
#define quaternion_glsl

struct Quaternion {
    vec4 q;
};

Quaternion quat() {
    return Quaternion(vec4(0.0, 0.0, 0.0, 1.0));
}

Quaternion quat(vec4 v) {
    return Quaternion(v);
}

Quaternion quat(float theta, vec3 axis) {
    vec4 norm_axis = vec4(normalize(axis), 1.0);
    vec4 q = vec4(sin(theta/2.0), sin(theta/2.0), sin(theta/2.0), cos(theta/2.0)) * norm_axis;
    return Quaternion(q);
}

Quaternion quat_add(Quaternion q1, Quaternion q2) {
    float w1 = q1.q.w;
    float w2 = q2.q.w;
    vec3 v1 = q1.q.xyz;
    vec3 v2 = q2.q.xyz;
    vec4 q = vec4(v1+v2, w1+w2);
    return Quaternion(q);
}

Quaternion quat_mult(float s, Quaternion q) {
    vec4 vals = s * q.q;
    return Quaternion(vals);
}

Quaternion quat_mult(Quaternion q1, Quaternion q2) {
    float w1 = q1.q.w;
    float w2 = q2.q.w;
    vec3 v1 = q1.q.xyz;
    vec3 v2 = q2.q.xyz;
    vec4 vals = vec4(
    w1*v2 + w2*v1 + cross(v1, v2),
    w1*w2 - dot(v1, v2)
    );
    return Quaternion(vals);
}

Quaternion quat_mult(vec4 q1, vec4 q2) {
    float w1 = q1.w;
    float w2 = q2.w;
    vec3 v1 = q1.xyz;
    vec3 v2 = q2.xyz;
    vec4 vals = vec4(
    w1*v2 + w2*v1 + cross(v1, v2),
    w1*w2 - dot(v1, v2)
    );
    return Quaternion(vals);
}

Quaternion quat_conj(Quaternion q) {
    vec4 vals = vec4(-q.q.xyz, q.q.w);
    return Quaternion(vals);
}

float quat_mag(Quaternion q) {
    return quat_mult(q, quat_conj(q)).q.w;
}

Quaternion quat_normalize(Quaternion q) {
    float mag = quat_mag(q);
    vec4 vals = q.q / mag;
    return Quaternion(vals);
}

vec4 quat_add_oriented(vec4 q1, vec4 q2)
{
    if (dot(q1, q2) >= 0) {
        return q1 + q2;
    } else {
        return q1 - q2;
    }
}

mat4 quat_to_matrix(Quaternion q) {
    Quaternion tmp = quat_normalize(q);
    mat4 m  = mat4(1.0);
    vec4 rv = tmp.q;

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

    return m;
}

#endif