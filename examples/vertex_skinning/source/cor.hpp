// based on https://github.com/pmbittner/OptimisedCentresOfRotationSkinning
// and https://github.com/damian0815/skellington

#pragma once
#include <gvk.hpp>
#include <string>
#include <vector>
#include <cmath>
#include <future>


struct weights_per_bone {
	// index of vector equals bone-index (the vertex-index must be remembered outside of this struct for an instance of a weights_per_bone)
	std::vector<float> weights;

	weights_per_bone() = default;
	explicit weights_per_bone(unsigned long n) {
		weights = std::vector<float>(n, 0);
	}

	unsigned long size() const {
		return weights.size();
	}

	float& operator[](int idx) {
		return weights[idx];
	}

	float operator[](int idx) const {
		return weights[idx];
	}

	/*
	 * Sums each individual weight from w_A to with each individual weight at the same index from w_b
	 * Therefore internally a new vector [w_A[0] + w_B[0], w_A[1] + w_B[1], w_A[2] + w_B[2], ...]
	 */
	weights_per_bone operator + (const weights_per_bone & other) const {
		weights_per_bone sum(size());
		for (int i = 0; i < size(); ++i)
			sum.weights[i] = weights[i] + other.weights[i];
		return sum;
	}

	weights_per_bone operator - (const weights_per_bone & other) const {
		weights_per_bone dif(size());
		for (int i = 0; i < size(); ++i)
			dif.weights[i] = weights[i] - other.weights[i];
		return dif;
	}

	weights_per_bone operator * (const float scalar) const {
		weights_per_bone ret(size());
		for (int i = 0; i < size(); ++i)
			ret.weights[i] = weights[i] * scalar;
		return ret;
	}

	float norm() const {
		float norm = 0;
		for (int i = 0; i < size(); ++i)
			norm += weights[i] * weights[i];
		return std::sqrt(norm);
	}

	void validate() {
		float sum_of_weight_of_vertex = 0;
		for (uint32_t bone_index = 0; bone_index < weights.size(); bone_index++) {
			if (weights[bone_index] < -0.0) {
				LOG_ERROR(std::string("Weight was: ") + std::to_string(weights[bone_index]));
				throw avk::runtime_error("Encountered weight which was < 0 !");
			}
			if (weights[bone_index] > 1.00001) {
				LOG_ERROR(std::string("Weight was: ") + std::to_string(weights[bone_index]));
				throw avk::runtime_error("Encountered weight which was > 1 !");
			}
			sum_of_weight_of_vertex += weights[bone_index];
			//LOG_INFO(std::string("Weight of bone ") + std::to_string(bone_index) + ": " + std::to_string(weights[bone_index]));
		}
		if (sum_of_weight_of_vertex > 1.00001 || sum_of_weight_of_vertex < 0.999) {
			LOG_ERROR(std::string("Sum of weight was: ") + std::to_string(sum_of_weight_of_vertex));
			throw avk::runtime_error("Weights did not sum up to 1 !");
		}
	}
};

struct cor_triangle {
	int mAlpha, mBeta, mGamma;

	glm::vec3 mCenter;
	weights_per_bone mAverageWeight;
	float mArea = 0;

	std::vector<cor_triangle *> mNeighbors;

	explicit cor_triangle(int bones = 0) : mAlpha(0), mBeta(0), mGamma(0), mCenter(glm::vec3(0)), mAverageWeight(weights_per_bone(bones)), mArea(0), mNeighbors(std::vector<cor_triangle *>()) {
		mNeighbors.reserve(3);
	}

	int& operator[](int idx);
	bool try_add_neighbor(cor_triangle & triangle);
	bool try_add_neighbor(cor_triangle & triangle, int equal_vertex);
};

struct cor_mesh {
	std::vector<glm::vec3> mVertices;
	std::vector<cor_triangle> mTriangles;
	std::vector<weights_per_bone> mWeights;

	// triangle adjacency graph
	std::vector<std::vector<cor_triangle *>> mTrianglesOfVertex;
	std::vector<std::vector<int>> mVerticesWithSimilarSkinningWeights;

	cor_mesh(
		const std::vector<glm::vec3>& vertices,
		const std::vector<cor_triangle>& triangles,
		const std::vector<weights_per_bone>& weights)
		: mVertices(vertices),
		  mTriangles(triangles),
		  mWeights(weights)
	{

	}
};

float skinning_weights_distance(const weights_per_bone & wp, const weights_per_bone & wv);
float similarity(const weights_per_bone & wp, const weights_per_bone & wv, float sigma);

class cor_calculator {
	std::future<void> mWorker;

protected:
	// cor computation values
	float mSigma;
	float mOmega;

	int mNumThreads;
	bool mSubdivide;

	// pre-calculation
	virtual void convert_triangles(std::vector<unsigned int> triangle_indices, cor_mesh * mesh) const;

	// cor computation
	virtual bool calculate_cor(unsigned long vertex, const cor_mesh & mesh, glm::vec3*cor_out) const;
	// [from, to)
	void calculate_cors_interval(unsigned long from, unsigned long to, const cor_mesh & mesh, std::vector<glm::vec3>& cors) const;

public:
	explicit cor_calculator(
		float sigma = 0.1f,
		float omega = 0.1f,
		bool subdivide = true,
		unsigned int number_of_threads_to_create = 4
	);

	std::vector<weights_per_bone> convert_weights(unsigned int num_bones,
											   const std::vector<std::vector<unsigned int>>&skeleton_bone_indices,
											   const std::vector<std::vector<float>>&skeleton_bone_weights) const;

	cor_mesh create_cor_mesh(
		const std::vector<glm::vec3>& vertices,
		const std::vector<unsigned int>& indices,
		std::vector<weights_per_bone>&skeleton_bone_weights,
		float subdiv_epsilon = 0.1f) const;

	void calculate_cors_async(const cor_mesh & mesh, const std::function<void(std::vector<glm::vec3>&)>& callback);

	// IO
	void save_cors_to_binary_file(const std::string& path, std::vector<glm::vec3>& cors) const;
	void save_cors_to_text_file(const std::string& path, std::vector<glm::vec3>& cors, const std::string& separator = ", ") const;
	std::vector<glm::vec3> load_cors_from_binary_file(const std::string& path) const;
};