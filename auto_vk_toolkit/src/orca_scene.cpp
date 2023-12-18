
#include "orca_scene.hpp"

namespace avk
{
	std::unordered_map<material_config, std::vector<model_and_mesh_indices>> orca_scene_t::distinct_material_configs_for_all_models(bool aAlsoConsiderCpuOnlyDataForDistinctMaterials)
	{
		std::unordered_map<material_config, std::vector<model_and_mesh_indices>> result;

		for (size_t i = 0; i < mModelData.size(); ++i) {
			auto modelMaterials = mModelData[i].mLoadedModel->distinct_material_configs(aAlsoConsiderCpuOnlyDataForDistinctMaterials);
			for (auto& pair : modelMaterials) {
				result[pair.first].push_back({ static_cast<model_index_t>(i), pair.second });
			}
		}

		return result;
	}

	avk::owning_resource<orca_scene_t> orca_scene_t::load_from_file(const std::string& aPath, model_t::aiProcessFlagsType aAssimpFlags)
	{
		std::ifstream stream(aPath, std::ifstream::in);
		if (!stream.good() || !stream || stream.fail())
		{
			throw avk::runtime_error(std::format("Unable to load scene from path[{}]", aPath));
		}
		std::string filecontents = std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
		if (filecontents.empty())
		{
			throw avk::runtime_error(std::format("Filecontents empty when loading scene from path[{}]", aPath));
		}

		nlohmann::json j = nlohmann::json::parse(filecontents);
		nlohmann::json jModels = j["models"];
		nlohmann::json jLights = j["lights"];
		nlohmann::json jCameras = j["cameras"];
		nlohmann::json jLightProbes = j["light_probes"];
		nlohmann::json jUserDefinded = j["user_defined"];
		nlohmann::json jPaths = j["paths"];

		orca_scene_t result;
		result.mLoadPath = aPath;

		// === process models === //
		result.mModelData.reserve(jModels.size());
		for (auto i = jModels.begin(); i != jModels.end(); ++i) {
			nlohmann::json jModel = i.value();
			model_data model;
			model.mFileName = convert_json_to_string(jModel["file"]);
			model.mName = convert_json_to_string(jModel["name"]);

			nlohmann::json jInstances = jModel["instances"];
			for (auto h = jInstances.begin(); h != jInstances.end(); ++h) {
				nlohmann::json jInstance = h.value();
				model_instance_data instance;
				instance.mName = convert_json_to_string(jInstance["name"]);
				instance.mTranslation = convert_json_to_vec3(jInstance["translation"]);
				instance.mScaling = convert_json_to_vec3(jInstance["scaling"]);
				instance.mRotation = convert_json_to_vec3(jInstance["rotation"]);
				model.mInstances.push_back(instance);
			}

			result.mModelData.push_back(model);
		}

		// === process lights === //
		for (auto i = jLights.begin(); i != jLights.end(); ++i) {
			nlohmann::json jLight = i.value();

			std::string lightType = jLight["type"];
			glm::vec3 direction = convert_json_to_vec3(jLight["direction"]);
			glm::vec3 intensity = convert_json_to_vec3(jLight["intensity"]);

			if (lightType.compare("dir_light") == 0) {
				direct_light_data d;
				d.mDirection = direction;
				d.mIntensity = intensity;
				result.mDirLightsData.push_back(d);
			} else if (lightType.compare("point_light") == 0) {
				point_light_data p;
				p.mDirection = direction;
				p.mIntensity = intensity;
				p.mPosition = convert_json_to_vec3(jLight["pos"]);
				p.mOpeningAngle = jLight["opening_angle"];
				p.mPenumbraAngle = jLight["penumbra_angle"];
				result.mPointLightsData.push_back(p);
			} else {
				std::string msg = "The light type '" + lightType + "' does not exist in this framework.";
				LOG_WARNING(msg);
			}
		}

		// === process cameras === //
		for (auto i = jCameras.begin(); i != jCameras.end(); ++i) {
			nlohmann::json jCamera = i.value();
			camera_data c;
			c.mPosition = convert_json_to_vec3(jCamera["pos"]);
			c.mTarget = convert_json_to_vec3(jCamera["target"]);
			c.mUp = convert_json_to_vec3(jCamera["up"]);
			c.mFocalLength = jCamera["focal_length"];
			c.mAspectRatio = jCamera["aspect_ratio"];
			std::vector<float> v = jCamera["depth_range"];
			c.mDepthRange = glm::vec2(v[0], v[1]);
			result.mCamerasData.push_back(c);
		}

		// === process light probes === //
		for (auto i = jLightProbes.begin(); i != jLightProbes.end(); ++i) {
			nlohmann::json jLightProbe = i.value();
			light_probes_data l;

			l.mFileName = convert_json_to_string(jLightProbe["file"]);
			l.mDiffSamples = jLightProbe["diff_samples"];
			l.mSpecSamples = jLightProbe["spec_samples"];
			l.mIntensity = convert_json_to_vec3(jLightProbe["intensity"]);
			l.mPosition = convert_json_to_vec3(jLightProbe["pos"]);
        
			result.mLightProbesData.push_back(l);
		}

		// === process userdefined === //
		if (!jUserDefinded.empty()) {
			user_defined_data data;
			data.mSkyBoxPath = convert_json_to_string(jUserDefinded["sky_box"]);
			result.mUserDefinedData.push_back(data);
		}

		// === process paths === //
		for (auto i = jPaths.begin(); i != jPaths.end(); ++i) {
			nlohmann::json jPath = i.value();

			path_data p;
			p.mName = convert_json_to_string(jPath["name"]);
			p.mLoop = jPath["loop"];
        
			nlohmann::json jFrames = jPath["frames"];
			std::vector<frame_data> frames;
			frames.reserve(jFrames.size());
			for (auto j = jFrames.begin(); j != jFrames.end(); ++j) {
				nlohmann::json jFrame = j.value();
				frame_data f;
				f.mTime = jFrame["time"];
				f.mPosition = convert_json_to_vec3(jFrame["pos"]);
				f.mTarget = convert_json_to_vec3(jFrame["target"]);
				f.mUp = convert_json_to_vec3(jFrame["up"]);
				frames.push_back(f);
			}

			p.mFrames = frames;
			result.mPathsData.push_back(p);
		}

		// Load the models into memory:
		auto fsceneBasePath = avk::extract_base_path(result.mLoadPath);
		for (auto& modelData : result.mModelData) {
			modelData.mFullPathName = avk::combine_paths(fsceneBasePath, modelData.mFileName);
			modelData.mLoadedModel = model_t::load_from_file(modelData.mFullPathName, aAssimpFlags);
		}
		
		return result;
	}

	glm::vec3 convert_json_to_vec3(nlohmann::json& j)
	{
		std::vector<float> v = j;
		if (v.size() != 3) {
			LOG_ERROR(std::format("Called function convert_json_to_vec3, but vector contains {} values", v.size()));
			return glm::vec3{};
		}
		return glm::vec3(v[0], v[1], v[2]);
	}

	
	std::string convert_json_to_string(nlohmann::json& j)
	{
		std::string s = j;
		return s;
	}
}
