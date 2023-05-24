#pragma once

#include "model.hpp"

namespace avk
{
	struct model_and_mesh_indices
	{
		model_index_t mModelIndex;
		std::vector<mesh_index_t> mMeshIndices;
	};

	struct model_instance_data
	{
		std::string mName;
		glm::vec3 mTranslation;
		glm::vec3 mScaling;
		glm::vec3 mRotation;
	};

	struct model_data
	{
		std::string mFileName;
		std::string mName;
		std::vector<model_instance_data> mInstances;
		std::string mFullPathName;
		avk::model mLoadedModel;
	};

	struct direct_light_data
	{
		glm::vec3 mDirection;
		glm::vec3 mIntensity;
	};

	struct point_light_data
	{
		glm::vec3 mDirection;
		glm::vec3 mIntensity;
		glm::vec3 mPosition;
		float mOpeningAngle;
		float mPenumbraAngle;
	};

	struct light_probes_data
	{
		std::string mFileName;
		int mDiffSamples;
		int mSpecSamples;
		glm::vec3 mIntensity;
		glm::vec3 mPosition;
	};

	struct camera_data 
	{
		glm::vec3 mPosition;
		glm::vec3 mTarget;
		glm::vec3 mUp;
		glm::vec2 mDepthRange;
		float mFocalLength;
		float mAspectRatio;
	};

	struct user_defined_data
	{
		std::string mSkyBoxPath;
	};

	struct frame_data
	{
		float mTime;
		glm::vec3 mPosition;
		glm::vec3 mTarget;
		glm::vec3 mUp;
	};

	struct path_data 
	{
		std::string mName;
		bool mLoop;
		std::vector<frame_data> mFrames;
	};

	/** A class representing an ORCA Scene */
	class orca_scene_t
	{
	public:
		orca_scene_t() = default;
		orca_scene_t(orca_scene_t&&) noexcept = default;
		orca_scene_t(const orca_scene_t&) = delete;
		orca_scene_t& operator=(orca_scene_t&&) noexcept = default;
		orca_scene_t& operator=(const orca_scene_t&) = delete;
		~orca_scene_t() = default;

		const auto& models() const { return mModelData; }
		auto& models() { return mModelData; }
		const auto& model_at_index(size_t aIndex) const { return mModelData[aIndex]; }
		auto& model_at_index(size_t aIndex) { return mModelData[aIndex]; }
		const auto& directional_lights() const { return mDirLightsData; }
		const auto& point_lights() const { return mPointLightsData; }
		const auto& cameras() const { return mCamerasData; }
		const auto& light_probes() const { return mLightProbesData; }
		const auto& paths() const { return mPathsData; }

		/** Return the indices of all models which the given predicate evaluates true for.
		 *	@tparam F	bool(size_t, const model_data&) where the first parameter is the
		 *				model index and the second a reference to the loaded data, which
		 *				also includes the loaded model_t.
		 */
		template <typename F>
		std::vector<size_t> select_models(F aPredicate) const
		{
			std::vector<size_t> result;
			for (size_t i = 0; i < mModelData.size(); ++i) {
				if (aPredicate(i, mModelData[i])) {
					result.push_back(i);
				}
			}
			return result;
		}

		/**	Gets all distinct `material_config` structs for all of this ORACA scene's models and also get all the model indices and mesh indices which have the materials assigned to.
		 *	@param	aAlsoConsiderCpuOnlyDataForDistinctMaterials	Setting this parameter to `true` means that for determining if a material is unique or not,
		 *															also the data in the material struct are evaluated which only remain on the CPU. This CPU 
		 *															data will not be transmitted to the GPU. By default, this parameter is set to `false`, i.e.
		 *															only the GPU data of the `material_config` struct will be evaluated when determining the distinct
		 *															materials.
		 *															You'll want to set this parameter to `true` if you are planning to adapt your draw calls based
		 *															on one or all of the following `material_config` members: `mShadingModel`, `mWireframeMode`,
		 *															`mTwosided`, `mBlendMode`. If you don't plan to differentiate based on these, set to `false`.
		 *	@return	A `std::unordered_map` containing the distinct `material_config` structs as the
		 *			keys and a vector of tuples value type, where the tuples consist of the following two elements:
		 *				[0] The model index as `size_t` at the first spot, and
		 *				[1] The model's mesh indices as `std::vector<size_t>` at the second spot.
		 */
		std::unordered_map<material_config, std::vector<model_and_mesh_indices>> distinct_material_configs_for_all_models(bool aAlsoConsiderCpuOnlyDataForDistinctMaterials = false);

		static avk::owning_resource<orca_scene_t> load_from_file(const std::string& aPath, model_t::aiProcessFlagsType aAssimpFlags = aiProcess_Triangulate | aiProcess_PreTransformVertices);

	private:
		std::string mLoadPath;
		std::vector<model_data> mModelData;
		std::vector<direct_light_data> mDirLightsData;
		std::vector<point_light_data> mPointLightsData;
		std::vector<camera_data> mCamerasData;
		std::vector<light_probes_data> mLightProbesData;
		std::vector<user_defined_data> mUserDefinedData;
		std::vector<path_data> mPathsData;
	};

	using orca_scene = avk::owning_resource<orca_scene_t>;

	// JSON helper functions:
	static glm::vec3 convert_json_to_vec3(nlohmann::json& j);
	static std::string convert_json_to_string(nlohmann::json& j);
}
