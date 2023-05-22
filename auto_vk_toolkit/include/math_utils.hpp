#pragma once

namespace avk
{
	/// Helper method to get up-down and left-right rotation angles from a given direction
	extern glm::vec2 get_angles_from_direction_yaw_pitch(const glm::vec3& direction);

	/// Helper method to get roll and left-right rotation angles from a given direction
	extern glm::vec2 get_angles_from_direction_roll_pitch(const glm::vec3& direction);

	/// extract the translation part out of a matrix
	extern glm::vec3 get_translation_from_matrix(const glm::mat4& m);

	/** Set the translation part of a matrix to zero
	*   @param  aMatrix   a generic 4x4 input matrix
	*   @return a new matrix based on aMatrix, with the translation part set to zero
	*/
	glm::mat4 cancel_translation_from_matrix(const glm::mat4& aMatrix);

	/** A type defining a principal axis or direction, which can be x, y, or z */
	enum struct principal_axis : uint32_t { x = 0u, y, z };

	/** Mirror a matrix transformation along one axis
	*   @param  aMatrix		a generic 4x4 input matrix
	*   @param  aAxis		the mirrored axis
	*   @return a new matrix based on aMatrix, with the column defined by aAxis mirrored
	*/
	glm::mat4 mirror_matrix(const glm::mat4& aMatrix, principal_axis aAxis);

	/// <summary>
	/// Solve a system of equations with 3 unknowns.
	/// </summary>
	/// <param name="A">
	/// Matrix format: new Mat3(new Vec3(x1, y1, z1), new Vec3(x2, y2, z2), new Vec3(x3, y3, z3));
	/// => row vectors!!
	/// </param>
	/// <param name="c">Spaltenvektor, Format: new Vec3(c1, c2, c3)</param>
	/// <param name="outX">Lösungsvektor</param>
	/// <returns></returns>
	extern bool solve_system_of_equations(const glm::dmat3& A, const glm::dvec3& c, glm::dvec3& outX);

	/// <summary>
	/// Solve a system of equations with 2 unknowns.
	/// </summary>
	/// <param name="A">
	/// Matrix format: new Mat2(new Vec3(x1, y1), new Vec2(x2, y2), new Vec2(x3, y3));
	/// => row vectors!!
	/// </param>
	/// <param name="c">Spaltenvektor, Format: new Vec2(c1, c2)</param>
	/// <param name="outX">Lösungsvektor</param>
	/// <returns></returns>
	extern bool solve_system_of_equations(const glm::dmat2& A, const glm::dvec2& c, glm::dvec2& outX);

	extern bool points_in_same_direction(const glm::dvec2& a, const glm::dvec2& b);

	extern bool points_in_same_direction(const glm::dvec3& a, const glm::dvec3& b);

	extern bool almost_same_as(const glm::dvec2& a, const glm::dvec2& b, const double epsilon = 0.00001);

	extern bool almost_same_as(const glm::dvec3& a, const glm::dvec3& b, const double epsilon = 0.00001);

	extern bool same_as(const glm::dvec2& a, const glm::dvec2& b);

	extern bool same_as(const glm::dvec3& a, const glm::dvec3& b);

	/// Creates rotation matrix for rotating vector a to vector b
	/// Vectors a, and b must be normalized (I guess)
	/// [See Real-Time Rendering (Akenine Möller et. al) chapter 4.3.2]
	extern glm::mat4 rotate_vector_a_to_vector_b(glm::vec3 a, glm::vec3 b);

	///<summary>Solve an equation of the form a * x + b = 0</summary>
	///Checks if a given equation can be solved with real numbers.
	///If a real solution exists, a result is returned
	///<param name="constant_coeff">the constant coefficient b</param>
	///<param name="linear_coeff">the linear coefficient a</param>
	///<returns>The solution to the linear equation if a real solution can be found, std::nullopt otherwise</returns>
	extern std::optional<float> solve_linear_equation(float constant_coeff, float linear_coeff);

	///<summary>Solve an equation of the form a * x^2 + b * x + c = 0</summary>
	///Checks if a given equation can be solved with real numbers.
	///If a real solution exists, a result is returned
	///<param name="constant_coeff">the constant coefficient c</param>
	///<param name="linear_coeff">the linear coefficient b</param>
	///<param name="quadratic_coeff">the quadratic coefficient a</param>
	///<returns>Both solutions to the quadratic equation if a real solution can be found, std::nullopt otherwise</returns>
	extern std::optional<std::tuple<float, float>> solve_quadratic_equation(float constant_coeff, float linear_coeff, float quadratic_coeff);

	/// <summary>
	/// Calculate the factorial of an integer, has no kind of optimization 
	/// like memoizing values or suchlike => use this method for small numbers
	/// </summary>
	template <typename T>
	T factorial(T n)
	{
		T f = 1;
		for (int i = 2; i <= n; ++i) {
			f = f * i;
		}
		return f;
	}

	/**	Calculates the binomial coefficient, a.k.a. n over k
	 */
	template <typename T>
	T binomial_coefficient(T n, T k)
	{
		// return n.Factorial() / ((n - k).Factorial() * k.Factorial());

		// Optimized method, see http://stackoverflow.com/questions/9619743/how-to-calculate-binomial-coefficents-for-large-numbers			
		// (n C k) and (n C (n-k)) are the same, so pick the smaller as k:
		if (k > n - k) {
			k = n - k;
		}
		T result = 1;
		for (T i = 1; i <= k; ++i)
		{
			result *= n - k + i;
			result /= i;
		}
		return result;
	}

	/// <summary>
	/// Calculates the bernstein polynomial b_{i,n}(t)
	/// </summary>
	/// <returns>
	/// The polynomial.
	/// </returns>
	/// <param name='i'>
	/// The i parameter
	/// </param>
	/// <param name='n'>
	/// The n parameter
	/// </param>
	/// <param name='t'>
	/// t
	/// </param>
	template <typename T, typename P>
	P bernstein_polynomial(T i, T n, P t)
	{
		return static_cast<P>(binomial_coefficient(n, i) * glm::pow(t, i) * glm::pow(P(1) - t, n - i));
	}

	// Returns a quaternion such that q*start = dest
	// Sources: 
	//  1) "The Shortest Arc Quaternion" in Game Programming Gems 1, by Stan Melax
	//  2) opengl-tutorial, Tutorial 17 : Rotations, accessed 26.06.2019,
	//	   http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-17-quaternions/
	extern glm::quat rotation_between_vectors(glm::vec3 v0, glm::vec3 v1);
}
