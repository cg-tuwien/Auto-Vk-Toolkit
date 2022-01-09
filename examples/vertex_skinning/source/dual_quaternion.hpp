#pragma once
#include "quaternion.hpp"

class dual_quaternion {
public:
	struct dual_quaternion_struct {
		glm::vec4 r;
		glm::vec4 d;
	};
	
	dual_quaternion();
	dual_quaternion(quaternion r, glm::vec3 t);
	
	void set_real(quaternion r);
	void set_dual(quaternion d);
	quaternion get_real();
	quaternion get_dual();

	static dual_quaternion add(const dual_quaternion& q1, const dual_quaternion& q2);
	static dual_quaternion mult(float s, const dual_quaternion& q);
	static dual_quaternion mult(const dual_quaternion& q1, const dual_quaternion& q2);
	static dual_quaternion conj(const dual_quaternion& q);
	static float mag(const dual_quaternion& q);
	static dual_quaternion normalize(dual_quaternion q);
	static quaternion get_rotation(dual_quaternion q);
	static glm::vec3 get_translation(dual_quaternion q);
	static glm::mat4 to_matrix(dual_quaternion q);
	static dual_quaternion from_rot_and_trans(float theta, glm::vec3 axis, glm::vec3 t);
	static dual_quaternion from_mat4(glm::mat4 m);
	static dual_quaternion_struct to_struct(dual_quaternion dq);
	
private:
	quaternion mReal;
	quaternion mDual;
};