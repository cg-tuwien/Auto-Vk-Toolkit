#include <gvk.hpp>

namespace gvk
{

	std::tuple<std::vector<material_gpu_data>, std::vector<avk::image_sampler>> convert_for_gpu_usage(
		const std::vector<gvk::material_config>& aMaterialConfigs, 
		bool aLoadTexturesInSrgb,
		bool aFlipTextures,
		avk::image_usage aImageUsage,
		avk::filter_mode aTextureFilterMode, 
		avk::border_handling_mode aBorderHandlingMode,
		avk::sync aSyncHandler)
	{
		// These are the texture names loaded from file -> mapped to vector of usage-pointers
		std::unordered_map<std::string, std::vector<int*>> texNamesToUsages;
		// Textures contained in this array shall be loaded into an sRGB format
		std::set<std::string> srgbTextures;
		
		// However, if some textures are missing, provide 1x1 px textures in those spots
		std::vector<int*> whiteTexUsages;				// Provide a 1x1 px almost everywhere in those cases,
		std::vector<int*> straightUpNormalTexUsages;	// except for normal maps, provide a normal pointing straight up there.

		std::vector<material_gpu_data> gpuMaterial;
		gpuMaterial.reserve(aMaterialConfigs.size()); // important because of the pointers

		for (auto& mc : aMaterialConfigs) {
			auto& gm = gpuMaterial.emplace_back();
			gm.mDiffuseReflectivity			= mc.mDiffuseReflectivity		 ;
			gm.mAmbientReflectivity			= mc.mAmbientReflectivity		 ;
			gm.mSpecularReflectivity		= mc.mSpecularReflectivity		 ;
			gm.mEmissiveColor				= mc.mEmissiveColor				 ;
			gm.mTransparentColor			= mc.mTransparentColor			 ;
			gm.mReflectiveColor				= mc.mReflectiveColor			 ;
			gm.mAlbedo						= mc.mAlbedo					 ;
																			 
			gm.mOpacity						= mc.mOpacity					 ;
			gm.mBumpScaling					= mc.mBumpScaling				 ;
			gm.mShininess					= mc.mShininess					 ;
			gm.mShininessStrength			= mc.mShininessStrength			 ;
																			
			gm.mRefractionIndex				= mc.mRefractionIndex			 ;
			gm.mReflectivity				= mc.mReflectivity				 ;
			gm.mMetallic					= mc.mMetallic					 ;
			gm.mSmoothness					= mc.mSmoothness				 ;
																			 
			gm.mSheen						= mc.mSheen						 ;
			gm.mThickness					= mc.mThickness					 ;
			gm.mRoughness					= mc.mRoughness					 ;
			gm.mAnisotropy					= mc.mAnisotropy				 ;
																			 
			gm.mAnisotropyRotation			= mc.mAnisotropyRotation		 ;
			gm.mCustomData					= mc.mCustomData				 ;
																			 
			gm.mDiffuseTexIndex				= -1;	
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

			gm.mSpecularTexIndex			= -1;							 
			if (mc.mSpecularTex.empty()) {
				whiteTexUsages.push_back(&gm.mSpecularTexIndex);
			}
			else {
				texNamesToUsages[avk::clean_up_path(mc.mSpecularTex)].push_back(&gm.mSpecularTexIndex);
			}

			gm.mAmbientTexIndex				= -1;							 
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

			gm.mEmissiveTexIndex			= -1;							 
			if (mc.mEmissiveTex.empty()) {
				whiteTexUsages.push_back(&gm.mEmissiveTexIndex);
			}
			else {
				texNamesToUsages[avk::clean_up_path(mc.mEmissiveTex)].push_back(&gm.mEmissiveTexIndex);
			}

			gm.mHeightTexIndex				= -1;							 
			if (mc.mHeightTex.empty()) {
				whiteTexUsages.push_back(&gm.mHeightTexIndex);
			}
			else {
				texNamesToUsages[avk::clean_up_path(mc.mHeightTex)].push_back(&gm.mHeightTexIndex);
			}

			gm.mNormalsTexIndex				= -1;							 
			if (mc.mNormalsTex.empty()) {
				straightUpNormalTexUsages.push_back(&gm.mNormalsTexIndex);
			}
			else {
				texNamesToUsages[avk::clean_up_path(mc.mNormalsTex)].push_back(&gm.mNormalsTexIndex);
			}

			gm.mShininessTexIndex			= -1;							 
			if (mc.mShininessTex.empty()) {
				whiteTexUsages.push_back(&gm.mShininessTexIndex);
			}
			else {
				texNamesToUsages[avk::clean_up_path(mc.mShininessTex)].push_back(&gm.mShininessTexIndex);
			}

			gm.mOpacityTexIndex				= -1;							 
			if (mc.mOpacityTex.empty()) {
				whiteTexUsages.push_back(&gm.mOpacityTexIndex);
			}
			else {
				texNamesToUsages[avk::clean_up_path(mc.mOpacityTex)].push_back(&gm.mOpacityTexIndex);
			}

			gm.mDisplacementTexIndex		= -1;							 
			if (mc.mDisplacementTex.empty()) {
				whiteTexUsages.push_back(&gm.mDisplacementTexIndex);
			}
			else {
				texNamesToUsages[avk::clean_up_path(mc.mDisplacementTex)].push_back(&gm.mDisplacementTexIndex);
			}

			gm.mReflectionTexIndex			= -1;							 
			if (mc.mReflectionTex.empty()) {
				whiteTexUsages.push_back(&gm.mReflectionTexIndex);
			}
			else {
				texNamesToUsages[avk::clean_up_path(mc.mReflectionTex)].push_back(&gm.mReflectionTexIndex);
			}

			gm.mLightmapTexIndex			= -1;							 
			if (mc.mLightmapTex.empty()) {
				whiteTexUsages.push_back(&gm.mLightmapTexIndex);
			}
			else {
				texNamesToUsages[avk::clean_up_path(mc.mLightmapTex)].push_back(&gm.mLightmapTexIndex);
			}

			gm.mExtraTexIndex				= -1;							 
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
																			 
			gm.mDiffuseTexOffsetTiling		= mc.mDiffuseTexOffsetTiling	 ;
			gm.mSpecularTexOffsetTiling		= mc.mSpecularTexOffsetTiling	 ;
			gm.mAmbientTexOffsetTiling		= mc.mAmbientTexOffsetTiling	 ;
			gm.mEmissiveTexOffsetTiling		= mc.mEmissiveTexOffsetTiling	 ;
			gm.mHeightTexOffsetTiling		= mc.mHeightTexOffsetTiling		 ;
			gm.mNormalsTexOffsetTiling		= mc.mNormalsTexOffsetTiling	 ;
			gm.mShininessTexOffsetTiling	= mc.mShininessTexOffsetTiling	 ;
			gm.mOpacityTexOffsetTiling		= mc.mOpacityTexOffsetTiling	 ;
			gm.mDisplacementTexOffsetTiling	= mc.mDisplacementTexOffsetTiling;
			gm.mReflectionTexOffsetTiling	= mc.mReflectionTexOffsetTiling	 ;
			gm.mLightmapTexOffsetTiling		= mc.mLightmapTexOffsetTiling	 ;
			gm.mExtraTexOffsetTiling		= mc.mExtraTexOffsetTiling		 ;
		}

		std::vector<avk::image_sampler> imageSamplers;
		const auto numSamplers = texNamesToUsages.size() + (whiteTexUsages.empty() ? 0 : 1) + (straightUpNormalTexUsages.empty() ? 0 : 1);
		imageSamplers.reserve(numSamplers);

		auto getSync = [numSamplers, &aSyncHandler, lSyncCount = size_t{0}] () mutable -> avk::sync {
			++lSyncCount;
			if (lSyncCount < numSamplers) {
				return avk::sync::auxiliary_with_barriers(aSyncHandler, avk::sync::steal_before_handler_on_demand, {}); // Invoke external sync exactly once (if there is something to sync)
			}
			assert (lSyncCount == numSamplers);
			return std::move(aSyncHandler); // For the last image, pass the main sync => this will also have the after-handler invoked.
		};

		// Create the white texture and assign its index to all usages
		if (!whiteTexUsages.empty()) {
			imageSamplers.push_back(
				context().create_image_sampler(
					owned(context().create_image_view(
						owned(create_1px_texture({ 255, 255, 255, 255 }, vk::Format::eR8G8B8A8Unorm, avk::memory_usage::device, aImageUsage, getSync()))
					)),
					owned(context().create_sampler(avk::filter_mode::nearest_neighbor, avk::border_handling_mode::repeat))
				)
			);
			int index = static_cast<int>(imageSamplers.size() - 1);
			for (auto* img : whiteTexUsages) {
				*img = index;
			}
		}

		// Create the normal texture, containing a normal pointing straight up, and assign to all usages
		if (!straightUpNormalTexUsages.empty()) {
			imageSamplers.push_back(
				context().create_image_sampler(
					owned(context().create_image_view(
						owned(create_1px_texture({ 127, 127, 255, 0 }, vk::Format::eR8G8B8A8Unorm, avk::memory_usage::device, aImageUsage, getSync()))
					)),
					owned(context().create_sampler(avk::filter_mode::nearest_neighbor, avk::border_handling_mode::repeat))
				)
			);
			int index = static_cast<int>(imageSamplers.size() - 1);
			for (auto* img : straightUpNormalTexUsages) {
				*img = index;
			}
		}

		// Load all the images from file, and assign them to all usages
		for (auto& pair : texNamesToUsages) {
			assert (!pair.first.empty());

			bool potentiallySrgb = srgbTextures.contains(pair.first);
			
			imageSamplers.push_back(
				context().create_image_sampler(
					owned(context().create_image_view(
						owned(create_image_from_file(pair.first, true, potentiallySrgb, aFlipTextures, 4, avk::memory_usage::device, aImageUsage, getSync()))
					)),
					owned(context().create_sampler(aTextureFilterMode, aBorderHandlingMode))
				)
			);
			int index = static_cast<int>(imageSamplers.size() - 1);
			for (auto* img : pair.second) {
				*img = index;
			}
		}

		// Hand over ownership to the caller
		return std::make_tuple(std::move(gpuMaterial), std::move(imageSamplers));
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
	
	std::tuple<avk::buffer, avk::buffer> create_vertex_and_index_buffers(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, vk::BufferUsageFlags aUsageFlags, avk::sync aSyncHandler)
	{
		auto [positionsData, indicesData] = get_vertices_and_indices(aModelsAndSelectedMeshes);

		auto positionsBuffer = context().create_buffer(
			avk::memory_usage::device, aUsageFlags,
			avk::vertex_buffer_meta::create_from_data(positionsData)
				.describe_only_member(positionsData[0], avk::content_description::position)
		);
		positionsBuffer->fill(positionsData.data(), 0, avk::sync::auxiliary_with_barriers(aSyncHandler, avk::sync::steal_before_handler_on_demand, {}));
		// It is fine to let positionsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		auto indexBuffer = context().create_buffer(
			avk::memory_usage::device, aUsageFlags,
			avk::index_buffer_meta::create_from_data(indicesData)
		);
		indexBuffer->fill(indicesData.data(), 0, std::move(aSyncHandler));
		// It is fine to let indicesData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return std::make_tuple(std::move(positionsBuffer), std::move(indexBuffer));
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
	
	avk::buffer create_normals_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler)
	{
		auto normalsData = get_normals(aModelsAndSelectedMeshes);
		
		auto normalsBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(normalsData)
		);
		normalsBuffer->fill(normalsData.data(), 0, std::move(aSyncHandler));
		// It is fine to let normalsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.
		
		return normalsBuffer;
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
	
	avk::buffer create_tangents_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler)
	{
		auto tangentsData = get_tangents(aModelsAndSelectedMeshes);
		
		auto tangentsBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(tangentsData)
		);
		tangentsBuffer->fill(tangentsData.data(), 0, std::move(aSyncHandler));
		// It is fine to let tangentsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return tangentsBuffer;
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
	
	avk::buffer create_bitangents_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler)
	{
		auto bitangentsData = get_bitangents(aModelsAndSelectedMeshes);
		
		auto bitangentsBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(bitangentsData)
		);
		bitangentsBuffer->fill(bitangentsData.data(), 0, std::move(aSyncHandler));
		// It is fine to let bitangentsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return bitangentsBuffer;
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
	
	avk::buffer create_colors_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aColorsSet, avk::sync aSyncHandler)
	{
		auto colorsData = get_colors(aModelsAndSelectedMeshes, aColorsSet);
		
		auto colorsBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(colorsData)
		);
		colorsBuffer->fill(colorsData.data(), 0, std::move(aSyncHandler));
		// It is fine to let colorsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return colorsBuffer;
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
	
	avk::buffer create_bone_weights_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler) {
		return create_bone_weights_buffer(aModelsAndSelectedMeshes, false, std::move(aSyncHandler));
	}

	avk::buffer create_bone_weights_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, bool aNormalizeBoneWeights, avk::sync aSyncHandler)
	{
		auto boneWeightsData = get_bone_weights(aModelsAndSelectedMeshes, aNormalizeBoneWeights);
		
		auto boneWeightsBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(boneWeightsData)
		);
		boneWeightsBuffer->fill(boneWeightsData.data(), 0, std::move(aSyncHandler));
		// It is fine to let boneWeightsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return boneWeightsBuffer;
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
	
	avk::buffer create_bone_indices_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aBoneIndexOffset, avk::sync aSyncHandler)
	{
		auto boneIndicesData = get_bone_indices(aModelsAndSelectedMeshes, aBoneIndexOffset);
		
		auto boneIndicesBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(boneIndicesData)
		);
		boneIndicesBuffer->fill(boneIndicesData.data(), 0, std::move(aSyncHandler));
		// It is fine to let boneIndicesData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return boneIndicesBuffer;
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
	
	avk::buffer create_bone_indices_for_single_target_buffer_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aInitialBoneIndexOffset, avk::sync aSyncHandler)
	{
		auto boneIndicesData = get_bone_indices_for_single_target_buffer(aModelsAndSelectedMeshes, aInitialBoneIndexOffset);
		
		auto boneIndicesBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(boneIndicesData)
		);
		boneIndicesBuffer->fill(boneIndicesData.data(), 0, std::move(aSyncHandler));
		// It is fine to let boneIndicesData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return boneIndicesBuffer;
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
	
	avk::buffer create_bone_indices_for_single_target_buffer_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, const std::vector<mesh_index_t>& aReferenceMeshIndices, avk::sync aSyncHandler)
	{
		auto boneIndicesData = get_bone_indices_for_single_target_buffer(aModelsAndSelectedMeshes, aReferenceMeshIndices);
		
		auto boneIndicesBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(boneIndicesData)
		);
		boneIndicesBuffer->fill(boneIndicesData.data(), 0, std::move(aSyncHandler));
		// It is fine to let boneIndicesData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return boneIndicesBuffer;
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
	
	avk::buffer create_2d_texture_coordinates_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet, avk::sync aSyncHandler)
	{
		auto texCoordsData = get_2d_texture_coordinates(aModelsAndSelectedMeshes, aTexCoordSet);
		
		auto texCoordsBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(texCoordsData)
		);
		texCoordsBuffer->fill(texCoordsData.data(), 0, std::move(aSyncHandler));
		// It is fine to let texCoordsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return texCoordsBuffer;
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
	
	avk::buffer create_2d_texture_coordinates_flipped_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet, avk::sync aSyncHandler)
	{
		auto texCoordsData = get_2d_texture_coordinates_flipped(aModelsAndSelectedMeshes, aTexCoordSet);
		
		auto texCoordsBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(texCoordsData)
		);
		texCoordsBuffer->fill(texCoordsData.data(), 0, std::move(aSyncHandler));
		// It is fine to let texCoordsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return texCoordsBuffer;
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
	
	avk::buffer create_3d_texture_coordinates_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet, avk::sync aSyncHandler)
	{
		auto texCoordsData = get_3d_texture_coordinates(aModelsAndSelectedMeshes, aTexCoordSet);
		
		auto texCoordsBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(texCoordsData)
		);
		texCoordsBuffer->fill(texCoordsData.data(), 0, std::move(aSyncHandler));
		// It is fine to let texCoordsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return texCoordsBuffer;
	}

}