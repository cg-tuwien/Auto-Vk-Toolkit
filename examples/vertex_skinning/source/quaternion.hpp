#pragma once
#include <gvk.hpp>

class quaternion {
public:
	struct quaternion_struct {
		glm::vec4 q;
	};
	quaternion();
	quaternion(float theta, glm::vec3 axis);

	void set_values(glm::vec4 values);
	glm::vec4 get_values();

	static quaternion add(const quaternion& q1, const quaternion& q2);
	static quaternion mult(float s, const quaternion& q);
	static quaternion mult(const quaternion& q1, const quaternion& q2);
	static quaternion conj(const quaternion& q);
	static float mag(const quaternion& q);
	static quaternion normalize(const quaternion& q);
	static glm::mat4 to_matrix(quaternion q);
	static quaternion_struct to_struct(quaternion q);

private:
	// x,y,z,w
	glm::vec4 mValues;
};