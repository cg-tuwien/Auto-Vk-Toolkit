// #version 460
#ifndef dual_quaternion_glsl
#define dual_quaternion_glsl
#extension GL_GOOGLE_include_directive : require
#include "quaternion.glsl"

// code from https://github.com/pmbittner/OptimisedCentresOfRotationSkinning/blob/master/shader/quaternion.glsl

// real part + dual part
// = real + e*dual
// = q1   + e*q2
// = (w0 + i*x0 + j*y0 + k*z0)   +   e*(we + i*xe + j*ye + k*ze)
struct DualQuaternion {
    vec4 real, dual;
};

vec3 dualquat_getTranslation(in DualQuaternion dq)
{
    return (2 * quat_mult(dq.dual, quat_conj(dq.real))).xyz;
}

DualQuaternion dualquat_conj(in DualQuaternion dq)
{
    return DualQuaternion(quat_conj(dq.real), quat_conj(dq.dual));
}

float dualquat_norm(in DualQuaternion dq)
{
    return quat_norm(dq.real);
}

DualQuaternion dualquat_add(in DualQuaternion first, in DualQuaternion second)
{
    if (dot(first.real, second.real) >= 0) {
        return DualQuaternion(first.real + second.real, first.dual + second.dual);
    } else {
        return DualQuaternion(first.real - second.real, first.dual - second.dual);
    }
}

DualQuaternion dualquat_mult(in float scalar, in DualQuaternion dq)
{
    return DualQuaternion(scalar * dq.real, scalar * dq.dual);
}

DualQuaternion dualquat_mult(in DualQuaternion first, in DualQuaternion second)
{
    return DualQuaternion(
        quat_mult(first.real, second.real),
        quat_mult(first.real, second.dual) + quat_mult(first.dual, second.real)
    );
}

DualQuaternion dualquat_normalize(in DualQuaternion dq)
{
    float inv_real_norm = 1.0 / quat_norm(dq.real);
    return DualQuaternion(
        dq.real * inv_real_norm,
        dq.dual * inv_real_norm
    );
}

#endif