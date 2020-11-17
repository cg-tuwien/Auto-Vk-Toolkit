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

	/**	Struct holding some bone<-->mesh-related data used in animation_node
	 */
	struct bone_mesh_data
	{
		/**	Matrix that transform from bone space to mesh space in bind pose.
		 *	This is called "inverse bind pose matrix" or "offset matrix".
		 */
		glm::mat4 mInverseBindPoseMatrix;

		/**	This vector contains a target pointer into target storage where
		 *	the resulting bone matrix is to be written to.
		 */
		glm::mat4* mBoneMatrixTarget;

		/**	For each mesh, this vector contains the mesh's root node's inverse 
		 *	matrix; i.e. the matrix that changes the basis of given coordinates
		 *	into mesh space.
		 *
		 *	Similarly to the matrix in mInverseBindPoseMatrix, this matrix
		 *	transform into mesh space. However, this matrix transform from OBJECT
		 *	SPACE into mesh space while mInverseBindPoseMatrix transform from
		 *	BONE SPACE into mesh space.
		 */
		glm::mat4 mInverseMeshRootMatrix;

		/**	Bone index within a given mesh. This corresponds, among others, to the
		 *	indices that are returned by model_t::inverse_bind_pose_matrices, if
		 *	used for the right/same mesh.
		 */
		unsigned int mBoneIndex;
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
		
		/** The GLOBAL transform of this node */
		glm::mat4 mTransform;

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

		/** Object space is the space of a model (within which meshes are positioned). */
		object_space,

		/** Bone space is the local space of a bone. */
		bone_space
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
		 *								This callback function MUST also write the bone matrix into the given target memory location (first parameter).
		 *								The parameters of this callback-function are as follows:
		 *								- First parameter:	Target pointer to the glm::mat4* memory where the RESULTING bone matrix MUST be written to.
		 *								- Second parameter: the "inverse mesh root matrix" that transforms coordinates into mesh root space
		 *								- Third parameter:	represents the "node transformation matrix" that represents a bone-transformation in bone-local space
		 *								- Fourth parameter:	represents the "inverse bind pose matrix" or "offset matrix" that transforms coordinates into bone-local space
		 *	@example [](glm::mat4* target, const glm::mat4& m1, const glm::mat4& m2, const glm::mat4& m3) { *target = m1 * m2 * m3; }
		 */
		template <typename F>
		void animate(const animation_clip_data& aClip, double aTime, F&& aFu)//  void(*aBoneMatrixCalc)(glm::mat4*, const glm::mat4&, const glm::mat4&, const glm::mat4&));
		{
			if (aClip.mTicksPerSecond == 0.0) {
				throw gvk::runtime_error("mTicksPerSecond may not be 0.0 => set a different value!");
			}
			if (aClip.mAnimationIndex != mAnimationIndex) {
				throw gvk::runtime_error("The animation index of the passed animation_clip_data is not the same that was used to create this animation.");
			}

			double timeInTicks = aTime * aClip.mTicksPerSecond;

			for (auto& anode : mAnimationData) {
				// First, calculate the local transform
				glm::mat4 localTransform{1.0f};

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
					anode.mTransform = mAnimationData[anode.mAnimatedParentIndex.value()].mTransform * anode.mParentTransform * localTransform;
				}
				else {
					anode.mTransform = anode.mParentTransform * localTransform;
				}

				// Calculate the final bone matrices for this node, for each mesh that is affected; and write out the matrix into the target storage:
				const auto n = anode.mBoneMeshTargets.size();
				for (size_t i = 0; i < n; ++i) {
					// Construction of the bone matrix for this node:
					//  1. Bring vertex into bone space
					//  2. Apply transformaton in bone space
					//  3. Convert transformed vertex back to mesh space
					// The callback function will store into target:
					aFu(anode, i);
				}
			}
		}

		/** Calculates the bone animation, calculates and writes all the bone matrices into their target storage.
		 *	The resulting bone matrices are transformed into mesh space (see also bone_matrices_space::mesh_space).
		 *	@param	aClip				Animation clip to use for the animation
		 *	@param	aTime				Time in seconds to calculate the bone matrices at.
		 */
		void animate(const animation_clip_data& aClip, double aTime);

		/**	Calculates the bone animation, calculates and writes all the bone matrices into their target storage.
		 *	The space of the resulting bone matrices can be controlled with the third parameter.
		 *	@param	aClip				Animation clip to use for the animation
		 *	@param	aTime				Time in seconds to calculate the bone matrices at.
		 *	@param	aTargetSpace		Desired target space for the bone matrices (see bone_matrices_space)
		 */
		void animate(const animation_clip_data& aClip, double aTime, bone_matrices_space aTargetSpace);

		std::vector<double> animation_keys_within_clip(const animation_clip_data& aClip);
		
	private:
		/** Helper function used during animate() to find two positions of key-elements
		 *	between which the given aTime lies.
		 */
		template <typename T>
		std::tuple<size_t, size_t> find_positions_in_keys(const T& aCollection, double aTime)
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
		float get_interpolation_factor(const K& key1, const K& key2, double aTime)
		{
			double timeDifferenceTicks = key2.mTime - key1.mTime;
			if (std::abs(timeDifferenceTicks) < std::numeric_limits<double>::epsilon()) {
				return 1.0f; // Same time, doesn't really matter which one to use => take key2
			}
			assert (key2.mTime > key1.mTime);
			return static_cast<float>((aTime - key1.mTime) / timeDifferenceTicks);
		}
		
		/** Collection of tuples with two elements:
		 *  [0]: The mesh index to be animated
		 *  [1]: Pointer to the target storage where bone matrices shall be written to
		 */
		std::vector<std::tuple<mesh_index_t, glm::mat4*>> mMeshIndicesAndTargetStorage;

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
