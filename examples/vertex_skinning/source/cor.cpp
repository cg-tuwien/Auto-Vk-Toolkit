// based on https://github.com/pmbittner/OptimisedCentresOfRotationSkinning
// and https://github.com/damian0815/skellington

#pragma once
#include "cor.hpp"

float skinning_weights_distance(const weights_per_bone & wp, const weights_per_bone & wv) {
	return (wp - wv).norm();
}

float similarity(const weights_per_bone & wp, const weights_per_bone & wv, float sigma) {
	float sigma_squared = sigma * sigma;
	float sim = 0;
	for (int j = 0; j < wp.size(); ++j) {
		for (int k = 0; k < wv.size(); ++k) {
			if (j != k) {
				float exponent = wp[j] * wv[k] - wp[k] * wv[j];
				exponent *= exponent;
				exponent /= sigma_squared;

				sim += wp[j] * wp[k] * wv[j] * wv[k] * std::exp(-exponent);
			}
		}
	}
	return sim;
}

int& cor_triangle::operator[](int idx) {
	assert(idx < 3);
	return *(&mAlpha + idx);
}

bool cor_triangle::try_add_neighbor(cor_triangle & triangle) {
	for (cor_triangle * neighbour : mNeighbors)
		if (neighbour == &triangle)
			return false;

	int equal_vertices = 0;
	// two vertices have to be equal
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			if (operator[](i) == triangle[j]) {
				if (++equal_vertices == 2) {
					mNeighbors.push_back(&triangle);
					return true;
				}
			}
		}
	}
	return false;
}

bool cor_triangle::try_add_neighbor(cor_triangle & triangle, int equal_vertex) {
	for (cor_triangle * neighbour : mNeighbors)
		if (neighbour == &triangle)
			return false;

	// two vertices have to be equal
	for (int i = 0; i < 3; ++i) {
		if (operator[](i) == equal_vertex) continue;
		for (int j = 0; j < 3; ++j) {
			if (triangle[j] == equal_vertex) continue;
			if (operator[](i) == triangle[j]) {
				mNeighbors.push_back(&triangle);
				return true;
			}
		}
	}
	return false;
}

cor_calculator::cor_calculator(
	float sigma,
	float omega,
	bool subdivide,
	unsigned int number_of_threads_to_create)
	: mSigma(sigma), mOmega(omega), mSubdivide(subdivide), mNumThreads(number_of_threads_to_create)
{}

std::vector<weights_per_bone> cor_calculator::convert_weights(
	unsigned int num_bones,
	const std::vector<std::vector<unsigned int>>& skeleton_bone_indices,
	const std::vector<std::vector<float>>& skeleton_bone_weights) const
{
	std::vector<weights_per_bone> weights(skeleton_bone_weights.size(), weights_per_bone(num_bones));

	for (int i = 0; i < skeleton_bone_weights.size(); ++i) {
		const std::vector<unsigned int>& indices = skeleton_bone_indices[i];
		const std::vector<float>& weights_to_convert = skeleton_bone_weights[i];
		weights_per_bone &weights_to_set = weights[i];

		for (int weight_index = 0; weight_index < indices.size(); ++weight_index) {
			int bone_index = indices[weight_index];
			weights_to_set[bone_index] = weights_to_convert[weight_index];
		}
	}

	return weights;
}

bool cor_calculator::calculate_cor(unsigned long vertex, const cor_mesh & mesh, glm::vec3*cor_out) const {
	weights_per_bone weight = mesh.mWeights[vertex];

	glm::vec3 numerator(0);
	float denominator = 0;

	for (const cor_triangle & t : mesh.mTriangles) {
		float sim = similarity(weight, t.mAverageWeight, mSigma);

		float area_times_sim = t.mArea * sim;
		numerator += area_times_sim * t.mCenter;
		denominator += area_times_sim;
	}

	//p_i^*
	glm::vec3 cor(0);
	if (denominator != 0)
		cor = numerator / denominator;
	*cor_out = cor;

	return denominator != 0; // <=> has cor
}

void cor_calculator::calculate_cors_interval(unsigned long from, unsigned long to, const cor_mesh & mesh, std::vector<glm::vec3>& cors) const {
	for (unsigned long i = from; i < to; ++i) {
		calculate_cor(i, mesh, cors.data() + i);
	}
}

void cor_calculator::calculate_cors_async(const cor_mesh & mesh, const std::function<void(std::vector<glm::vec3>&)>& callback) {
	mWorker = std::async([this, mesh, callback] {
		using namespace std::chrono;

		unsigned long vertex_count = mesh.mVertices.size();

		// calculate CoRs
		std::vector<glm::vec3> cors(vertex_count, glm::vec3(0));
		{
			std::vector<std::future<void>> threads;
			auto interval = static_cast<unsigned long>(std::ceil(static_cast<double>(vertex_count) / static_cast<double>(mNumThreads)));

			for (unsigned long from = 0; from < vertex_count; from += interval) {
				unsigned long to = std::min(from + interval, vertex_count);
				threads.push_back(
					std::async(std::launch::async, &cor_calculator::calculate_cors_interval, this, from, to, std::ref(mesh), std::ref(cors))
				);
			}
		} // End of scrope destroys vector of threads, whose destructors force us to wait until they finished execution

		callback(cors);
	});
}

void cor_calculator::save_cors_to_binary_file(const std::string& path, std::vector<glm::vec3>& cors) const
{
	unsigned long cor_count = cors.size();

	std::ofstream output_file;
	output_file.open(path, std::ios::out | std::ios::binary);
	output_file << cor_count;
	output_file.write(reinterpret_cast<char*>(cors.data()), 3 * sizeof(float) * cors.size());
	output_file.close();
}

void cor_calculator::save_cors_to_text_file(const std::string& path, std::vector<glm::vec3>& cors, const std::string& separator) const {
	unsigned long cor_count = cors.size();

	std::ofstream output_file;
	output_file.open(path, std::ios::out);
	output_file << std::to_string(cor_count) << std::endl;
	for (auto& cor : cors) {
		output_file << std::to_string(cor.x) << separator << std::to_string(cor.y) << separator << std::to_string(cor.z) << "\n";
	}
	output_file.flush();
	output_file.close();
}

std::vector<glm::vec3> cor_calculator::load_cors_from_binary_file(const std::string& path) const
{
	unsigned int cor_count = 0;

	std::ifstream file(path, std::ifstream::in | std::ifstream::binary);
	file >> cor_count;
	std::vector<glm::vec3> cors(cor_count, glm::vec3(0.5));
	file.read(reinterpret_cast<char*>(cors.data()), 3 * sizeof(float) * cor_count);

	return cors;
}

cor_mesh cor_calculator::create_cor_mesh(
	const std::vector<glm::vec3>& vertices,
	const std::vector<unsigned int>& indices,
	std::vector<weights_per_bone>& skeleton_bone_weights,
	float subdiv_epsilon) const
{
	std::vector<glm::vec3> sub_vertices = vertices;
	std::vector<unsigned int> sub_triangle_indices = indices;
	std::vector<weights_per_bone> sub_weights = skeleton_bone_weights;

	// subdivision
	if (mSubdivide) {
		for (int t = 0; t < sub_triangle_indices.size(); t += 3) {
			for (int edge_beg = 0; edge_beg < 3; ++edge_beg) {
				int edge_end = (edge_beg + 1) % 3;

				int global_edge_beg = t + edge_beg;
				int global_edge_end = t + edge_end;

				unsigned int v0_index = sub_triangle_indices[global_edge_beg];
				unsigned int v1_index = sub_triangle_indices[global_edge_end];

				weights_per_bone & w0 = sub_weights[v0_index];
				weights_per_bone & w1 = sub_weights[v1_index];

				if (skinning_weights_distance(w0, w1) >= subdiv_epsilon) {
					// split edge into halfs

					// 1.) calc new vertex
					glm::vec3 v0 = sub_vertices[v0_index];
					glm::vec3 v1 = sub_vertices[v1_index];
					glm::vec3 new_vertex = 0.5f * (v0 + v1);
					unsigned int new_vertex_index = sub_vertices.size();
					sub_vertices.push_back(new_vertex);

					// 2.) calc new weight by linear interpolating
					sub_weights.push_back((w0 + w1) * 0.5f);

					// 3.) split triangle into two new ones
					unsigned int v2_index = sub_triangle_indices[t + ((edge_end + 1) % 3)];
					// append first new triangle at end of triangle list
					sub_triangle_indices.push_back(v0_index);
					sub_triangle_indices.push_back(v2_index);
					sub_triangle_indices.push_back(new_vertex_index);

					// replace current triangle with second new one and ...
					sub_triangle_indices[global_edge_beg] = new_vertex_index;
					//    => sub_triangle_indices[global_edge_end]   = global_edge_end;
					//    => sub_triangle_indices[lastVertexIndex] = lastVertexIndex;
					// ...recheck it
					t -= 3;
					break;
				}
			}
		}
	}

	cor_mesh mesh(
		sub_vertices,
		std::vector<cor_triangle>(sub_triangle_indices.size() / 3, cor_triangle()),
		sub_weights
	);

	// convert triangle indices to triangles and precompute cor values
	convert_triangles(sub_triangle_indices, &mesh);

	return mesh;
}

void cor_calculator::convert_triangles(std::vector<unsigned int> triangle_indices, cor_mesh * mesh) const
{
	std::vector<glm::vec3>& vertices = mesh->mVertices;
	std::vector<cor_triangle>& triangles = mesh->mTriangles;
	std::vector<weights_per_bone>& weights = mesh->mWeights;

	for (int i = 0; i < triangles.size(); ++i) {
		cor_triangle & t = triangles[i];

		int tIndex = 3 * i;
		t.mAlpha = triangle_indices[tIndex];
		t.mBeta = triangle_indices[tIndex + 1];
		t.mGamma = triangle_indices[tIndex + 2];

		glm::vec3 vm_alpha = vertices[t.mAlpha];
		glm::vec3 vm_beta = vertices[t.mBeta];
		glm::vec3 vm_gamma = vertices[t.mGamma];

		t.mCenter = (vm_alpha + vm_beta + vm_gamma) * (1.0f / 3.0f);

		t.mAverageWeight = (weights[t.mAlpha] + weights[t.mBeta] + weights[t.mGamma]) * (1.0f / 3.0f);

		glm::vec3 side_AB = vm_beta - vm_alpha;
		glm::vec3 side_AC = vm_gamma - vm_alpha;
		t.mArea = 0.5f * glm::length(glm::cross(side_AB, side_AC));
	}
}