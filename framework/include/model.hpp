#pragma once

namespace cgb
{
	struct material
	{
		std::string m_name;
		glm::vec3 m_diffuse_reflectivity;
		glm::vec3 m_specular_reflectivity;
		glm::vec3 m_ambient_reflectivity;
		glm::vec3 m_emissive_color;
		glm::vec3 m_transparent_color;
		bool m_wireframe_mode;
		bool m_twosided;
		cfg::color_blending_config m_blend_mode;
		float m_opacity;
		float m_shininess;
		float m_shininess_strength;
		float m_refraction_index;
		float m_reflectivity;
		std::string m_diffuse_tex;
		std::string m_specular_tex;
		std::string m_ambient_tex;
		std::string m_emissive_tex;
		std::string m_height_tex;
		std::string m_normals_tex;
		std::string m_shininess_tex;
		std::string m_opacity_tex;
		std::string m_displacement_tex;
		std::string m_reflection_tex;
		std::string m_lightmap_tex;
		glm::vec3 m_albedo;
		float m_metallic;
		float m_smoothness;
		float m_sheen;
		float m_thickness;
		float m_roughness;
		float m_anisotropy;
		glm::vec3 m_anisotropy_rotation;
		glm::vec2 m_offset;
		glm::vec2 m_tiling;
	};

	class model_t
	{
	public:
		using aiProcessFlagsType = unsigned int;

		model_t() = default;
		model_t(const model_t&) = delete;
		model_t(model_t&&) = default;
		model_t& operator=(const model_t&) = delete;
		model_t& operator=(model_t&&) = default;
		~model_t() = default;

		glm::mat4 transformation_matrix_for_mesh(size_t _MeshIndex);
		std::string name_of_mesh(size_t _MeshIndex);
		size_t material_index_for_mesh(size_t _MeshIndex);
		std::string name_of_material(size_t _MaterialIndex);
		material material_of_mesh(size_t _MeshIndex);

		std::vector<glm::vec3> positions_for_mesh(size_t _MeshIndex);
		std::vector<glm::vec3> normals_for_mesh(size_t _MeshIndex);
		std::vector<uint32_t> indices_for_mesh(size_t _MeshIndex);

		static owning_resource<model_t> load_from_file(const std::string& _Path, aiProcessFlagsType _AssimpFlags = 0);
		static owning_resource<model_t> load_from_memory(const std::string& _Memory, aiProcessFlagsType _AssimpFlags = 0);

	private:
		std::optional<glm::mat4> transformation_matrix_tranverser(const unsigned int _MeshIndexToFind, const aiNode* _Node, const aiMatrix4x4& _M);

		std::unique_ptr<Assimp::Importer> mImporter;
		const aiScene* mScene;
	};

	using model = owning_resource<model_t>;


}
