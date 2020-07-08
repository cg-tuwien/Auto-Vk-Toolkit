#include <exekutor.hpp>

namespace xk
{

	std::tuple<std::vector<material_gpu_data>, std::vector<image_sampler>> convert_for_gpu_usage(
		std::vector<xk::material_config> aMaterialConfigs, 
		xv::image_usage aImageUsage,
		xk::filter_mode aTextureFilterMode, 
		xk::border_handling_mode aBorderHandlingMode,
		sync aSyncHandler)
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
				auto path = xk::clean_up_path(mc.mDiffuseTex);
				texNamesToUsages[path].push_back(&gm.mDiffuseTexIndex);
				if (settings::gLoadImagesInSrgbFormatByDefault) {
					srgbTextures.insert(path);
				}
			}

			gm.mSpecularTexIndex			= -1;							 
			if (mc.mSpecularTex.empty()) {
				whiteTexUsages.push_back(&gm.mSpecularTexIndex);
			}
			else {
				texNamesToUsages[xk::clean_up_path(mc.mSpecularTex)].push_back(&gm.mSpecularTexIndex);
			}

			gm.mAmbientTexIndex				= -1;							 
			if (mc.mAmbientTex.empty()) {
				whiteTexUsages.push_back(&gm.mAmbientTexIndex);
			}
			else {
				auto path = xk::clean_up_path(mc.mAmbientTex);
				texNamesToUsages[path].push_back(&gm.mAmbientTexIndex);
				if (settings::gLoadImagesInSrgbFormatByDefault) {
					srgbTextures.insert(path);
				}
			}

			gm.mEmissiveTexIndex			= -1;							 
			if (mc.mEmissiveTex.empty()) {
				whiteTexUsages.push_back(&gm.mEmissiveTexIndex);
			}
			else {
				texNamesToUsages[xk::clean_up_path(mc.mEmissiveTex)].push_back(&gm.mEmissiveTexIndex);
			}

			gm.mHeightTexIndex				= -1;							 
			if (mc.mHeightTex.empty()) {
				whiteTexUsages.push_back(&gm.mHeightTexIndex);
			}
			else {
				texNamesToUsages[xk::clean_up_path(mc.mHeightTex)].push_back(&gm.mHeightTexIndex);
			}

			gm.mNormalsTexIndex				= -1;							 
			if (mc.mNormalsTex.empty()) {
				straightUpNormalTexUsages.push_back(&gm.mNormalsTexIndex);
			}
			else {
				texNamesToUsages[xk::clean_up_path(mc.mNormalsTex)].push_back(&gm.mNormalsTexIndex);
			}

			gm.mShininessTexIndex			= -1;							 
			if (mc.mShininessTex.empty()) {
				whiteTexUsages.push_back(&gm.mShininessTexIndex);
			}
			else {
				texNamesToUsages[xk::clean_up_path(mc.mShininessTex)].push_back(&gm.mShininessTexIndex);
			}

			gm.mOpacityTexIndex				= -1;							 
			if (mc.mOpacityTex.empty()) {
				whiteTexUsages.push_back(&gm.mOpacityTexIndex);
			}
			else {
				texNamesToUsages[xk::clean_up_path(mc.mOpacityTex)].push_back(&gm.mOpacityTexIndex);
			}

			gm.mDisplacementTexIndex		= -1;							 
			if (mc.mDisplacementTex.empty()) {
				whiteTexUsages.push_back(&gm.mDisplacementTexIndex);
			}
			else {
				texNamesToUsages[xk::clean_up_path(mc.mDisplacementTex)].push_back(&gm.mDisplacementTexIndex);
			}

			gm.mReflectionTexIndex			= -1;							 
			if (mc.mReflectionTex.empty()) {
				whiteTexUsages.push_back(&gm.mReflectionTexIndex);
			}
			else {
				texNamesToUsages[xk::clean_up_path(mc.mReflectionTex)].push_back(&gm.mReflectionTexIndex);
			}

			gm.mLightmapTexIndex			= -1;							 
			if (mc.mLightmapTex.empty()) {
				whiteTexUsages.push_back(&gm.mLightmapTexIndex);
			}
			else {
				texNamesToUsages[xk::clean_up_path(mc.mLightmapTex)].push_back(&gm.mLightmapTexIndex);
			}

			gm.mExtraTexIndex				= -1;							 
			if (mc.mExtraTex.empty()) {
				whiteTexUsages.push_back(&gm.mExtraTexIndex);
			}
			else {
				texNamesToUsages[xk::clean_up_path(mc.mExtraTex)].push_back(&gm.mExtraTexIndex);
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

		std::vector<image_sampler> imageSamplers;
		const auto numSamplers = texNamesToUsages.size() + (whiteTexUsages.empty() ? 0 : 1) + (straightUpNormalTexUsages.empty() ? 0 : 1);
		imageSamplers.reserve(numSamplers);

		aSyncHandler.set_queue_hint(xk::context().transfer_queue());
		
		auto getSync = [numSamplers, &aSyncHandler, lSyncCount = size_t{0}] () mutable -> sync {
			++lSyncCount;
			if (lSyncCount < numSamplers) {
				return sync::auxiliary_with_barriers(aSyncHandler, sync::steal_before_handler_on_demand, {}); // Invoke external sync exactly once (if there is something to sync)
			}
			assert (lSyncCount == numSamplers);
			return std::move(aSyncHandler); // For the last image, pass the main sync => this will also have the after-handler invoked.
		};

		// Create the white texture and assign its index to all usages
		if (!whiteTexUsages.empty()) {
			imageSamplers.push_back(
				image_sampler_t::create(
					image_view_t::create(
						create_1px_texture({ 255, 255, 255, 255 }, xv::memory_usage::device, aImageUsage, getSync())
					),
					sampler_t::create(filter_mode::nearest_neighbor, border_handling_mode::repeat)
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
				image_sampler_t::create(
					image_view_t::create(
						create_1px_texture({ 127, 127, 255, 0 }, xv::memory_usage::device, aImageUsage, getSync())
					),
					sampler_t::create(filter_mode::nearest_neighbor, border_handling_mode::repeat)
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
				image_sampler_t::create(
					image_view_t::create(create_image_from_file(pair.first, true, potentiallySrgb, 4, xv::memory_usage::device, aImageUsage, getSync())),
					sampler_t::create(aTextureFilterMode, aBorderHandlingMode)
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

	std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>> get_vertices_and_indices(const std::vector<std::tuple<std::reference_wrapper<const model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> positionsData;
		std::vector<uint32_t> indicesData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<std::reference_wrapper<const model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				xk::append_indices_and_vertex_data(
					xk::additional_index_data(	indicesData,	[&]() { return modelRef.get().indices_for_mesh<uint32_t>(meshIndex);	} ),
					xk::additional_vertex_data(positionsData,	[&]() { return modelRef.get().positions_for_mesh(meshIndex);			} )
				);
			}
		}

		return std::make_tuple( std::move(positionsData), std::move(indicesData) );
	}
	
	std::tuple<vertex_buffer, index_buffer> create_vertex_and_index_buffers(const std::vector<std::tuple<std::reference_wrapper<const model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, sync aSyncHandler)
	{
		aSyncHandler.set_queue_hint(xk::context().transfer_queue());
		auto [positionsData, indicesData] = get_vertices_and_indices(std::move(aModelsAndSelectedMeshes));

		vk::BufferUsageFlags usageFlags{};
		if (xk::settings::gEnableBufferDeviceAddress) {
			usageFlags = vk::BufferUsageFlagBits::eShaderDeviceAddressKHR; // TODO: Abstract this better/in a different way! Global setting affecting ALL buffers can't be the right way.
		}
		
		vertex_buffer positionsBuffer = xk::create_and_fill(
			xk::vertex_buffer_meta::create_from_data(positionsData)
				.describe_only_member(positionsData[0], 0, content_description::position),
			xv::memory_usage::device,
			positionsData.data(),
			sync::auxiliary_with_barriers(aSyncHandler, sync::steal_before_handler_on_demand, {})
			// It is fine to let positionsData go out of scope, since its data has been copied to a
			// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.
			, usageFlags
		);

		index_buffer indexBuffer = xk::create_and_fill(
			xk::index_buffer_meta::create_from_data(indicesData),
			xv::memory_usage::device,
			indicesData.data(),
			std::move(aSyncHandler)
			// It is fine to let indicesData go out of scope, since its data has been copied to a
			// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.
			, usageFlags
		);

		return std::make_tuple(std::move(positionsBuffer), std::move(indexBuffer));
	}

	std::vector<glm::vec3> get_normals(const std::vector<std::tuple<std::reference_wrapper<const model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> normalsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<std::reference_wrapper<const model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				insert_into(normalsData, modelRef.get().normals_for_mesh(meshIndex));
			}
		}

		return normalsData;
	}
	
	vertex_buffer create_normals_buffer(const std::vector<std::tuple<std::reference_wrapper<const model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, sync aSyncHandler)
	{
		aSyncHandler.set_queue_hint(xk::context().transfer_queue());
		auto normalsData = get_normals(std::move(aModelsAndSelectedMeshes));
		
		vertex_buffer normalsBuffer = xk::create_and_fill(
			xk::vertex_buffer_meta::create_from_data(normalsData),
			xv::memory_usage::device,
			normalsData.data(),
			std::move(aSyncHandler)
			// It is fine to let normalsData go out of scope, since its data has been copied to a
			// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.
		);
		
		return normalsBuffer;
	}

	std::vector<glm::vec3> get_tangents(const std::vector<std::tuple<std::reference_wrapper<const model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> tangentsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<std::reference_wrapper<const model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				insert_into(tangentsData, modelRef.get().tangents_for_mesh(meshIndex));
			}
		}

		return tangentsData;
	}
	
	vertex_buffer create_tangents_buffer(const std::vector<std::tuple<std::reference_wrapper<const model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, sync aSyncHandler)
	{
		aSyncHandler.set_queue_hint(xk::context().transfer_queue());
		auto tangentsData = get_tangents(std::move(aModelsAndSelectedMeshes));
		
		vertex_buffer tangentsBuffer = xk::create_and_fill(
			xk::vertex_buffer_meta::create_from_data(tangentsData),
			xv::memory_usage::device,
			tangentsData.data(),
			std::move(aSyncHandler)
			// It is fine to let tangentsData go out of scope, since its data has been copied to a
			// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.
		);

		return tangentsBuffer;
	}

	std::vector<glm::vec3> get_bitangents(const std::vector<std::tuple<std::reference_wrapper<const model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> bitangentsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<std::reference_wrapper<const model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				insert_into(bitangentsData, modelRef.get().bitangents_for_mesh(meshIndex));
			}
		}

		return bitangentsData;
	}
	
	vertex_buffer create_bitangents_buffer(const std::vector<std::tuple<std::reference_wrapper<const model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, sync aSyncHandler)
	{
		aSyncHandler.set_queue_hint(xk::context().transfer_queue());
		auto bitangentsData = get_bitangents(std::move(aModelsAndSelectedMeshes));
		
		vertex_buffer bitangentsBuffer = xk::create_and_fill(
			xk::vertex_buffer_meta::create_from_data(bitangentsData),
			xv::memory_usage::device,
			bitangentsData.data(),
			std::move(aSyncHandler)
			// It is fine to let bitangentsData go out of scope, since its data has been copied to a
			// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.
		);

		return bitangentsBuffer;
	}

	std::vector<glm::vec4> get_colors(const std::vector<std::tuple<std::reference_wrapper<const model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, int _ColorsSet)
	{
		std::vector<glm::vec4> colorsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<std::reference_wrapper<const model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				insert_into(colorsData, modelRef.get().colors_for_mesh(meshIndex, _ColorsSet));
			}
		}

		return colorsData;
	}
	
	vertex_buffer create_colors_buffer(const std::vector<std::tuple<std::reference_wrapper<const model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, int _ColorsSet, sync aSyncHandler)
	{
		aSyncHandler.set_queue_hint(xk::context().transfer_queue());
		auto colorsData = get_colors(std::move(aModelsAndSelectedMeshes), _ColorsSet);
		
		vertex_buffer colorsBuffer = xk::create_and_fill(
			xk::vertex_buffer_meta::create_from_data(colorsData),
			xv::memory_usage::device,
			colorsData.data(),
			std::move(aSyncHandler)
			// It is fine to let colorsData go out of scope, since its data has been copied to a
			// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.
		);

		return colorsBuffer;
	}

	std::vector<glm::vec2> get_2d_texture_coordinates(const std::vector<std::tuple<std::reference_wrapper<const model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet)
	{
		std::vector<glm::vec2> texCoordsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<std::reference_wrapper<const model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				insert_into(texCoordsData, modelRef.get().texture_coordinates_for_mesh<glm::vec2>(meshIndex, aTexCoordSet));
			}
		}

		return texCoordsData;
	}
	
	vertex_buffer create_2d_texture_coordinates_buffer(const std::vector<std::tuple<std::reference_wrapper<const model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet, sync aSyncHandler)
	{
		aSyncHandler.set_queue_hint(xk::context().transfer_queue());
		auto texCoordsData = get_2d_texture_coordinates(std::move(aModelsAndSelectedMeshes), aTexCoordSet);
		
		vertex_buffer texCoordsBuffer = xk::create_and_fill(
			xk::vertex_buffer_meta::create_from_data(texCoordsData),
			xv::memory_usage::device,
			texCoordsData.data(),
			std::move(aSyncHandler)
			// It is fine to let texCoordsData go out of scope, since its data has been copied to a
			// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.
		);

		return texCoordsBuffer;
	}

	std::vector<glm::vec3> get_3d_texture_coordinates(const std::vector<std::tuple<std::reference_wrapper<const model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet)
	{
		std::vector<glm::vec3> texCoordsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<std::reference_wrapper<const model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				insert_into(texCoordsData, modelRef.get().texture_coordinates_for_mesh<glm::vec3>(meshIndex, aTexCoordSet));
			}
		}

		return texCoordsData;
	}
	
	vertex_buffer create_3d_texture_coordinates_buffer(const std::vector<std::tuple<std::reference_wrapper<const model_t>, std::vector<size_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet, sync aSyncHandler)
	{
		aSyncHandler.set_queue_hint(xk::context().transfer_queue());
		auto texCoordsData = get_3d_texture_coordinates(std::move(aModelsAndSelectedMeshes), aTexCoordSet);
		
		vertex_buffer texCoordsBuffer = xk::create_and_fill(
			xk::vertex_buffer_meta::create_from_data(texCoordsData),
			xv::memory_usage::device,
			texCoordsData.data(),
			std::move(aSyncHandler)
			// It is fine to let texCoordsData go out of scope, since its data has been copied to a
			// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.
		);

		return texCoordsBuffer;
	}

}