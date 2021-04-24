#include <gvk.hpp>

namespace gvk
{
	static inline std::tuple<std::vector<material_gpu_data>, std::vector<avk::image_sampler>> convert_for_gpu_usage_cached(
		const std::vector<gvk::material_config>& aMaterialConfigs,
		bool aLoadTexturesInSrgb,
		bool aFlipTextures,
		avk::image_usage aImageUsage,
		avk::filter_mode aTextureFilterMode,
		avk::border_handling_mode aBorderHandlingMode,
		avk::sync aSyncHandler,
		std::optional<std::reference_wrapper<gvk::serializer>> aSerializer = {})
	{
		// These are the texture names loaded from file -> mapped to vector of usage-pointers
		std::unordered_map<std::string, std::vector<int*>> texNamesToUsages;
		// Textures contained in this array shall be loaded into an sRGB format
		std::set<std::string> srgbTextures;

		// However, if some textures are missing, provide 1x1 px textures in those spots
		std::vector<int*> whiteTexUsages;				// Provide a 1x1 px almost everywhere in those cases,
		std::vector<int*> straightUpNormalTexUsages;	// except for normal maps, provide a normal pointing straight up there.

		std::vector<material_gpu_data> gpuMaterial;
		if (!aSerializer ||
			(aSerializer && (aSerializer->get().mode() == serializer::mode::serialize))) {
			size_t materialConfigSize = aMaterialConfigs.size();
			gpuMaterial.reserve(materialConfigSize); // important because of the pointers

			for (auto& mc : aMaterialConfigs) {
				auto& gm = gpuMaterial.emplace_back();
				gm.mDiffuseReflectivity = mc.mDiffuseReflectivity;
				gm.mAmbientReflectivity = mc.mAmbientReflectivity;
				gm.mSpecularReflectivity = mc.mSpecularReflectivity;
				gm.mEmissiveColor = mc.mEmissiveColor;
				gm.mTransparentColor = mc.mTransparentColor;
				gm.mReflectiveColor = mc.mReflectiveColor;
				gm.mAlbedo = mc.mAlbedo;

				gm.mOpacity = mc.mOpacity;
				gm.mBumpScaling = mc.mBumpScaling;
				gm.mShininess = mc.mShininess;
				gm.mShininessStrength = mc.mShininessStrength;

				gm.mRefractionIndex = mc.mRefractionIndex;
				gm.mReflectivity = mc.mReflectivity;
				gm.mMetallic = mc.mMetallic;
				gm.mSmoothness = mc.mSmoothness;

				gm.mSheen = mc.mSheen;
				gm.mThickness = mc.mThickness;
				gm.mRoughness = mc.mRoughness;
				gm.mAnisotropy = mc.mAnisotropy;

				gm.mAnisotropyRotation = mc.mAnisotropyRotation;
				gm.mCustomData = mc.mCustomData;

				gm.mDiffuseTexIndex = -1;
				if (mc.mDiffuseTex.empty()) {
					whiteTexUsages.push_back(&gm.mDiffuseTexIndex);
				}
				else {
					auto path = avk::clean_up_path(mc.mDiffuseTex);
					texNamesToUsages[path].push_back(&gm.mDiffuseTexIndex);
					if (aLoadTexturesInSrgb) {
						srgbTextures.insert(path);
					}
				}

				gm.mSpecularTexIndex = -1;
				if (mc.mSpecularTex.empty()) {
					whiteTexUsages.push_back(&gm.mSpecularTexIndex);
				}
				else {
					texNamesToUsages[avk::clean_up_path(mc.mSpecularTex)].push_back(&gm.mSpecularTexIndex);
				}

				gm.mAmbientTexIndex = -1;
				if (mc.mAmbientTex.empty()) {
					whiteTexUsages.push_back(&gm.mAmbientTexIndex);
				}
				else {
					auto path = avk::clean_up_path(mc.mAmbientTex);
					texNamesToUsages[path].push_back(&gm.mAmbientTexIndex);
					if (aLoadTexturesInSrgb) {
						srgbTextures.insert(path);
					}
				}

				gm.mEmissiveTexIndex = -1;
				if (mc.mEmissiveTex.empty()) {
					whiteTexUsages.push_back(&gm.mEmissiveTexIndex);
				}
				else {
					texNamesToUsages[avk::clean_up_path(mc.mEmissiveTex)].push_back(&gm.mEmissiveTexIndex);
				}

				gm.mHeightTexIndex = -1;
				if (mc.mHeightTex.empty()) {
					whiteTexUsages.push_back(&gm.mHeightTexIndex);
				}
				else {
					texNamesToUsages[avk::clean_up_path(mc.mHeightTex)].push_back(&gm.mHeightTexIndex);
				}

				gm.mNormalsTexIndex = -1;
				if (mc.mNormalsTex.empty()) {
					straightUpNormalTexUsages.push_back(&gm.mNormalsTexIndex);
				}
				else {
					texNamesToUsages[avk::clean_up_path(mc.mNormalsTex)].push_back(&gm.mNormalsTexIndex);
				}

				gm.mShininessTexIndex = -1;
				if (mc.mShininessTex.empty()) {
					whiteTexUsages.push_back(&gm.mShininessTexIndex);
				}
				else {
					texNamesToUsages[avk::clean_up_path(mc.mShininessTex)].push_back(&gm.mShininessTexIndex);
				}

				gm.mOpacityTexIndex = -1;
				if (mc.mOpacityTex.empty()) {
					whiteTexUsages.push_back(&gm.mOpacityTexIndex);
				}
				else {
					texNamesToUsages[avk::clean_up_path(mc.mOpacityTex)].push_back(&gm.mOpacityTexIndex);
				}

				gm.mDisplacementTexIndex = -1;
				if (mc.mDisplacementTex.empty()) {
					whiteTexUsages.push_back(&gm.mDisplacementTexIndex);
				}
				else {
					texNamesToUsages[avk::clean_up_path(mc.mDisplacementTex)].push_back(&gm.mDisplacementTexIndex);
				}

				gm.mReflectionTexIndex = -1;
				if (mc.mReflectionTex.empty()) {
					whiteTexUsages.push_back(&gm.mReflectionTexIndex);
				}
				else {
					texNamesToUsages[avk::clean_up_path(mc.mReflectionTex)].push_back(&gm.mReflectionTexIndex);
				}

				gm.mLightmapTexIndex = -1;
				if (mc.mLightmapTex.empty()) {
					whiteTexUsages.push_back(&gm.mLightmapTexIndex);
				}
				else {
					texNamesToUsages[avk::clean_up_path(mc.mLightmapTex)].push_back(&gm.mLightmapTexIndex);
				}

				gm.mExtraTexIndex = -1;
				if (mc.mExtraTex.empty()) {
					whiteTexUsages.push_back(&gm.mExtraTexIndex);
				}
				else {
					auto path = avk::clean_up_path(mc.mExtraTex);
					texNamesToUsages[path].push_back(&gm.mExtraTexIndex);
					if (aLoadTexturesInSrgb) {
						srgbTextures.insert(path);
					}
				}

				gm.mDiffuseTexOffsetTiling = mc.mDiffuseTexOffsetTiling;
				gm.mSpecularTexOffsetTiling = mc.mSpecularTexOffsetTiling;
				gm.mAmbientTexOffsetTiling = mc.mAmbientTexOffsetTiling;
				gm.mEmissiveTexOffsetTiling = mc.mEmissiveTexOffsetTiling;
				gm.mHeightTexOffsetTiling = mc.mHeightTexOffsetTiling;
				gm.mNormalsTexOffsetTiling = mc.mNormalsTexOffsetTiling;
				gm.mShininessTexOffsetTiling = mc.mShininessTexOffsetTiling;
				gm.mOpacityTexOffsetTiling = mc.mOpacityTexOffsetTiling;
				gm.mDisplacementTexOffsetTiling = mc.mDisplacementTexOffsetTiling;
				gm.mReflectionTexOffsetTiling = mc.mReflectionTexOffsetTiling;
				gm.mLightmapTexOffsetTiling = mc.mLightmapTexOffsetTiling;
				gm.mExtraTexOffsetTiling = mc.mExtraTexOffsetTiling;
			}
		}

		std::vector<avk::image_sampler> imageSamplers;

		size_t numTexUsages = texNamesToUsages.size();
		size_t numWhiteTexUsages = whiteTexUsages.empty() ? 0 : 1;
		size_t numStraightUpNormalTexUsages = (straightUpNormalTexUsages.empty() ? 0 : 1);

		if (aSerializer) {
			aSerializer->get().archive(numTexUsages);
			aSerializer->get().archive(numWhiteTexUsages);
			aSerializer->get().archive(numStraightUpNormalTexUsages);
		}

		const auto numSamplers = numTexUsages + numWhiteTexUsages + numStraightUpNormalTexUsages;
		imageSamplers.reserve(numSamplers);

		auto getSync = [numSamplers, &aSyncHandler, lSyncCount = size_t{0}]() mutable -> avk::sync {
			++lSyncCount;
			if (lSyncCount < numSamplers) {
				return avk::sync::auxiliary_with_barriers(aSyncHandler, avk::sync::steal_before_handler_on_demand, {}); // Invoke external sync exactly once (if there is something to sync)
			}
			assert(lSyncCount == numSamplers);
			return std::move(aSyncHandler); // For the last image, pass the main sync => this will also have the after-handler invoked.
		};

		// Create the white texture and assign its index to all usages
		if (numWhiteTexUsages > 0) {
			// Dont need to check if we have a serializer since create_1px_texture_cached takes it as an optional
			imageSamplers.push_back(
				context().create_image_sampler(
					owned(context().create_image_view(
						owned(create_1px_texture_cached({ 255, 255, 255, 255 }, vk::Format::eR8G8B8A8Unorm, avk::memory_usage::device, aImageUsage, getSync(), aSerializer))
					)),
					owned(context().create_sampler(avk::filter_mode::nearest_neighbor, avk::border_handling_mode::repeat))
				)
			);
			if (!aSerializer ||
				(aSerializer && (aSerializer->get().mode() == serializer::mode::serialize))) {
				int index = static_cast<int>(imageSamplers.size() - 1);
				for (auto* img : whiteTexUsages) {
					*img = index;
				}
			}
		}

		// Create the normal texture, containing a normal pointing straight up, and assign to all usages
		if (numStraightUpNormalTexUsages > 0) {
			// Dont need to check if we have a serializer since create_1px_texture_cached takes it as an optional
			imageSamplers.push_back(
				context().create_image_sampler(
					owned(context().create_image_view(
						owned(create_1px_texture_cached({ 127, 127, 255, 0 }, vk::Format::eR8G8B8A8Unorm, avk::memory_usage::device, aImageUsage, getSync(), aSerializer))
					)),
					owned(context().create_sampler(avk::filter_mode::nearest_neighbor, avk::border_handling_mode::repeat))
				)
			);
			if (!aSerializer ||
				(aSerializer && (aSerializer->get().mode() == serializer::mode::serialize))) {
				int index = static_cast<int>(imageSamplers.size() - 1);
				for (auto* img : straightUpNormalTexUsages) {
					*img = index;
				}
			}
		}

		// Load all the images from file, and assign them to all usages
		if (!aSerializer ||
			(aSerializer && (aSerializer->get().mode() == serializer::mode::serialize))) {
			// create_image_from_file_cached takes the serializer as an optional,
			// therefore the call is safe with and without one
			for (auto& pair : texNamesToUsages) {
				assert(!pair.first.empty());

				bool potentiallySrgb = srgbTextures.contains(pair.first);

				imageSamplers.push_back(
					context().create_image_sampler(
						owned(context().create_image_view(
							create_image_from_file_cached(pair.first, true, potentiallySrgb, aFlipTextures, 4, avk::memory_usage::device, aImageUsage, getSync(), aSerializer)
						)),
						owned(context().create_sampler(aTextureFilterMode, aBorderHandlingMode))
					)
				);
				int index = static_cast<int>(imageSamplers.size() - 1);
				for (auto* img : pair.second) {
					*img = index;
				}
			}
		}
		else {
			// We sure have the serializer here
			for (int i = 0; i < numTexUsages; ++i) {
				// create_image_from_file_cached does not need these values, as the
				// image data is loaded from a cache file, so we can just pass some
				// defaults to avoid having to save them to the cache file
				const bool potentiallySrgbDontCare = false;
				const std::string pathDontCare = "";

				imageSamplers.push_back(
					context().create_image_sampler(
						owned(context().create_image_view(
							create_image_from_file_cached(pathDontCare, true, potentiallySrgbDontCare, aFlipTextures, 4, avk::memory_usage::device, aImageUsage, getSync(), aSerializer)
						)),
						owned(context().create_sampler(aTextureFilterMode, aBorderHandlingMode))
					)
				);
			}
		}

		if (aSerializer) {
			aSerializer->get().archive(gpuMaterial);
		}

		// Hand over ownership to the caller
		return std::make_tuple(std::move(gpuMaterial), std::move(imageSamplers));
	}

	std::tuple<std::vector<material_gpu_data>, std::vector<avk::image_sampler>> convert_for_gpu_usage(
		const std::vector<gvk::material_config>& aMaterialConfigs, 
		bool aLoadTexturesInSrgb,
		bool aFlipTextures,
		avk::image_usage aImageUsage,
		avk::filter_mode aTextureFilterMode, 
		avk::border_handling_mode aBorderHandlingMode,
		avk::sync aSyncHandler)
	{
		return convert_for_gpu_usage_cached(
			aMaterialConfigs,
			aLoadTexturesInSrgb,
			aFlipTextures,
			aImageUsage,
			aTextureFilterMode,
			aBorderHandlingMode,
			std::move(aSyncHandler));
	}

	std::tuple<std::vector<material_gpu_data>, std::vector<avk::image_sampler>> convert_for_gpu_usage_cached(
		gvk::serializer& aSerializer,
		const std::vector<gvk::material_config>& aMaterialConfigs,
		bool aLoadTexturesInSrgb,
		bool aFlipTextures,
		avk::image_usage aImageUsage,
		avk::filter_mode aTextureFilterMode,
		avk::border_handling_mode aBorderHandlingMode,
		avk::sync aSyncHandler)
	{
		return convert_for_gpu_usage_cached(
			aMaterialConfigs,
			aLoadTexturesInSrgb,
			aFlipTextures,
			aImageUsage,
			aTextureFilterMode,
			aBorderHandlingMode,
			std::move(aSyncHandler),
			aSerializer);
	}

	std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>> get_vertices_and_indices(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> positionsData;
		std::vector<uint32_t> indicesData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<mesh_index_t>>(pair)) {
				gvk::append_indices_and_vertex_data(
					gvk::additional_index_data(	indicesData,	[&]() { return modelRef.get().indices_for_mesh<uint32_t>(meshIndex);	} ),
					gvk::additional_vertex_data(positionsData,	[&]() { return modelRef.get().positions_for_mesh(meshIndex);			} )
				);
			}
		}

		return std::make_tuple( std::move(positionsData), std::move(indicesData) );
	}

	std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>> get_vertices_and_indices_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes)
	{
		std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>> verticesAndIndices;
		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			verticesAndIndices = get_vertices_and_indices(aModelsAndSelectedMeshes);
		}
		aSerializer.archive(verticesAndIndices);
		return verticesAndIndices;
	}

	std::vector<glm::vec3> get_normals(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> normalsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<mesh_index_t>>(pair)) {
				insert_into(normalsData, modelRef.get().normals_for_mesh(meshIndex));
			}
		}

		return normalsData;
	}

	std::vector<glm::vec3> get_normals_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> normalsData;
		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			normalsData = get_normals(aModelsAndSelectedMeshes);
		}
		aSerializer.archive(normalsData);

		return normalsData;
	}
	
	std::vector<glm::vec3> get_tangents(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> tangentsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<mesh_index_t>>(pair)) {
				insert_into(tangentsData, modelRef.get().tangents_for_mesh(meshIndex));
			}
		}

		return tangentsData;
	}

	std::vector<glm::vec3> get_tangents_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> tangentsData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			tangentsData = get_tangents(aModelsAndSelectedMeshes);
		}
		aSerializer.archive(tangentsData);

		return tangentsData;
	}
	

	std::vector<glm::vec3> get_bitangents(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> bitangentsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<mesh_index_t>>(pair)) {
				insert_into(bitangentsData, modelRef.get().bitangents_for_mesh(meshIndex));
			}
		}

		return bitangentsData;
	}

	std::vector<glm::vec3> get_bitangents_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> bitangentsData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			bitangentsData = get_bitangents(aModelsAndSelectedMeshes);
		}
		aSerializer.archive(bitangentsData);

		return bitangentsData;
	}

	std::vector<glm::vec4> get_colors(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aColorsSet)
	{
		std::vector<glm::vec4> colorsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<mesh_index_t>>(pair)) {
				insert_into(colorsData, modelRef.get().colors_for_mesh(meshIndex, aColorsSet));
			}
		}

		return colorsData;
	}

	std::vector<glm::vec4> get_colors_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aColorsSet)
	{
		std::vector<glm::vec4> colorsData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			colorsData = get_colors(aModelsAndSelectedMeshes, aColorsSet);
		}
		aSerializer.archive(colorsData);

		return colorsData;
	}


	std::vector<glm::vec4> get_bone_weights(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, bool aNormalizeBoneWeights)
	{
		std::vector<glm::vec4> boneWeightsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<mesh_index_t>>(pair)) {
				insert_into(boneWeightsData, modelRef.get().bone_weights_for_mesh(meshIndex, aNormalizeBoneWeights));
			}
		}

		return boneWeightsData;
	}

	std::vector<glm::vec4> get_bone_weights_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, bool aNormalizeBoneWeights)
	{
		std::vector<glm::vec4> boneWeightsData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			boneWeightsData = get_bone_weights(aModelsAndSelectedMeshes, aNormalizeBoneWeights);
		}
		aSerializer.archive(boneWeightsData);

		return boneWeightsData;
	}

	std::vector<glm::uvec4> get_bone_indices(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aBoneIndexOffset)
	{
		std::vector<glm::uvec4> boneIndicesData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<mesh_index_t>>(pair)) {
				insert_into(boneIndicesData, modelRef.get().bone_indices_for_mesh(meshIndex, aBoneIndexOffset));
			}
		}

		return boneIndicesData;
	}

	std::vector<glm::uvec4> get_bone_indices_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aBoneIndexOffset)
	{
		std::vector<glm::uvec4> boneIndicesData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			boneIndicesData = get_bone_indices(aModelsAndSelectedMeshes, aBoneIndexOffset);
		}
		aSerializer.archive(boneIndicesData);

		return boneIndicesData;
	}

	std::vector<glm::uvec4> get_bone_indices_for_single_target_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aInitialBoneIndexOffset)
	{
		std::vector<glm::uvec4> boneIndicesData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			insert_into(boneIndicesData, modelRef.get().bone_indices_for_meshes_for_single_target_buffer(std::get<std::vector<mesh_index_t>>(pair), aInitialBoneIndexOffset));
		}

		return boneIndicesData;
	}

	std::vector<glm::uvec4> get_bone_indices_for_single_target_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aInitialBoneIndexOffset)
	{
		std::vector<glm::uvec4> boneIndicesData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			boneIndicesData = get_bone_indices_for_single_target_buffer(aModelsAndSelectedMeshes, aInitialBoneIndexOffset);
		}
		aSerializer.archive(boneIndicesData);

		return boneIndicesData;
	}

	std::vector<glm::uvec4> get_bone_indices_for_single_target_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, const std::vector<mesh_index_t>& aReferenceMeshIndices)
	{
		std::vector<glm::uvec4> boneIndicesData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<mesh_index_t>>(pair)) {
				insert_into(boneIndicesData, modelRef.get().bone_indices_for_mesh_for_single_target_buffer(meshIndex, aReferenceMeshIndices));
			}
		}

		return boneIndicesData;
	}

	std::vector<glm::uvec4> get_bone_indices_for_single_target_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, const std::vector<mesh_index_t>& aReferenceMeshIndices)
	{
		std::vector<glm::uvec4> boneIndicesData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			boneIndicesData = get_bone_indices_for_single_target_buffer(aModelsAndSelectedMeshes, aReferenceMeshIndices);
		}
		aSerializer.archive(boneIndicesData);

		return boneIndicesData;
	}

	std::vector<glm::vec2> get_2d_texture_coordinates(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet)
	{
		std::vector<glm::vec2> texCoordsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<mesh_index_t>>(pair)) {
				insert_into(texCoordsData, modelRef.get().texture_coordinates_for_mesh<glm::vec2>(meshIndex, aTexCoordSet));
			}
		}

		return texCoordsData;
	}

	std::vector<glm::vec2> get_2d_texture_coordinates_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet)
	{
		std::vector<glm::vec2> texCoordsData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			texCoordsData = get_2d_texture_coordinates(aModelsAndSelectedMeshes, aTexCoordSet);
		}
		aSerializer.archive(texCoordsData);

		return texCoordsData;
	}

	std::vector<glm::vec2> get_2d_texture_coordinates_flipped(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet)
	{
		std::vector<glm::vec2> texCoordsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<mesh_index_t>>(pair)) {
				insert_into(texCoordsData, modelRef.get().texture_coordinates_for_mesh<glm::vec2>([](const glm::vec2& aValue){ return glm::vec2{aValue.x, 1.0f - aValue.y}; }, meshIndex, aTexCoordSet));
			}
		}

		return texCoordsData;
	}

	std::vector<glm::vec2> get_2d_texture_coordinates_flipped_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet)
	{
		std::vector<glm::vec2> texCoordsData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			texCoordsData = get_2d_texture_coordinates_flipped(aModelsAndSelectedMeshes, aTexCoordSet);
		}
		aSerializer.archive(texCoordsData);

		return texCoordsData;
	}

	std::vector<glm::vec3> get_3d_texture_coordinates(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet)
	{
		std::vector<glm::vec3> texCoordsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<mesh_index_t>>(pair)) {
				insert_into(texCoordsData, modelRef.get().texture_coordinates_for_mesh<glm::vec3>(meshIndex, aTexCoordSet));
			}
		}

		return texCoordsData;
	}

	std::vector<glm::vec3> get_3d_texture_coordinates_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet)
	{
		std::vector<glm::vec3> texCoordsData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			texCoordsData = get_3d_texture_coordinates(aModelsAndSelectedMeshes, aTexCoordSet);
		}
		aSerializer.archive(texCoordsData);

		return texCoordsData;
	}
}
