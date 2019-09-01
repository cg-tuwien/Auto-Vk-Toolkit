#pragma once

namespace cgb
{
	class orca_scene_t
	{
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

	public:
		orca_scene_t() = default;
		orca_scene_t(const orca_scene_t&) = delete;
		orca_scene_t(orca_scene_t&&) = default;
		orca_scene_t& operator=(const orca_scene_t&) = delete;
		orca_scene_t& operator=(orca_scene_t&&) = default;
		~orca_scene_t() = default;

		static owning_resource<orca_scene_t> load_from_file(const std::string& _Path, model_t::aiProcessFlagsType _AssimpFlags = aiProcess_Triangulate);

	private:
		static glm::vec3 convert_json_to_vec3(nlohmann::json& j);
		static std::string convert_json_to_string(nlohmann::json& j);

		std::string path;
		std::string filename;
		std::vector<model_data> mModels;
		std::vector<direct_light_data> mDirLights;
		std::vector<point_light_data> mPointLights;
		std::vector<camera_data> mCameras;
		std::vector<light_probes_data> mLightProbes;
		std::vector<user_defined_data> mUserDefinedData;
		std::vector<path_data> mPaths;
	};

	using orca_scene = cgb::owning_resource<orca_scene_t>;
}
