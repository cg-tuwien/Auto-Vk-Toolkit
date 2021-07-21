#pragma once

#include <gvk.hpp>
#include <imgui.h>
#include <imgui_internal.h>

// An invokee that handles triangle mesh geometry:
class triangle_mesh_geometry_manager : public gvk::invokee
{
public: // v== gvk::invokee overrides which will be invoked by the framework ==v
	triangle_mesh_geometry_manager()
		: invokee{ -10 } // This invokee must execute BEFORE the main invokee
	{}

	void initialize() override
	{
		// Load an ORCA scene from file:
		auto orca = gvk::orca_scene_t::load_from_file("assets/sponza_and_terrain.fscene", aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);

		// Prepare a vector to hold all the material information of all models:
		std::vector<gvk::material_config> materialData;

		for (auto& model : orca->models()) {
			auto& nameAndRangeInfo = mBlasNamesAndRanges.emplace_back(
				model.mName,                                  // We are about to add several entries to mBlas for the model with this name
				static_cast<int>(mAllGeometryInstances.size()),  // These ^ entries start at this index
				-1                                            //  ... and we'll figure out the end index as we go.
			);

			// Get the distinct materials for every (static) mesh and accumulate them in a big array
			// wich will be transformed and stored in a big buffer, eventually:
			auto distinctMaterials = model.mLoadedModel->distinct_material_configs();
			for (const auto& [materialConfig, meshIndices] : distinctMaterials) {
				materialData.push_back(materialConfig);

				auto selection = gvk::make_models_and_meshes_selection(model.mLoadedModel, meshIndices);
				// Store all of this data in buffers and buffer views, s.t. we can access it later in ray tracing shaders
				auto [posBfr, idxBfr] = gvk::create_vertex_and_index_buffers<avk::uniform_texel_buffer_meta, avk::read_only_input_to_acceleration_structure_builds_buffer_meta>(
					selection                                          // Select several indices (those with the same material) from a model
				);
				auto nrmBfr = gvk::create_normals_buffer               <avk::uniform_texel_buffer_meta>(selection);
				auto texBfr = gvk::create_2d_texture_coordinates_buffer<avk::uniform_texel_buffer_meta>(selection);

				// Create a bottom level acceleration structure instance with this geometry.
				auto blas = gvk::context().create_bottom_level_acceleration_structure(
					{ avk::acceleration_structure_size_requirements::from_buffers(avk::vertex_index_buffer_pair{ posBfr, idxBfr }) },
					false // no need to allow updates for static geometry
				);
				blas->build({ avk::vertex_index_buffer_pair{ posBfr, idxBfr } });

				// Create a geometry instance entry per instance in the ORCA scene file:
				for (const auto& inst : model.mInstances) {
					auto bufferViewIndex = static_cast<uint32_t>(mTexCoordsBufferViews.size());

					// Create a concrete geometry instance:
					mAllGeometryInstances.push_back(
						gvk::context().create_geometry_instance(blas) // Refer to the concrete BLAS
							// Set this instance's transformation matrix:
						.set_transform_column_major(gvk::to_array(gvk::matrix_from_transforms(inst.mTranslation, glm::quat(inst.mRotation), inst.mScaling)))
						// Set this instance's custom index, which is especially important since we'll use it in shaders
						// to refer to the right material and also vertex data (these two are aligned index-wise):
						.set_custom_index(bufferViewIndex)
					);

					// State that this geometry instance shall be included in TLAS generation by default:
					mGeometryInstanceActive.push_back(true);

					// Generate and store a description for what this geometry instance entry represents:
					assert(!selection.empty());
					std::string description = model.mName + " (" + inst.mName + "): "; // ...refers to one specific model, and...
					for (const auto& tpl : selection) {
						for (const auto meshIndex : std::get<1>(tpl)) {
							description += std::get<0>(tpl)->name_of_mesh(meshIndex) + ", "; // ...to one or multiple submeshes.
						}
					}
					mGeometryInstanceDescriptions.push_back(description.substr(0, description.size() - 2));
				}

				mBlas.push_back(std::move(blas)); // Move this BLAS s.t. we don't have to enable_shared_ownership. We're done with it here.

				// After we have used positions and indices for building the BLAS, still need to create buffer views which allow us to access
				// the per vertex data in ray tracing shaders, where they will be accessible via samplerBuffer- or usamplerBuffer-type uniforms.
				mPositionsBufferViews.push_back(gvk::context().create_buffer_view(avk::owned(posBfr))); // owned is equivalent to move
				mIndexBufferViews.push_back(gvk::context().create_buffer_view(avk::owned(idxBfr)));
				mNormalsBufferViews.push_back(gvk::context().create_buffer_view(avk::owned(nrmBfr)));
				mTexCoordsBufferViews.push_back(gvk::context().create_buffer_view(avk::owned(texBfr)));
			}

			// Set the final range-to index (one after the end, i.e. excluding the last index):
			std::get<2>(nameAndRangeInfo) = static_cast<int>(mAllGeometryInstances.size());
		}

		// Set add all the geometry instances to mActiveGeometryInstances and set the mTlasUpdateRequired flag
		// in order to trigger initial TLAS build in our main invokee:
		mActiveGeometryInstances.insert(std::begin(mActiveGeometryInstances), std::begin(mAllGeometryInstances), std::end(mAllGeometryInstances));
		mTlasUpdateRequired = true;

		// Convert the materials that were gathered above into a GPU-compatible format and generate and upload images to the GPU:
		auto [gpuMaterials, imageSamplers] = gvk::convert_for_gpu_usage<gvk::material_gpu_data>(
			materialData, true /* assume textures in sRGB */, true /* flip textures */,
			avk::image_usage::general_texture,
			avk::filter_mode::trilinear // No need for MIP-mapping (which would be activated with trilinear or anisotropic) since we're using ray tracing
			);

		// Store images in a member variable, otherwise they would get destroyed.
		mImageSamplers = std::move(imageSamplers);

		// Upload materials in GPU-compatible format into a GPU storage buffer:
		mMaterialBuffer = gvk::context().create_buffer(
			avk::memory_usage::host_visible, {},
			avk::storage_buffer_meta::create_from_data(gpuMaterials)
		);
		mMaterialBuffer->fill(
			gpuMaterials.data(), 0,
			avk::sync::with_barriers(gvk::context().main_window()->command_buffer_lifetime_handler())
		);

		// Add an "ImGui Manager" which handles the UI specific to the requirements of this invokee:
		auto imguiManager = gvk::current_composition()->element_by_type<gvk::imgui_manager>();
		if (nullptr != imguiManager) {
			imguiManager->add_callback([this]() {
				ImGui::Begin("Triangle Mesh Geometry Instances");
				ImGui::SetWindowPos(ImVec2(10.0f, 430.0f), ImGuiCond_FirstUseEver);
				ImGui::SetWindowSize(ImVec2(400.0f, 600.0f), ImGuiCond_FirstUseEver);

				ImGui::Text("Specify which geometry instances to include in the TLAS:");

				// Let the user enable/disable some geometry instances w.r.t. includion into the TLAS:
				assert(mAllGeometryInstances.size() == mGeometryInstanceActive.size());
				assert(mAllGeometryInstances.size() == mGeometryInstanceDescriptions.size());
				auto numActive = std::accumulate(std::begin(mGeometryInstanceActive), std::end(mGeometryInstanceActive), 0, [](auto cur, auto nxt) { return cur + (nxt ? 1 : 0); });
				for (size_t i = 0; i < mAllGeometryInstances.size(); ++i) {
					bool tmp = mGeometryInstanceActive[i];

					// If there is only one active element left, it may not be disabled:
					const bool disable = numActive <= 1 && tmp;
					if (disable) {
						ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
					}

					mTlasUpdateRequired = ImGui::Checkbox((mGeometryInstanceDescriptions[i] + "##geominst" + std::to_string(i)).c_str(), &tmp) || mTlasUpdateRequired;
					mGeometryInstanceActive[i] = tmp;

					if (disable) {
						ImGui::PopItemFlag();
					}
				}

				ImGui::End();
			});
		}
	}

	// Returns true if a TLAS that uses the geometry of this invokee must be updated because the geometry (selection) has changed.
	[[nodiscard]] bool has_updated_geometry_for_tlas() const
	{
		return mTlasUpdateRequired;
	}
	
	// Return the active geometry instances to the caller, who will use it for a TLAS build:
	[[nodiscard]] std::vector<avk::geometry_instance> get_active_geometry_instances_for_tlas_build()
	{
		std::vector<avk::geometry_instance> toBeReturned = std::move(mActiveGeometryInstances);
		mActiveGeometryInstances.clear();
		mTlasUpdateRequired = false;
		return toBeReturned;
	}

	// Invoked by the framework every frame:
	void update() override
	{
		if (mTlasUpdateRequired)
		{
			// Getometry selection has changed => gather updated geometry information for TLAS build:
			assert(mAllGeometryInstances.size() == mGeometryInstanceActive.size());
			
			mActiveGeometryInstances.clear();
			for (size_t i = 0; i < mAllGeometryInstances.size(); ++i) {
				if (mGeometryInstanceActive[i]) {
					mActiveGeometryInstances.push_back(mAllGeometryInstances[i]);
				}
			}
		}
	}

	// Some getters that will be used by the main invokee:
	uint32_t max_number_of_geometry_instances() const { return static_cast<uint32_t>(mAllGeometryInstances.size()); }
	const auto& material_buffer() const { return mMaterialBuffer; }
	const auto& image_samplers() const { return mImageSamplers; }
	const auto& index_buffer_views() const { return mIndexBufferViews; }
	const auto& position_buffer_views() const { return mPositionsBufferViews; }
	const auto& tex_coords_buffer_views() const { return mTexCoordsBufferViews; }
	const auto& normals_buffer_views() const { return mNormalsBufferViews; }
	
private: // v== Member variables ==v

	// ------------------ Buffers and Buffer Views ------------------

	// A buffer that stores all material data of the loaded models:
	avk::buffer mMaterialBuffer;

	// Several images(+samplers) which store the material data's images:
	std::vector<avk::image_sampler> mImageSamplers;

	// Buffer views which provide the indexed geometry's index data:
	std::vector<avk::buffer_view> mIndexBufferViews;

	// Buffer views which provide the indexed geometry's positions data:
	std::vector<avk::buffer_view> mPositionsBufferViews;

	// Buffer views which provide the indexed geometry's texture coordinates data:
	std::vector<avk::buffer_view> mTexCoordsBufferViews;

	// Buffer views which provide the indexed geometry's normals data:
	std::vector<avk::buffer_view> mNormalsBufferViews;
	
	// ---------------- Acceleration Structures --------------------

	// A vector which stores a model name and a range of indices, refering to the mBlas vector.
	// The indices referred to by the [std:get<1>, std::get<2>) range are the associated submeshes.
	std::vector<std::tuple<std::string, int, int>> mBlasNamesAndRanges;

	// A vector of multiple bottom-level acceleration structures (BLAS) which store geometry:
	std::vector<avk::bottom_level_acceleration_structure> mBlas;

	// Geometry instance data which store the instance data per BLAS inststance:
	//    In our specific setup, this will be perfectly aligned with:
	//     - mIndexBufferViews
	//     - mTexCoordBufferViews
	//     - mNormalsBufferViews
	std::vector<avk::geometry_instance> mAllGeometryInstances;

	// A description per geometry instance to roughly describe what they refer to:
	std::vector<std::string> mGeometryInstanceDescriptions;

	// ------------------- UI settings -----------------------

	// One boolean per geometry instance to tell if it shall be included in the
	// generation of the TLAS or not:
	std::vector<bool> mGeometryInstanceActive;

	// True when an TLAS update is immanent:
	bool mTlasUpdateRequired = true;

	// Contains only the geometry instances which are acitive (and should participate
	// in the TLAS build):
	std::vector<avk::geometry_instance> mActiveGeometryInstances;

}; // End of triangle_mesh_geometry_manager
