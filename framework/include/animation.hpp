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

	class model_t;
	
	/**	Class that represents one specific animation for one or multiple meshes
	 */
	class animation
	{
		friend class model_t;
		
	public:
		void animate(const animation_clip_data& aClip, double mTime);

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
