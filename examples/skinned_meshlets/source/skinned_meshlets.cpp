#include <gvk.hpp>
#include <imgui.h>
#include "../shaders/cpu_gpu_shared_config.h"

static constexpr size_t sNumVertices = 64;
static constexpr size_t sNumIndices = 378;
static constexpr uint32_t cConcurrecntFrames = 3u;

class skinned_meshlets_app : public gvk::invokee
{
	struct alignas(16) push_constants
	{
		VkBool32 mHighlightMeshlets;
	};

	struct data_for_draw_call
	{
		avk::buffer mPositionsBuffer;
		avk::buffer mTexCoordsBuffer;
		avk::buffer mNormalsBuffer;
		avk::buffer mIndexBuffer;
		avk::buffer mBoneIndicesBuffer;
		avk::buffer mBoneWeightsBuffer;
#if !USE_DIRECT_MESHLET
		avk::buffer mMeshletDataBuffer;
#endif

		glm::mat4 mModelMatrix;

		uint32_t mMaterialIndex;
		uint32_t mModelIndex;
	};

	struct loaded_data_for_draw_call
	{
		std::vector<glm::vec3> mPositions;
		std::vector<glm::vec2> mTexCoords;
		std::vector<glm::vec3> mNormals;
		std::vector<uint32_t> mIndices;
		std::vector<glm::uvec4> mBoneIndices;
		std::vector<glm::vec4> mBoneWeights;
#if !USE_DIRECT_MESHLET
		std::vector<uint32_t> mMeshletData;
#endif

		glm::mat4 mModelMatrix;

		uint32_t mMaterialIndex;
		uint32_t mModelIndex;
	};

	struct nav_path_element
	{
		uint32_t mNodeIndex;
		vk::Bool32 mApplyInverse;
		vk::Bool32 mExtendBounds;
		vk::Bool32 mApplyParentTransform;
	};

	struct additional_animated_model_data
	{
		std::vector<glm::mat4> mBoneMatricesAni;
	};

	struct animated_model_data
	{
		animated_model_data() = default;
		animated_model_data(animated_model_data&&) = default;
		animated_model_data& operator=(animated_model_data&&) = default;
		~animated_model_data() = default;
		// prevent copying of the data:
		animated_model_data(const animated_model_data&) = delete;
		animated_model_data& operator=(const animated_model_data&) = delete;

		std::string mModelName;
		gvk::animation_clip_data mClip;
		uint32_t mNumBoneMatrices;
		size_t mBoneMatricesBufferIndex;
		gvk::animation mAnimation;

		// Contains vectors of the following elements:
		//  [0]: node index
		//  [1]: (bool) is inversed direction
		//  [2]: (bool) should extend bounds
		//  [3]: (bool) should apply parent transformation
		std::vector<nav_path_element> mNavPaths;
		std::optional<avk::buffer> mNavPathBuffer;

		// Contains the following elements:
		//  [0]: path index (pointing into mNavPaths)
		//  [1]-[3]: padding
		std::vector<std::vector<glm::ivec4>> mNavPathsPerOriginBone;
		std::optional<std::vector<avk::buffer>> mNavPathsPerOriginBoneBuffers;


		double start_sec() const { return mClip.mStartTicks / mClip.mTicksPerSecond; }
		double end_sec() const { return mClip.mEndTicks / mClip.mTicksPerSecond; }
		double duration_sec() const { return end_sec() - start_sec(); }
		double duration_ticks() const { return mClip.mEndTicks - mClip.mStartTicks; }
	};

	struct alignas(16) meshlet
	{
		glm::mat4 mTransformationMatrix;
		uint32_t mMaterialIndex;
		uint32_t mTexelBufferIndex;
		uint32_t mModelIndex;

#if USE_DIRECT_MESHLET
		gvk::meshlet_gpu_data<sNumVertices, sNumIndices> mGeometry;
#else
		gvk::meshlet_redirected_gpu_data mGeometry;
#endif
	};

public: // v== avk::invokee overrides which will be invoked by the framework ==v
	skinned_meshlets_app(avk::queue& aQueue)
		: mQueue{ &aQueue }
	{}

	void initialize() override
	{
		// use helper functions to create ImGui elements
		auto surfaceCap = gvk::context().physical_device().getSurfaceCapabilitiesKHR(gvk::context().main_window()->surface());

		// Create a descriptor cache that helps us to conveniently create descriptor sets:
		mDescriptorCache = gvk::context().create_descriptor_cache();

		glm::mat4 globalTransform = glm::scale(glm::vec3(0.01f));
		std::vector<gvk::model> loadedModels;
		// Load a model from file:
		auto model = gvk::model_t::load_from_file("assets/pigman.fbx", aiProcess_Triangulate);

		loadedModels.push_back(std::move(model));


		std::vector<gvk::material_config> allMatConfigs; // <-- Gather the material config from all models to be loaded
		std::vector<std::vector<glm::mat4>> meshRootMatricesPerModel;
		std::vector<std::vector<glm::mat4>> rMeshRootMatricesPerModel;
		std::vector<std::vector<glm::mat4>> rBindPoseMatricesMeshSpacePerModel;
		std::vector<std::vector<glm::mat4>> rInverseBindPoseMatricesMeshSpacePerModel;
		std::vector<loaded_data_for_draw_call> dataForDrawCall;
		std::vector<meshlet> meshletsGeometry;
		std::vector<animated_model_data> rAnimatedModels;


		for (size_t i = 0; i < loadedModels.size(); ++i) {
			auto curModel = std::move(loadedModels[i]);

			auto curClip = curModel->load_animation_clip(0, 0, 100);
			curClip.mTicksPerSecond = 30;
			auto& curEntry = rAnimatedModels.emplace_back();
			curEntry.mModelName = curModel->path();
			curEntry.mClip = std::move(curClip);

			// get all the meshlet indices of the model
			const auto meshIndicesInOrder = curModel->select_all_meshes();

			curEntry.mNumBoneMatrices = curModel->num_bone_matrices(meshIndicesInOrder);

			// Store offset into the vector of buffers that store the bone matrices
			curEntry.mBoneMatricesBufferIndex = i;

			auto& mrms = rMeshRootMatricesPerModel.emplace_back();
			for (auto mi : meshIndicesInOrder) {
				mrms.push_back(curModel->mesh_root_matrix(mi));
			}

			auto& bpmsMS = rBindPoseMatricesMeshSpacePerModel.emplace_back();
			auto& ibpmsMS = rInverseBindPoseMatricesMeshSpacePerModel.emplace_back();
			bpmsMS.reserve(curModel->num_bone_matrices(meshIndicesInOrder));
			for (auto mi : meshIndicesInOrder) {
				// Insert all the "bind pose matrices" (i.e. the INVERTED "inverse bind pose matrices"!), also store all the "inverse bind pose matrices".
				auto inverseBindPoseMatrices = curModel->inverse_bind_pose_matrices(mi, gvk::bone_matrices_space::mesh_space);
				std::transform(
					std::begin(inverseBindPoseMatrices), std::end(inverseBindPoseMatrices),
					std::back_inserter(bpmsMS), [](const glm::mat4& m) { return glm::inverse(m); }
				);
				ibpmsMS = std::move(inverseBindPoseMatrices);
			}


			auto distinctMaterials = curModel->distinct_material_configs();
			const auto matOffset = allMatConfigs.size();
			// add all the materials of the model
			for (auto& pair : distinctMaterials) {
				allMatConfigs.push_back(pair.first);
			}

			curEntry.mAnimation = curModel->prepare_animation(curEntry.mClip.mAnimationIndex, meshIndicesInOrder);

			// We'll need some storage to hold bone matrices further down, in the inner loop:
			std::vector<glm::mat4> spaceForBoneMatrices; // Provides storage for the bone matrices
			spaceForBoneMatrices.resize(curEntry.mNumBoneMatrices); // Make sure it has enough space for all the matrices
			std::vector<glm::mat4> inverseBindPoseMatrices; // Stores the inverse bind pose matrices, i.e. to transform mesh input data into bind pose
			std::vector<glm::mat4> inverseMeshRootMatrices;
			std::vector<glm::mat4> intoBoneSpaceMatrices;
			std::vector<uint32_t> boneMatToAniNode; // Stores for each bone matrix which animation_node it was that wrote it (into spaceForBoneMatrices)
			spaceForBoneMatrices.resize(curEntry.mNumBoneMatrices); // Make sure it has enough space for all the matrices
			inverseBindPoseMatrices.resize(curEntry.mNumBoneMatrices); // Make sure it has enough space for all the matrices
			inverseMeshRootMatrices.resize(curEntry.mNumBoneMatrices);
			intoBoneSpaceMatrices.resize(curEntry.mNumBoneMatrices);
			boneMatToAniNode.resize(curEntry.mNumBoneMatrices); // Make sure it has enough space for all the matrices

			curEntry.mAnimation.animate(curEntry.mClip, curEntry.mClip.start_time(), [&spaceForBoneMatrices, &inverseBindPoseMatrices, &boneMatToAniNode, &inverseMeshRootMatrices, &intoBoneSpaceMatrices](
				gvk::mesh_bone_info aInfo,
				const glm::mat4& aInverseMeshRootMatrix,
				const glm::mat4& aTransformMatrix,
				const glm::mat4& aInverseBindPoseMatrix,
				const glm::mat4& aLocalTransformMatrix,
				size_t aAnimatedNodeIndex,
				size_t aBoneMeshTargetIndex,
				double aAnimationTimeInTicks) {
					// Construction of the bone matrix for this node:
					//   1. Bring vertex into bone space
					//   2. Apply transformaton in bone space
					//   3. Convert transformed vertex back to mesh space

					size_t bmi = aInfo.mGlobalBoneIndexOffset + aInfo.mMeshLocalBoneIndex;
					spaceForBoneMatrices[bmi] = aTransformMatrix * aInverseBindPoseMatrix; // Store the bone matrix
					// Into PBS:
					spaceForBoneMatrices[bmi] = aInverseMeshRootMatrix * spaceForBoneMatrices[bmi];
					inverseBindPoseMatrices[bmi] = aInverseBindPoseMatrix; // Store the inverse bind pose matrix a.k.a. offset matrix
					boneMatToAniNode[bmi] = static_cast<uint32_t>(aAnimatedNodeIndex); // Store the animation node index this bone matrix is assigned to
																// (This means that this animation node is the last one in the bone hierarchy
																//  which is relevant for modifying the bone's position. Hence, from this
																//  animation_node, it is safe to write the bone matrix.)
					inverseMeshRootMatrices[bmi] = aInverseMeshRootMatrix;
					intoBoneSpaceMatrices[bmi] = glm::inverse(aInverseMeshRootMatrix * aTransformMatrix);
				}
			);

			auto boneOffsetSoFar = 0u;
			for (size_t mpos = 0; mpos < meshIndicesInOrder.size(); mpos++) {
				auto meshIndex = meshIndicesInOrder[mpos];
				std::string meshname = curModel->name_of_mesh(mpos);

				auto texelBufferIndex = dataForDrawCall.size();
				auto& drawCallData = dataForDrawCall.emplace_back();

				drawCallData.mMaterialIndex = static_cast<int32_t>(matOffset);
				drawCallData.mModelMatrix = globalTransform * curModel->transformation_matrix_for_mesh(meshIndex);
				drawCallData.mModelIndex = curEntry.mBoneMatricesBufferIndex;
				// Find and assign the correct material (in the ~"global" allMatConfigs vector!)
				for (auto pair : distinctMaterials) {
					if (std::end(pair.second) != std::find(std::begin(pair.second), std::end(pair.second), meshIndex)) {
						break;
					}

					drawCallData.mMaterialIndex++;
				}

				auto selection = gvk::make_models_and_meshes_selection(curModel, meshIndex);
				std::vector<gvk::mesh_index_t> meshIndices;
				meshIndices.push_back(meshIndex);
				// Build meshlets:
				std::tie(drawCallData.mPositions, drawCallData.mIndices) = gvk::get_vertices_and_indices(selection);
				drawCallData.mNormals = gvk::get_normals(selection);
				drawCallData.mTexCoords = gvk::get_2d_texture_coordinates(selection, 0);
				drawCallData.mBoneIndices = gvk::get_bone_indices_for_single_target_buffer(selection, meshIndicesInOrder);
				drawCallData.mBoneWeights = gvk::get_bone_weights(selection);

				// create selection for the meshlets
				std::vector<std::tuple<avk::resource_ownership<gvk::model_t>, std::vector<gvk::mesh_index_t>>> meshletSelection;
				meshletSelection.emplace_back(avk::shared(curModel), meshIndices);

				auto cpuMeshlets = gvk::divide_into_meshlets(meshletSelection);
#if USE_DIRECT_MESHLET
				gvk::serializer serializer("direct_meshlets-" + meshname + "-" + std::to_string(mpos) + ".cache");
				auto [gpuMeshlets, _] = gvk::convert_for_gpu_usage_cached<gvk::meshlet_gpu_data<sNumVertices, sNumIndices>, sNumVertices, sNumIndices>(serializer, cpuMeshlets);
#else
				gvk::serializer serializer("indirect_meshlets-" + meshname + "-" + std::to_string(mpos) + ".cache");
				auto [gpuMeshlets, generatedMeshletData] = gvk::convert_for_gpu_usage_cached<gvk::meshlet_redirected_gpu_data, sNumVertices, sNumIndices>(serializer, cpuMeshlets);
				drawCallData.mMeshletData = std::move(generatedMeshletData.value());
#endif

				serializer.flush();

				// fill our own meshlets with the loaded/generated data
				for (size_t mshltidx = 0; mshltidx < gpuMeshlets.size(); ++mshltidx) {
					auto& genMeshlet = gpuMeshlets[mshltidx];

					auto& ml = meshletsGeometry.emplace_back(meshlet{});

#pragma region start to assemble meshlet struct
					ml.mTransformationMatrix = drawCallData.mModelMatrix;
					ml.mMaterialIndex = drawCallData.mMaterialIndex;
					ml.mTexelBufferIndex = static_cast<uint32_t>(texelBufferIndex);
					ml.mModelIndex = drawCallData.mModelIndex;

					ml.mGeometry = genMeshlet;
#pragma endregion 
				}
			}
		} // end for (auto& loadInfo : toLoad)

		// create buffers for animation data
		for (int i = 0; i < loadedModels.size(); ++i) {
			auto& dude = mAnimatedModels.emplace_back(std::move(rAnimatedModels[i]), additional_animated_model_data{});

			std::get<additional_animated_model_data>(dude).mBoneMatricesAni.resize(std::get<animated_model_data>(dude).mNumBoneMatrices);
			for (size_t cfi = 0; cfi < cConcurrecntFrames; ++cfi) {
				mBoneMatricesBuffersAni[cfi].push_back(gvk::context().create_buffer(
					avk::memory_usage::host_coherent, {},
					avk::storage_buffer_meta::create_from_data(std::get<additional_animated_model_data>(dude).mBoneMatricesAni)
				));
			}
			assert(std::get<additional_animated_model_data>(dude).mBoneMatricesAni.size() == std::get<additional_animated_model_data>(dude).mBoneMatricesAabbs.size());
			assert(mBoneMatricesBuffersAni.size() == mBoneMatricesBuffersAabbs.size());

			auto& mrm = mMeshRootMatrices.emplace_back(gvk::context().create_buffer(
				avk::memory_usage::device, {},
				avk::storage_buffer_meta::create_from_data(rMeshRootMatricesPerModel[i])
			));
			mrm->fill(rMeshRootMatricesPerModel[i].data(), 0, avk::sync::wait_idle());

			auto& bpmtrxesMeshSpace = mBindPoseMatrices.emplace_back(gvk::context().create_buffer(
				avk::memory_usage::device, {},
				avk::storage_buffer_meta::create_from_data(rBindPoseMatricesMeshSpacePerModel[i])
			));
			bpmtrxesMeshSpace->fill(rBindPoseMatricesMeshSpacePerModel[i].data(), 0, avk::sync::wait_idle());
		}

		// lambda to create all the buffers for us
		auto addDrawCalls = [this](auto& dataForDrawCall, auto& drawCallsTarget) {
			for (auto& drawCallData : dataForDrawCall) {
				auto& drawCall = drawCallsTarget.emplace_back();
				drawCall.mModelMatrix = drawCallData.mModelMatrix;
				drawCall.mMaterialIndex = drawCallData.mMaterialIndex;

				drawCall.mPositionsBuffer = gvk::context().create_buffer(avk::memory_usage::device, {},
					avk::vertex_buffer_meta::create_from_data(drawCallData.mPositions).describe_only_member(drawCallData.mPositions[0], avk::content_description::position),
					avk::storage_buffer_meta::create_from_data(drawCallData.mPositions),
					avk::uniform_texel_buffer_meta::create_from_data(drawCallData.mPositions).describe_only_member(drawCallData.mPositions[0]) // just take the vec3 as it is
				);

				drawCall.mIndexBuffer = gvk::context().create_buffer(avk::memory_usage::device, {},
					avk::index_buffer_meta::create_from_data(drawCallData.mIndices),
					avk::storage_buffer_meta::create_from_data(drawCallData.mIndices),
					avk::uniform_texel_buffer_meta::create_from_data(drawCallData.mIndices).template set_format<glm::uvec3>() // Set a diferent format => combine 3 consecutive elements into one unit
				);

				drawCall.mNormalsBuffer = gvk::context().create_buffer(avk::memory_usage::device, {},
					avk::vertex_buffer_meta::create_from_data(drawCallData.mNormals),
					avk::storage_buffer_meta::create_from_data(drawCallData.mNormals),
					avk::uniform_texel_buffer_meta::create_from_data(drawCallData.mNormals).describe_only_member(drawCallData.mNormals[0]) // just take the vec3 as it is
				);

				drawCall.mTexCoordsBuffer = gvk::context().create_buffer(avk::memory_usage::device, {},
					avk::vertex_buffer_meta::create_from_data(drawCallData.mTexCoords),
					avk::storage_buffer_meta::create_from_data(drawCallData.mTexCoords),
					avk::uniform_texel_buffer_meta::create_from_data(drawCallData.mTexCoords).describe_only_member(drawCallData.mTexCoords[0]) // just take the vec2 as it is   
				);

#if !USE_DIRECT_MESHLET
				drawCall.mMeshletDataBuffer = gvk::context().create_buffer(avk::memory_usage::device, {},
					avk::vertex_buffer_meta::create_from_data(drawCallData.mMeshletData),
					avk::storage_buffer_meta::create_from_data(drawCallData.mMeshletData),
					avk::uniform_texel_buffer_meta::create_from_data(drawCallData.mMeshletData).describe_only_member(drawCallData.mMeshletData[0])
				);
#endif

				drawCall.mBoneIndicesBuffer = gvk::context().create_buffer(avk::memory_usage::device, {},
					avk::vertex_buffer_meta::create_from_data(drawCallData.mBoneIndices),
					avk::storage_buffer_meta::create_from_data(drawCallData.mBoneIndices),
					avk::uniform_texel_buffer_meta::create_from_data(drawCallData.mBoneIndices).describe_only_member(drawCallData.mBoneIndices[0]) // just take the uvec4 as it is 
				);

				drawCall.mBoneWeightsBuffer = gvk::context().create_buffer(avk::memory_usage::device, {},
					avk::vertex_buffer_meta::create_from_data(drawCallData.mBoneWeights),
					avk::storage_buffer_meta::create_from_data(drawCallData.mBoneWeights),
					avk::uniform_texel_buffer_meta::create_from_data(drawCallData.mBoneWeights).describe_only_member(drawCallData.mBoneWeights[0]) // just take the vec4 as it is 
				);

				drawCall.mPositionsBuffer->fill(drawCallData.mPositions.data(), 0, avk::sync::wait_idle(true));
				drawCall.mIndexBuffer->fill(drawCallData.mIndices.data(), 0, avk::sync::wait_idle(true));
				drawCall.mNormalsBuffer->fill(drawCallData.mNormals.data(), 0, avk::sync::wait_idle(true));
				drawCall.mTexCoordsBuffer->fill(drawCallData.mTexCoords.data(), 0, avk::sync::wait_idle(true));
#if !USE_DIRECT_MESHLET
				drawCall.mMeshletDataBuffer->fill(drawCallData.mMeshletData.data(), 0, avk::sync::wait_idle(true));
#endif
				drawCall.mBoneIndicesBuffer->fill(drawCallData.mBoneIndices.data(), 0, avk::sync::wait_idle(true));
				drawCall.mBoneWeightsBuffer->fill(drawCallData.mBoneWeights.data(), 0, avk::sync::wait_idle(true));

				// add them to the texel buffers
				mPositionBuffers.push_back(gvk::context().create_buffer_view(shared(drawCall.mPositionsBuffer)));
				mIndexBuffers.push_back(gvk::context().create_buffer_view(shared(drawCall.mIndexBuffer)));
				mNormalBuffers.push_back(gvk::context().create_buffer_view(shared(drawCall.mNormalsBuffer)));
				mTexCoordsBuffers.push_back(gvk::context().create_buffer_view(shared(drawCall.mTexCoordsBuffer)));
#if !USE_DIRECT_MESHLET
				mMeshletDataBuffers.push_back(gvk::context().create_buffer_view(shared(drawCall.mMeshletDataBuffer)));
#endif
				mBoneIndicesBuffers.push_back(gvk::context().create_buffer_view(shared(drawCall.mBoneIndicesBuffer)));
				mBoneWeightsBuffers.push_back(gvk::context().create_buffer_view(shared(drawCall.mBoneWeightsBuffer)));
			}
		};
		// create all the buffers for our drawcall data
		addDrawCalls(dataForDrawCall, mDrawCalls);

		// Put the meshlets that we have gathered into a buffer:
		mMeshletsBuffer = gvk::context().create_buffer(
			avk::memory_usage::device, {},
			avk::storage_buffer_meta::create_from_data(meshletsGeometry)
		);
		mMeshletsBuffer->fill(meshletsGeometry.data(), 0, avk::sync::wait_idle(true));
		mNumMeshletWorkgroups = meshletsGeometry.size();

		// For all the different materials, transfer them in structs which are well
		// suited for GPU-usage (proper alignment, and containing only the relevant data),
		// also load all the referenced images from file and provide access to them
		// via samplers; It all happens in `ak::convert_for_gpu_usage`:
		auto [gpuMaterials, imageSamplers] = gvk::convert_for_gpu_usage<gvk::material_gpu_data>(
			allMatConfigs, false, true,
			avk::image_usage::general_texture,
			avk::filter_mode::trilinear,
			avk::sync::with_barriers(gvk::context().main_window()->command_buffer_lifetime_handler())
			);

		mViewProjBuffer = gvk::context().create_buffer(
			avk::memory_usage::host_visible, {},
			avk::uniform_buffer_meta::create_from_data(glm::mat4())
		);

		mMaterialBuffer = gvk::context().create_buffer(
			avk::memory_usage::host_visible, {},
			avk::storage_buffer_meta::create_from_data(gpuMaterials)
		);
		mMaterialBuffer->fill(
			gpuMaterials.data(), 0,
			avk::sync::not_required()
		);

		mImageSamplers = std::move(imageSamplers);

		auto swapChainFormat = gvk::context().main_window()->swap_chain_image_format();
		// Create our rasterization graphics pipeline with the required configuration:
		mPipeline = gvk::context().create_graphics_pipeline_for(
			// Specify which shaders the pipeline consists of:
			avk::task_shader("shaders/meshlet.task"),
			avk::mesh_shader("shaders/meshlet.mesh"),
			avk::fragment_shader("shaders/diffuse_shading_fixed_lightsource.frag"),
			// Some further settings:
			avk::cfg::front_face::define_front_faces_to_be_counter_clockwise(),
			avk::cfg::viewport_depth_scissors_config::from_framebuffer(gvk::context().main_window()->backbuffer_at_index(0)),
			// We'll render to the back buffer, which has a color attachment always, and in our case additionally a depth
			// attachment, which has been configured when creating the window. See main() function!
			avk::attachment::declare(gvk::format_from_window_color_buffer(gvk::context().main_window()), avk::on_load::clear, avk::color(0), avk::on_store::store),	 // But not in presentable format, because ImGui comes after
			avk::attachment::declare(gvk::format_from_window_depth_buffer(gvk::context().main_window()), avk::on_load::clear, avk::depth_stencil(), avk::on_store::dont_care),
			// The following define additional data which we'll pass to the pipeline:
			avk::push_constant_binding_data{ avk::shader_type::fragment, 0, sizeof(push_constants) },
			avk::descriptor_binding(0, 0, mImageSamplers),
			avk::descriptor_binding(0, 1, mViewProjBuffer),
			avk::descriptor_binding(1, 0, mMaterialBuffer),
			avk::descriptor_binding(2, 0, mBoneMatricesBuffersAni[0]),
			avk::descriptor_binding(2, 1, mMeshRootMatrices),
			avk::descriptor_binding(2, 2, mBindPoseMatrices),
			// texel buffers
			avk::descriptor_binding(3, 0, avk::as_uniform_texel_buffer_views(mPositionBuffers)),
			avk::descriptor_binding(3, 1, avk::as_uniform_texel_buffer_views(mIndexBuffers)),
			avk::descriptor_binding(3, 2, avk::as_uniform_texel_buffer_views(mNormalBuffers)),
			avk::descriptor_binding(3, 3, avk::as_uniform_texel_buffer_views(mTexCoordsBuffers)),
#if !USE_DIRECT_MESHLET
			avk::descriptor_binding(3, 4, avk::as_uniform_texel_buffer_views(mMeshletDataBuffers)),
#endif
			avk::descriptor_binding(3, 5, avk::as_uniform_texel_buffer_views(mBoneIndicesBuffers)),
			avk::descriptor_binding(3, 6, avk::as_uniform_texel_buffer_views(mBoneWeightsBuffers)),
			avk::descriptor_binding(4, 0, mMeshletsBuffer)
		);
		// set up updater
		// we want to use an updater, so create one:

		mUpdater.emplace();
		mPipeline.enable_shared_ownership(); // Make it usable with the updater
		mUpdater->on(gvk::shader_files_changed_event(mPipeline)).update(mPipeline);
		mPipeline.enable_shared_ownership(); // Make it usable with the updater

		mUpdater->on(gvk::swapchain_resized_event(gvk::context().main_window())).invoke([this]() {
			this->mQuakeCam.set_aspect_ratio(gvk::context().main_window()->aspect_ratio());
			});

		//first make sure render pass is updated
		mUpdater->on(gvk::swapchain_format_changed_event(gvk::context().main_window()),
			gvk::swapchain_additional_attachments_changed_event(gvk::context().main_window())
		).invoke([this]() {
			std::vector<avk::attachment> renderpassAttachments = {
				avk::attachment::declare(gvk::format_from_window_color_buffer(gvk::context().main_window()), avk::on_load::clear, avk::color(0),		avk::on_store::store),	 // But not in presentable format, because ImGui comes after
			};
			auto renderPass = gvk::context().create_renderpass(renderpassAttachments);
			gvk::context().replace_render_pass_for_pipeline(mPipeline, std::move(renderPass));
			}).then_on( // ... next, at this point, we are sure that the render pass is correct -> check if there are events that would update the pipeline
				gvk::swapchain_changed_event(gvk::context().main_window()),
				gvk::shader_files_changed_event(mPipeline)
			).update(mPipeline);

			// Add the camera to the composition (and let it handle the updates)
			mQuakeCam.set_translation({ 0.0f, 1.0f, 5.0f });
			mQuakeCam.set_perspective_projection(glm::radians(60.0f), gvk::context().main_window()->aspect_ratio(), 0.3f, 1000.0f);
			//mQuakeCam.set_orthographic_projection(-5, 5, -5, 5, 0.5, 100);
			gvk::current_composition()->add_element(mQuakeCam);

			auto imguiManager = gvk::current_composition()->element_by_type<gvk::imgui_manager>();
			if (nullptr != imguiManager) {
				imguiManager->add_callback([this]() {
					bool isEnabled = this->is_enabled();
					ImGui::Begin("Info & Settings");
					ImGui::SetWindowPos(ImVec2(1.0f, 1.0f), ImGuiCond_FirstUseEver);
					ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
					ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
					ImGui::TextColored(ImVec4(0.f, .6f, .8f, 1.f), "[F1]: Toggle input-mode");
					ImGui::TextColored(ImVec4(0.f, .6f, .8f, 1.f), " (UI vs. scene navigation)");
					ImGui::Checkbox("Enable/Disable invokee", &isEnabled);
					if (isEnabled != this->is_enabled())
					{
						if (!isEnabled) this->disable();
						else this->enable();
					}

					ImGui::Checkbox("Highlight Meshlets", &mHighlightMeshlets);

					ImGui::End();
					});
			}
	}

	void render() override
	{
		auto mainWnd = gvk::context().main_window();
		auto ifi = mainWnd->current_in_flight_index();

		auto viewProjMat = mQuakeCam.projection_matrix() * mQuakeCam.view_matrix();
		mViewProjBuffer->fill(glm::value_ptr(viewProjMat), 0, avk::sync::not_required());

		auto pushConstants = push_constants{ mHighlightMeshlets };

		auto& commandPool = gvk::context().get_command_pool_for_single_use_command_buffers(*mQueue);
		auto cmdbfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		cmdbfr->begin_recording();
		cmdbfr->begin_render_pass_for_framebuffer(mPipeline->get_renderpass(), gvk::context().main_window()->current_backbuffer());
		cmdbfr->bind_pipeline(avk::const_referenced(mPipeline));
		cmdbfr->bind_descriptors(mPipeline->layout(), mDescriptorCache.get_or_create_descriptor_sets({
			avk::descriptor_binding(0, 0, mImageSamplers),
			avk::descriptor_binding(0, 1, mViewProjBuffer),
			avk::descriptor_binding(1, 0, mMaterialBuffer),
			avk::descriptor_binding(2, 0, mBoneMatricesBuffersAni[ifi]),
			avk::descriptor_binding(2, 1, mMeshRootMatrices),
			avk::descriptor_binding(2, 2, mBindPoseMatrices),
			avk::descriptor_binding(3, 0, avk::as_uniform_texel_buffer_views(mPositionBuffers)),
			avk::descriptor_binding(3, 1, avk::as_uniform_texel_buffer_views(mIndexBuffers)),
			avk::descriptor_binding(3, 2, avk::as_uniform_texel_buffer_views(mNormalBuffers)),
			avk::descriptor_binding(3, 3, avk::as_uniform_texel_buffer_views(mTexCoordsBuffers)),
#if !USE_DIRECT_MESHLET
					avk::descriptor_binding(3, 4, avk::as_uniform_texel_buffer_views(mMeshletDataBuffers)),
#endif
			avk::descriptor_binding(3, 5, avk::as_uniform_texel_buffer_views(mBoneIndicesBuffers)),
			avk::descriptor_binding(3, 6, avk::as_uniform_texel_buffer_views(mBoneWeightsBuffers)),
			avk::descriptor_binding(4, 0, mMeshletsBuffer)
			}));
		cmdbfr->handle().pushConstants(mPipeline->layout_handle(), vk::ShaderStageFlagBits::eFragment, 0u, sizeof(push_constants), &pushConstants);
		// draw our meshlets
		cmdbfr->handle().drawMeshTasksNV(mNumMeshletWorkgroups, 0, gvk::context().dynamic_dispatch());

		cmdbfr->end_render_pass();
		cmdbfr->end_recording();

		// The swap chain provides us with an "image available semaphore" for the current frame.
		// Only after the swapchain image has become available, we may start rendering into it.
		auto imageAvailableSemaphore = mainWnd->consume_current_image_available_semaphore();

		// Submit the draw call and take care of the command buffer's lifetime:
		mQueue->submit(cmdbfr, imageAvailableSemaphore);
		mainWnd->handle_lifetime(avk::owned(cmdbfr));
	}

	void update() override
	{
		if (gvk::input().key_pressed(gvk::key_code::c)) {
			// Center the cursor:
			auto resolution = gvk::context().main_window()->resolution();
			gvk::context().main_window()->set_cursor_pos({ resolution[0] / 2.0, resolution[1] / 2.0 });
		}
		if (gvk::input().key_pressed(gvk::key_code::escape)) {
			// Stop the current composition:
			gvk::current_composition()->stop();
		}
		if (gvk::input().key_pressed(gvk::key_code::left)) {
			mQuakeCam.look_along(gvk::left());
		}
		if (gvk::input().key_pressed(gvk::key_code::right)) {
			mQuakeCam.look_along(gvk::right());
		}
		if (gvk::input().key_pressed(gvk::key_code::up)) {
			mQuakeCam.look_along(gvk::front());
		}
		if (gvk::input().key_pressed(gvk::key_code::down)) {
			mQuakeCam.look_along(gvk::back());
		}
		if (gvk::input().key_pressed(gvk::key_code::page_up)) {
			mQuakeCam.look_along(gvk::up());
		}
		if (gvk::input().key_pressed(gvk::key_code::page_down)) {
			mQuakeCam.look_along(gvk::down());
		}
		if (gvk::input().key_pressed(gvk::key_code::home)) {
			mQuakeCam.look_at(glm::vec3{ 0.0f, 0.0f, 0.0f });
		}

		if (gvk::input().key_pressed(gvk::key_code::f1)) {
			auto imguiManager = gvk::current_composition()->element_by_type<gvk::imgui_manager>();
			if (mQuakeCam.is_enabled()) {
				mQuakeCam.disable();
				if (nullptr != imguiManager) { imguiManager->enable_user_interaction(true); }
			}
			else {
				mQuakeCam.enable();
				if (nullptr != imguiManager) { imguiManager->enable_user_interaction(false); }
			}
		}

		// Animate all the meshes
		for (auto& dude : mAnimatedModels) {
			const auto boneMatSpaceAni = gvk::bone_matrices_space::model_space;

			const auto boneMatSpaceAabb = gvk::bone_matrices_space::mesh_space;

			static auto customAniFu = [this](gvk::animation& aAnimation, const gvk::animation_clip_data& aClip, double aTime, gvk::bone_matrices_space aTargetSpace, glm::mat4* aTargetMemory) {
				switch (aTargetSpace) {
				case gvk::bone_matrices_space::mesh_space:
					// Use lambda option 1 that takes as parameters: mesh_bone_info, inverse mesh root matrix, global node/bone transform w.r.t. the animation, inverse bind-pose matrix
					aAnimation.animate(aClip, aTime, [this, &aAnimation, aTargetMemory](gvk::mesh_bone_info aInfo, const glm::mat4& aInverseMeshRootMatrix, const glm::mat4& aTransformMatrix, const glm::mat4& aInverseBindPoseMatrix, const glm::mat4& aLocalTransformMatrix, size_t aAnimatedNodeIndex, size_t aBoneMeshTargetIndex, double aAnimationTimeInTicks) {
						// Construction of the bone matrix for this node:
						//   1. Bring vertex into bone space
						//   2. Apply transformaton in bone space
						//   3. Convert transformed vertex back to mesh space

							// => overwrite all the data! :O

						gvk::animated_node& anode = aAnimation.get_animated_node_at(aAnimatedNodeIndex);
						auto newLocalTransform = aAnimation.compute_node_local_transform(anode, 0.0 /* TODO: <-- which time for "no animation"? */);

						// Calculate the node's global transform, using its local transform and the transforms of its parents:
						if (anode.mAnimatedParentIndex.has_value()) {
							anode.mGlobalTransform = aAnimation.get_animated_node_at(anode.mAnimatedParentIndex.value()).get().mGlobalTransform * anode.mParentTransform * newLocalTransform;
						}
						else {
							anode.mGlobalTransform = anode.mParentTransform * newLocalTransform;
						}

						aTargetMemory[aInfo.mGlobalBoneIndexOffset + aInfo.mMeshLocalBoneIndex] = aInverseMeshRootMatrix * aTransformMatrix * aInverseBindPoseMatrix;
						});
					break;
				case gvk::bone_matrices_space::model_space:
					// Use lambda option 1 that takes as parameters: mesh_bone_info, inverse mesh root matrix, global node/bone transform w.r.t. the animation, inverse bind-pose matrix
					aAnimation.animate(aClip, aTime, [this, &aAnimation, aTargetMemory](gvk::mesh_bone_info aInfo, const glm::mat4& aInverseMeshRootMatrix, const glm::mat4& aTransformMatrix, const glm::mat4& aInverseBindPoseMatrix, const glm::mat4& aLocalTransformMatrix, size_t aAnimatedNodeIndex, size_t aBoneMeshTargetIndex, double aAnimationTimeInTicks) {
						// Construction of the bone matrix for this node:
						//   1. Bring vertex into bone space
						//   2. Apply transformaton in bone space => MODEL SPACE

							// => overwrite all the data! :O

						gvk::animated_node& anode = aAnimation.get_animated_node_at(aAnimatedNodeIndex);
						auto newLocalTransform = aAnimation.compute_node_local_transform(anode, 0.0 /* TODO: <-- which time for "no animation"? */);

						// Calculate the node's global transform, using its local transform and the transforms of its parents:
						if (anode.mAnimatedParentIndex.has_value()) {
							anode.mGlobalTransform = aAnimation.get_animated_node_at(anode.mAnimatedParentIndex.value()).get().mGlobalTransform * anode.mParentTransform * newLocalTransform;
						}
						else {
							anode.mGlobalTransform = anode.mParentTransform * newLocalTransform;
						}

						aTargetMemory[aInfo.mGlobalBoneIndexOffset + aInfo.mMeshLocalBoneIndex] = aTransformMatrix * aInverseBindPoseMatrix;
						});
					break;
				default:
					throw gvk::runtime_error("Unknown target space value.");
				}
			};

			auto aniTimeFu = [&dude]() {
				const auto doubleTime = fmod(gvk::time().absolute_time_dp(), std::get<animated_model_data>(dude).duration_sec() * 2);
				return  glm::lerp(std::get<animated_model_data>(dude).start_sec(), std::get<animated_model_data>(dude).end_sec(), (doubleTime > std::get<animated_model_data>(dude).duration_sec() ? doubleTime - std::get<animated_model_data>(dude).duration_sec() : doubleTime) / std::get<animated_model_data>(dude).duration_sec());
			};
			customAniFu(std::get<animated_model_data>(dude).mAnimation, std::get<animated_model_data>(dude).mClip, aniTimeFu(), boneMatSpaceAni, std::get<additional_animated_model_data>(dude).mBoneMatricesAni.data());
		}
	}

private: // v== Member variables ==v

	avk::queue* mQueue;
	avk::descriptor_cache mDescriptorCache;

	std::vector<std::tuple<animated_model_data, additional_animated_model_data>> mAnimatedModels;

	avk::buffer mViewProjBuffer;
	avk::buffer mMaterialBuffer;
	avk::buffer mMeshletsBuffer;
	std::array<std::vector<avk::buffer>, cConcurrecntFrames> mBoneMatricesBuffersAni;
	std::vector<avk::buffer> mMeshRootMatrices;
	std::vector<avk::buffer> mBindPoseMatrices;
	std::vector<avk::image_sampler> mImageSamplers;

	std::vector<data_for_draw_call> mDrawCalls;
	avk::graphics_pipeline mPipeline;
	gvk::quake_camera mQuakeCam;
	size_t mNumMeshletWorkgroups;

	std::vector<avk::buffer_view> mPositionBuffers;
	std::vector<avk::buffer_view> mIndexBuffers;
	std::vector<avk::buffer_view> mTexCoordsBuffers;
	std::vector<avk::buffer_view> mNormalBuffers;
	std::vector<avk::buffer_view> mBoneWeightsBuffers;
	std::vector<avk::buffer_view> mBoneIndicesBuffers;
#if !USE_DIRECT_MESHLET
	std::vector<avk::buffer_view> mMeshletDataBuffers;
#endif

	bool mHighlightMeshlets;

}; // skinned_meshlets_app

int main() // <== Starting point ==
{
	try {
		// Create a window and open it
		auto mainWnd = gvk::context().create_window("Static Meshlets");

		mainWnd->set_resolution({ 1920, 1080 });
		mainWnd->enable_resizing(true);
		mainWnd->set_additional_back_buffer_attachments({
			avk::attachment::declare(vk::Format::eD32Sfloat, avk::on_load::clear, avk::depth_stencil(), avk::on_store::dont_care)
			});
		mainWnd->set_presentaton_mode(gvk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(cConcurrecntFrames);
		mainWnd->open();

		auto& singleQueue = gvk::context().create_queue({}, avk::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->add_queue_family_ownership(singleQueue);
		mainWnd->set_present_queue(singleQueue);

		// Create an instance of our main avk::element which contains all the functionality:
		auto app = skinned_meshlets_app(singleQueue);
		// Create another element for drawing the UI with ImGui
		auto ui = gvk::imgui_manager(singleQueue);

		// GO:
		gvk::start(
			gvk::application_name("Gears-Vk + Auto-Vk Example: Static Meshlets"),
			gvk::required_device_extensions(VK_NV_MESH_SHADER_EXTENSION_NAME)
			.add_extension(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME),
			[](vk::PhysicalDeviceVulkan12Features& features) {
				features.setUniformAndStorageBuffer8BitAccess(VK_TRUE);
				features.setStorageBuffer8BitAccess(VK_TRUE);
			},
			mainWnd,
				app,
				ui
				);
	}
	catch (gvk::logic_error&) {}
	catch (gvk::runtime_error&) {}
	catch (avk::logic_error&) {}
	catch (avk::runtime_error&) {}
}

