#pragma once
#include <gvk.hpp>

namespace gvk
{
	struct animation_clip_data
	{
		unsigned int mAnimationIndex;
		double mTicksPerSecond;
		double mStartTicks;
		double mEndTicks;

		double start_ticks() const
		{
			return mStartTicks;
		}

		double end_ticks() const
		{
			return mEndTicks;
		}
		
		double start_time() const
		{
			if (mTicksPerSecond == 0.0) {
				throw gvk::runtime_error("animation_clip_data::mTicksPerSecond may not be 0.0 => set a different value!");
			}
			return mStartTicks / mTicksPerSecond;
		}

		double end_time() const
		{
			if (mTicksPerSecond == 0.0) {
				throw gvk::runtime_error("animation_clip_data::mTicksPerSecond may not be 0.0 => set a different value!");
			}
			return mEndTicks / mTicksPerSecond;
		}
	};

	struct position_key
	{
		double mTime;
		glm::vec3 mValue;
	};

	struct rotation_key
	{
		double mTime;
		glm::quat mValue;
	};

	struct scaling_key
	{
		double mTime;
		glm::vec3 mValue;
	};

	/**	Struct which contains information about a particular bone w.r.t. a particular mesh
	 *	during animation. I.e. when a certain mesh-specific(!) bone matrix shall be written
	 *	to its target location, this struct contains the following information:
	 *	 - Which mesh does this relate to => mMeshIndex
	 *	 - Which mesh-local bone-information is relevant => mMeshLocalBoneIndex
	 *	 - Which target index shall the mesh-specific(!) bone matrix be written to => mBoneMatrixTargetIndex
	 */
	struct mesh_bone_info
	{
		/** The index at which this mesh was passed to animation::prepare_animation. */
		size_t mMeshAnimationIndex;
		
		/** The model-based mesh-id, i.e. the id which model-internally uniquely identifies a mesh. */
		mesh_index_t mMeshIndexInModel;

		/** Mesh-local(!) bone index that the given bone matrix shall be set for.
		 *	The mesh-local bone index refers to the the n-th bone index that is
		 *	relevant for the given mesh. This does NOT mean that this is the
		 *	global bone index of the skeleton. It refers to ASSIMP's n-th aiBone
		 *	for this mesh. 
		 */
		uint32_t mMeshLocalBoneIndex;

		/** This, in combination with mMeshLocalBoneIndex, CAN be used as the the target
		 *	index where the given bone matrix shall be written to if a single target buffer
		 *	is being used for bone indices.
		 *	It represents the monotonically increasing bone indices offsets w.r.t. all 
		 *	meshes and mesh's bones IN THE ORDER in which they were passed to the method
		 *	model_t::prepare_animation. I.e. the order of the mesh_index_t values matters!
		 *	You can use this value to determine the offset for the target index, i.e. where to
		 *	write a bone matrix to, but it is also perfectly fine to use some other parameter
		 *	for determining that.
		 *
		 *	For example, let's assume there is a model that contains three child meshes,
		 *	and has been modeled so that its skeleton has a total of four bones. Let's 
		 *	further assume that all the vertices per child mesh are influenced by not
		 *	more than two bones, each. 
		 *	If we called that model m with model_t::prepare_animation and passed in all
		 *	mesh ids, e.g. via m->select_all_meshes(), then we had gotten 0 for the first
		 *	mesh index, 2 for the second mesh index, and 4 for the third mesh index as
		 *	mGlobalBoneIndexOffset-values, respectively.
		 */
		size_t mGlobalBoneIndexOffset;
	};

	/**	Struct holding some bone<-->mesh-related data used in animation_node
	 */
	struct bone_mesh_data
	{
		/**	Matrix that changes the basis of from mesh space to bone space in
		 *	bind pose (i.e. the initial/original, un-animated pose).
		 *	This is called "inverse bind pose matrix" or "offset matrix".
		 */
		glm::mat4 mInverseBindPoseMatrix;

		/**	The matrix which changes the basis of model space to mesh space.
		 *	
		 *	This matrix can be useful during bone animation: After applying the
		 *	transformation hierarchy matrices, the result will be in model space.
		 *	This matrix can be used to transform it back into mesh space, which
		 *	is the space in which the vertices were given initially, too. 
		 */
		glm::mat4 mInverseMeshRootMatrix;

		/**	Contains the following information:
		 *	- Index of the mesh being animated
		 *	- Mesh-local(!) bone index
		 *	- (optional) Target-index where the bone matrix target shall be written to.
		 */
		mesh_bone_info mMeshBoneInfo;


		glm::mat4 change_basis_from_mesh_space_to_bone_space() const
		{
			return mInverseBindPoseMatrix;
		}

		glm::mat4 change_basis_from_model_space_to_mesh_space() const
		{
			return mInverseMeshRootMatrix;
		}

		glm::mat4 change_basis_from_model_space_to_bone_space() const
		{
			return mInverseBindPoseMatrix * mInverseMeshRootMatrix;
		}

		glm::mat4 change_basis_from_bone_space_to_mesh_space() const
		{
			return glm::inverse(change_basis_from_mesh_space_to_bone_space());
		}

		glm::mat4 change_basis_from_mesh_space_to_model_space() const
		{
			return glm::inverse(change_basis_from_model_space_to_mesh_space());
		}

		glm::mat4 change_basis_from_bone_sace_to_model_space() const
		{
			return glm::inverse(change_basis_from_model_space_to_bone_space());
		}

	};

	/**	Struct containing data about one specific animated node.
	 *	Class animation will contain multiple of such in most cases.
	 */
	struct animated_node
	{
		/**	Animation keys for the positions of this node. */
		std::vector<position_key> mPositionKeys;

		/**	Animation keys for the rotations of this node. */
		std::vector<rotation_key> mRotationKeys;

		/**	Animation keys for the scalings of this node. */
		std::vector<scaling_key> mScalingKeys;

		/** True if the mPositionKeys and mRotationKeys contain the same
		 *	number of elements and each element with the same index has
		 *	the same mTime in both.
		 */
		bool mSameRotationAndPositionKeyTimes;

		/** True if the mPositionKeys and mScalingKeys contain the same
		 *	number of elements and each element with the same index has
		 *	the same mTime in both.
		 */
		bool mSameScalingAndPositionKeyTimes;

		/** The local transform of this node */
		glm::mat4 mLocalTransform;
		
		/** The global transform of this node */
		glm::mat4 mGlobalTransform;

		/** Contains the index of a parent node IF this node HAS a parent
		 *	node that is affected by animation.
		 */
		std::optional<size_t> mAnimatedParentIndex;

		/** Parent transform that must be applied to this node.
		 *
		 *	IF this node has an mAnimatedParentIndex, the mParentTransform
		 *	must be applied BEFORE the animated parent's transform is applied!
		 */
		glm::mat4 mParentTransform;

		/** Contains values only if this node IS a bone.
		 *	It contains matrices which are essential for calculating one specific
		 *	bone matrix for a certain mesh, and also the target pointer where to
		 *	write the resulting bone matrix (for the specific mesh) to in memory.
		 *
		 *	There will be multiple matrices contained if the animation has
		 *	been created for multiple meshes. It can contain at most one
		 *	entry if the animation is created for one mesh only.
		 *	The mesh index it refers to is not stored within this struct.
		 *	(It could probably be calculated from the offsets stored in
		 *	 mBoneMatrixTargets, though.)
		 */
		std::vector<bone_mesh_data> mBoneMeshTargets;
	};

	/** Represents possible spaces which the final bone matrices can be transformed into. */
	enum struct bone_matrices_space
	{
		/** Mesh space is the space of a mesh's root node. This is the default. */
		mesh_space,

		/** Model space is the space of a model (within which meshes are positioned). */
		model_space,
	};
	
	class model_t;

	/**	Class that represents one specific animation for one or multiple meshes
	 */
	class animation
	{
		friend class model_t;
		
	public:
		/**	Calculates the bone animation, calculates and writes all the bone matrices into their target storage.
		 *
		 *	@param	aClip				Animation clip to use for the animation
		 *	@param	aTime				Time in seconds to calculate the bone matrices at.
		 *	@param	aBoneMatrixCalc		Callback-function that receives the three matrices which can be relevant for computing the final bone matrix.
		 *								This callback function MUST write the bone matrix into the given target memory location or generally, use them in
		 *								some application-specific way. The bone matrices are not stored/written to automatically. This is your responsibility!
		 *	
		 *								There are multiple options for the parameters that the lambda can take---according to your requirements.
		 *								It must at least take the first four required parameters, but can take up to eight parameters (with the latter
		 *								four being optional, but they must be specified exactly in the right order!). 
		 *								
		 *								The parameters to the lambda function -- in the right order -- are as follows:
		 *								mesh_bone_info, inverse mesh root matrix, global node/bone transform w.r.t. the animation, inverse bind-pose matrix, local node/bone transformation, animated_node, bone mesh targets index, animation time in ticks
		 *								- gvk::mesh_bone_info aInfo:               (mandatory) Contains the indices which tell about the specific bone matrix this callback is being invoked for.
		 *								- const glm::mat4& aInverseMeshRootMatrix: (mandatory) The "inverse mesh root matrix" that transforms coordinates into mesh root space, that is the mesh space of the mesh with index aInfo.mMeshIndex
		 *								- const glm::mat4& aGlobalTransformMatrix: (mandatory) Represents the "node transformation matrix" that represents a bone-transformation, considering the whole parent hierarchy. That means, it contains the global transform, transforming from BONE SPACE into MODEL SPACE.
		 *								- const glm::mat4& aInverseBindPoseMatrix: (mandatory) Represents the "inverse bind pose matrix" or "offset matrix" that transforms coordinates from MESH SPACE (i.e. that from the mesh with index aInfo.mMeshIndex) into BONE SPACE.
		 *								- const glm::mat4& aLocalTransformMatrix:  (optional)  Contains the local bone transformation, i.e. the same data as aGlobalTransformationMatrix, but WITHOUT having the whole parent hierarchy applied to the transformation. I.e. this does NOT properly transform into MODEL SPACE.
		 *								- const gvk::animated_node& aAnimatedNode: (optional)  Contains all the animation data from the the current (internal) animation-node-structure.
		 *								- size_t aBoneMeshTargetIndex:             (optional)  Contains the index into animated_node::mBoneMeshTargets that is the current one at the point in time when this callback is invoked.
		 *								- double aAnimationTimeInTicks:            (optional)  Contains the animation time in ticks, at the point in time when this callback is invoked.
		 *								
		 *	@example [storagePointer](mesh_bone_info aInfo, const glm::mat4& aInverseMeshRootMatrix, const glm::mat4& aTransformMatrix, const glm::mat4& aInverseBindPoseMatrix){
		 *	    // You'll need to ^ capture some pointer or data structure to write into the target location.
		 *		assert (aInfo.mBoneMatrixTargetIndex.has_value());
		 *		// Store the result in mesh space (which is the same space as the original vertex data):
		 *		storagePointer[aInfo.mBoneMatrixTargetIndex.value()] = aInverseMeshRootMatrix * aTransformMatrix * aInverseBindPoseMatrix;
		 *	}
		 */
		template <typename F>
		void animate(const animation_clip_data& aClip, double aTime, F&& aBoneMatrixCalc)
		{
			if (aClip.mTicksPerSecond == 0.0) {
				throw gvk::runtime_error("animation_clip_data::mTicksPerSecond may not be 0.0 => set a different value!");
			}
			if (aClip.mAnimationIndex != mAnimationIndex) {
				throw gvk::runtime_error("The animation index of the passed animation_clip_data is not the same that was used to create this animation.");
			}

			double timeInTicks = aTime * aClip.mTicksPerSecond;

			for (auto& anode : mAnimationData) {
				// First, calculate the local transform
				glm::mat4 localTransform = anode.mLocalTransform;

				// The localTransform can only be different than the identity if there are animation keys.
				if (anode.mPositionKeys.size() + anode.mRotationKeys.size() + anode.mScalingKeys.size() > 0) {
					// Translation/position:
					auto [tpos1, tpos2] = find_positions_in_keys(anode.mPositionKeys, timeInTicks);
					auto tf = get_interpolation_factor(anode.mPositionKeys[tpos1], anode.mPositionKeys[tpos2], timeInTicks);
					auto translation = glm::lerp(anode.mPositionKeys[tpos1].mValue, anode.mPositionKeys[tpos2].mValue, tf);

					// Rotation:
					size_t rpos1 = tpos1, rpos2 = tpos2;
					if (!anode.mSameRotationAndPositionKeyTimes) {
						std::tie(rpos1, rpos2) = find_positions_in_keys(anode.mRotationKeys, timeInTicks);
					}
					auto rf = get_interpolation_factor(anode.mRotationKeys[rpos1], anode.mRotationKeys[rpos2], timeInTicks);
					auto rotation = glm::slerp(anode.mRotationKeys[rpos1].mValue, anode.mRotationKeys[rpos2].mValue, rf);	// use slerp, not lerp or mix (those lead to jerks)
					rotation = glm::normalize(rotation); // normalize the resulting quaternion, just to be on the safe side

					// Scaling:
					size_t spos1 = tpos1, spos2 = tpos2;
					if (!anode.mSameScalingAndPositionKeyTimes) {
						std::tie(spos1, spos2) = find_positions_in_keys(anode.mScalingKeys, timeInTicks);
					}
					auto sf = get_interpolation_factor(anode.mScalingKeys[spos1], anode.mScalingKeys[spos2], timeInTicks);
					auto scaling = glm::lerp(anode.mScalingKeys[spos1].mValue, anode.mScalingKeys[spos2].mValue, sf);

					localTransform = matrix_from_transforms(translation, rotation, scaling);
				}

				// Calculate the node's global transform, using its local transform and the transforms of its parents:
				if (anode.mAnimatedParentIndex.has_value()) {
					anode.mGlobalTransform = mAnimationData[anode.mAnimatedParentIndex.value()].mGlobalTransform * anode.mParentTransform * localTransform;
				}
				else {
					anode.mGlobalTransform = anode.mParentTransform * localTransform;
				}

				// Calculate the final bone matrices for this node, for each mesh that is affected; and write out the matrix into the target storage:
				const auto n = anode.mBoneMeshTargets.size();
				for (size_t i = 0; i < n; ++i) {
					// The final (mesh-specific!) bone matrix will be created in and stored via the lambda:
					if constexpr (std::is_assignable<std::function<void(mesh_bone_info, const glm::mat4&, const glm::mat4&, const glm::mat4&)>, decltype(aBoneMatrixCalc)>::value) {
						// Option 1: lambda that takes: mesh_bone_info, inverse mesh root matrix, global node/bone transform w.r.t. the animation, inverse bind-pose matrix
						aBoneMatrixCalc(anode.mBoneMeshTargets[i].mMeshBoneInfo, anode.mBoneMeshTargets[i].mInverseMeshRootMatrix, anode.mGlobalTransform, anode.mBoneMeshTargets[i].mInverseBindPoseMatrix);
					}
				    else if constexpr (std::is_assignable<std::function<void(mesh_bone_info, const glm::mat4&, const glm::mat4&, const glm::mat4&, const glm::mat4&)>, decltype(aBoneMatrixCalc)>::value) {
						// Option 2: lambda that takes: mesh_bone_info, inverse mesh root matrix, global node/bone transform w.r.t. the animation, inverse bind-pose matrix, local node/bone transformation
				    	//           (The first four parameters are the same as with Option 1. Parameter five is passed in addition.)
						aBoneMatrixCalc(anode.mBoneMeshTargets[i].mMeshBoneInfo, anode.mBoneMeshTargets[i].mInverseMeshRootMatrix, anode.mGlobalTransform, anode.mBoneMeshTargets[i].mInverseBindPoseMatrix, localTransform);
				    }
				    else if constexpr (std::is_assignable<std::function<void(mesh_bone_info, const glm::mat4&, const glm::mat4&, const glm::mat4&, const glm::mat4&, const animated_node&)>, decltype(aBoneMatrixCalc)>::value) {
						// Option 3: lambda that takes: mesh_bone_info, inverse mesh root matrix, global node/bone transform w.r.t. the animation, inverse bind-pose matrix, local node/bone transformation, animated_node
				    	//           (The first five parameters are the same as with Option 2. Parameter six is passed in addition.)
						aBoneMatrixCalc(anode.mBoneMeshTargets[i].mMeshBoneInfo, anode.mBoneMeshTargets[i].mInverseMeshRootMatrix, anode.mGlobalTransform, anode.mBoneMeshTargets[i].mInverseBindPoseMatrix, localTransform, anode);
				    }
				    else if constexpr (std::is_assignable<std::function<void(mesh_bone_info, const glm::mat4&, const glm::mat4&, const glm::mat4&, const glm::mat4&, const animated_node&, size_t)>, decltype(aBoneMatrixCalc)>::value) {
						// Option 4: lambda that takes: mesh_bone_info, inverse mesh root matrix, global node/bone transform w.r.t. the animation, inverse bind-pose matrix, local node/bone transformation, animated_node, bone mesh targets index
				    	//           (The first six parameters are the same as with Option 3. Parameter seven is passed in addition.)
						aBoneMatrixCalc(anode.mBoneMeshTargets[i].mMeshBoneInfo, anode.mBoneMeshTargets[i].mInverseMeshRootMatrix, anode.mGlobalTransform, anode.mBoneMeshTargets[i].mInverseBindPoseMatrix, localTransform, anode, i);
				    }
				    else if constexpr (std::is_assignable<std::function<void(mesh_bone_info, const glm::mat4&, const glm::mat4&, const glm::mat4&, const glm::mat4&, const animated_node&, size_t, double)>, decltype(aBoneMatrixCalc)>::value) {
						// Option 4: lambda that takes: mesh_bone_info, inverse mesh root matrix, global node/bone transform w.r.t. the animation, inverse bind-pose matrix, local node/bone transformation, animated_node, bone mesh targets index, animation time in ticks
				    	//           (The first seven parameters are the same as with Option 4. Parameter eight is passed in addition.)
						aBoneMatrixCalc(anode.mBoneMeshTargets[i].mMeshBoneInfo, anode.mBoneMeshTargets[i].mInverseMeshRootMatrix, anode.mGlobalTransform, anode.mBoneMeshTargets[i].mInverseBindPoseMatrix, localTransform, anode, i, timeInTicks);
				    }
					else {
#if defined(_MSC_VER) && defined(__cplusplus)
						static_assert(false);
#else
						assert(false);
#endif
						throw avk::logic_error("No lambda has been passed to animation::animate.");
					}
					
				}
			}
		}

		/** Convenience-overload to animation::animate which calculates the bone animation s.t. a vertex transformed
		 *	with one of the resulting bone matrices is given in mesh space (same as the original input data) again.
		 *	This method writes the bone matrices into contiguous strided memory where aTargetMemory points to the
		 *	location where the first bone matrix shall be written to.
		 *	
		 *	@param	aClip				Animation clip to use for the animation
		 *	@param	aTime				Time in seconds to calculate the bone matrices at.
		 *	@param	aTargetMemory		Pointer to the memory location where the first bone matrix shall be written to
		 *	@param	aMeshStride			Offset in BYTES between the first memory target location for mesh i, and the first memory target location for mesh i+1
		 *	@param	aMatricesStride		Offset in BYTES between two consecutive bone matrices that are assigned to the same mesh. By default, it will be set to sizeof(glm::mat4)
		 *	@param	aMaxMeshes			The maximum number of meshes to write out bone matrices for. That means, always the first #aMaxMeshes meshes w.r.t. mesh_bone_info::mMeshAnimationIndex will be written.
		 *	@param	aMaxBonesPerMesh	The maximum number of bones to write out bone matrices for per mesh. Only the first #aMaxBonesPerMesh bone matrices w.r.t. mesh_bone_info::mMeshLocalBoneIndex will be written.
		 */
		void animate_into_strided_target_per_mesh(const animation_clip_data& aClip, double aTime, glm::mat4* aTargetMemory, size_t aMeshStride, std::optional<size_t> aMatricesStride = {}, std::optional<size_t> aMaxMeshes = {}, std::optional<size_t> aMaxBonesPerMesh = {});

		/**	Convenience-overload to animation::animate which calculates the bone animation s.t. a vertex transformed
		 *	with one of the resulting bone matrices is given in mesh space (same as the original input data) again.
		 *	This method writes the bone matrices into one single contiguous piece of memory which is expected to
		 *	be the single target to receive ALL bone matrices of all meshes. This method is intended to be used with
		 *	bone indices that have been retrieved(or transformed!) with one of the model_t::*_for_single_target_buffer methods.
		 * 
		 *	@param	aClip				Animation clip to use for the animation
		 *	@param	aTime				Time in seconds to calculate the bone matrices at.
		 *	@param	aTargetMemory		Pointer to the memory location where the first bone matrix shall be written to
		 */
		void animate_into_single_target_buffer(const animation_clip_data& aClip, double aTime, glm::mat4* aTargetMemory);

		/** Convenience-overload to animation::animate which calculates the bone animation s.t. a vertex transformed
		 *	with one of the resulting bone matrices is transformed into the given target space.
		 *	This method writes the bone matrices into contiguous strided memory where aTargetMemory points to the
		 *	location where the first bone matrix shall be written to.
		 *
		 *	@param	aClip				Animation clip to use for the animation
		 *	@param	aTime				Time in seconds to calculate the bone matrices at.
		 *	@param	aTargetSpace		The target space into which the vertices shall be transformed by multiplying them with the bone matrices
		 *	@param	aTargetMemory		Pointer to the memory location where the first bone matrix shall be written to
		 *	@param	aMeshStride			Offset in BYTES between the first memory target location for mesh i, and the first memory target location for mesh i+1
		 *	@param	aMatricesStride		Offset in BYTES between two consecutive bone matrices that are assigned to the same mesh. By default, it will be set to sizeof(glm::mat4)
		 *	@param	aMaxMeshes			The maximum number of meshes to write out bone matrices for. That means, always the first #aMaxMeshes meshes w.r.t. mesh_bone_info::mMeshAnimationIndex will be written.
		 *	@param	aMaxBonesPerMesh	The maximum number of bones to write out bone matrices for per mesh. Only the first #aMaxBonesPerMesh bone matrices w.r.t. mesh_bone_info::mMeshLocalBoneIndex will be written.
		 */
		void animate_into_strided_target_per_mesh(const animation_clip_data& aClip, double aTime, bone_matrices_space aTargetSpace, glm::mat4* aTargetMemory, size_t aMeshStride, std::optional<size_t> aMatricesStride = {}, std::optional<size_t> aMaxMeshes = {}, std::optional<size_t> aMaxBonesPerMesh = {});

		/**	Convenience-overload to animation::animate which calculates the bone animation s.t. a vertex transformed
		 *	with one of the resulting bone matrices is transformed into the given target space.
		 *	This method writes the bone matrices into one single contiguous piece of memory which is expected to
		 *	be the single target to receive ALL bone matrices of all meshes. This method is intended to be used with
		 *	bone indices that have been retrieved(or transformed!) with one of the model_t::*_for_single_target_buffer methods.
		 *
		 *	@param	aClip				Animation clip to use for the animation
		 *	@param	aTime				Time in seconds to calculate the bone matrices at.
		 *	@param	aTargetSpace		The target space into which the vertices shall be transformed by multiplying them with the bone matrices
		 *	@param	aTargetMemory		Pointer to the memory location where the first bone matrix shall be written to
		 */
		void animate_into_single_target_buffer(const animation_clip_data& aClip, double aTime, bone_matrices_space aTargetSpace, glm::mat4* aTargetMemory);

		/**	Returns all the unique keyframe time-values of the given animation.
		 *	@param	aClip				Animation clip which to extract the unique keyframe time-values from
		 */
		std::vector<double> animation_key_times_within_clip(const animation_clip_data& aClip) const;

		/** Returns the total number of animated nodes stored in an animation */
		size_t number_of_animated_nodes() const;
		
		/** Returns the animated_node data structure at the given index
		 *	@param	aNodeIndex			Index referring to the node that shall be returned
		 */
		std::reference_wrapper<animated_node> get_animated_node_at(size_t aNodeIndex);
		
		/**	Returns the index of the parent node which is also animated by this animation for the given node index.
		 *	@param	aNodeIndex			Index referring to the node of which the animated parent shall be returned for.
		 */
		std::optional<size_t> get_animated_parent_index_of(size_t aNodeIndex) const;

		/**	Returns a reference to the parent node which is also animated by this animation for the given node index.
		 *	@param	aNodeIndex			Index referring to the node of which the animated parent shall be returned for.
		 */
		std::optional<std::reference_wrapper<animated_node>> get_animated_parent_node_of(size_t aNodeIndex);

		/**	Returns the indices of all nodes that the given node index is an animated parent for within the context of this animation.
		 *	@param	aNodeIndex			Index referring to the node of which the animated childs shall be returned for.
		 */
		std::vector<size_t> get_child_indices_of(size_t aNodeIndex) const;

		/**	Returns references to all nodes that the given node index is an animated parent for within the context of this animation.
		 *	@param	aNodeIndex			Index referring to the node of which the animated childs shall be returned for.
		 */
		std::vector<std::reference_wrapper<animated_node>> get_child_nodes_of(size_t aNodeIndex);
		
	private:
		/** Helper function used during animate() to find two positions of key-elements
		 *	between which the given aTime lies.
		 */
		template <typename T>
		std::tuple<size_t, size_t> find_positions_in_keys(const T& aCollection, double aTime) const
		{
			const auto maxIndex = aCollection.size() - 1;
			
			size_t pos1 = 0;
			while (pos1 + 1 <= maxIndex && aCollection[pos1 + 1].mTime <= aTime) {
				++pos1;
			}
			
			size_t pos2 = pos1 + (pos1 < maxIndex ? 1 : 0);
			while (pos2 + 1 < maxIndex && aCollection[pos2 + 1].mTime < aTime) {
				LOG_WARNING(fmt::format("Now that's strange: keys[{}].mTime {} < {}, despite keys[{}].mTime {} <= {}", pos2 + 1, aCollection[pos2 + 1].mTime, aTime, pos1, aCollection[pos1].mTime, aTime));
				++pos2;
			}
			
			return std::make_tuple(pos1, pos2);
		}

		/**	For two kiven keys (each of which must contain a .mTime member of type
		 *	double), and a given aTime value, return the corresponding interpolation
		 *	factor in the range [0..1].
		 */
		template <typename K>
		float get_interpolation_factor(const K& key1, const K& key2, double aTime) const
		{
			double timeDifferenceTicks = key2.mTime - key1.mTime;
			if (std::abs(timeDifferenceTicks) < std::numeric_limits<double>::epsilon()) {
				return 1.0f; // Same time, doesn't really matter which one to use => take key2
			}
			assert (key2.mTime > key1.mTime);
			return static_cast<float>((aTime - key1.mTime) / timeDifferenceTicks);
		}
		
		/**	All animated nodes, along with their animation data and target storage pointers
		 */
		std::vector<animated_node> mAnimationData;

		/**	ASSIMP's animation clip index that was used to create this animation instance.
		 */
		uint32_t mAnimationIndex;
		
		/** Maximum number of bone matrices to write into the target storage pointers.
		 *  (Second tuple element of mMeshIndicesAndTargetStorage)
		 */
		size_t mMaxNumBoneMatrices;
	};
}
