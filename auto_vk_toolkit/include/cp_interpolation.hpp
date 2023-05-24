#pragma once

namespace avk
{
	/*	Base class for something (a curve?!) which interpolates between control points
	*/
	class cp_interpolation
	{
	public:
		cp_interpolation() = default;
		// Initializes a instance with a set of control points
		cp_interpolation(std::vector<glm::vec3> pControlPoints);
		cp_interpolation(cp_interpolation&&) = default;
		cp_interpolation(const cp_interpolation&) = default;
		cp_interpolation& operator=(cp_interpolation&&) = default;
		cp_interpolation& operator=(const cp_interpolation&) = default;
		virtual ~cp_interpolation() = default;

		// Set new control points, completely overriding the previous	
		void set_control_points(std::vector<glm::vec3> pControlPoints);

		// Get the control points of this instance
		const std::vector<glm::vec3>& control_points() const { return mControlPoints; }

		// Returns the number of control points
		auto num_control_points() const { return mControlPoints.size(); }

		// Returns the control point at the given index
		const auto& control_point_at(size_t index) const { return mControlPoints[index]; }

		/**	Get the value of this `cp_interpolation` at a certain interpolant.
		*	@param	t	The interpolant, range: 0..1
		*	@note	Child classed have to implement this virtual method
		*/
		virtual glm::vec3 value_at(float t) = 0;

		/**	Gets the slope of this `cp_interpolation` at a certain interpolant.
		*	@param	t	The interpolant, range: 0..1
		*	@note	Child classed have to implement this virtual method
		*/
		virtual glm::vec3 slope_at(float t) = 0;

		/**	Gets the distance between two control points.
		*	@param	first	One control point index
		*	@param	second	The other control point index
		*/
		float distance_between_control_points(size_t first, size_t second);

		/**	Gets the squared distance between two control points (cheaper to compute than the distance).
		*	@param	first	One control point index
		*	@param	second	The other control point index
		*/
		float squared_distance_between_control_points(size_t first, size_t second);

		/** Gets the (approximated or precise) arc length between the given control point and its successor.
		*	@param	first	One control point, the adjacent control point is the one at index `first+1`.
		*					Valid range: 0..`num_control_points()-1`.
		*	@note	Can be overwritten by child classes.
		*/
		virtual float arc_length_between_adjacent_control_points(size_t first);

		/** Gets the (approximated or precise) arc length between the control point at index `first` and the control point at index `second`
		*	@param	first	First control point, the adjacent control point is the one at index `first+1`.
		*					Valid range: 0..`num_control_points()-1`.
		*	@note	Can be overwritten by child classes.
		*/
		virtual float arc_length_between_control_points(size_t first, size_t second);

		/** Gets the total (approximated or precise) arc length from the first to the last control point.
		*	@note	Can be overwritten by child classes.
		*/
		virtual float arc_length();

	protected:
		std::vector<glm::vec3> mControlPoints;
	};

}
