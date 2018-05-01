#pragma once

#include <cmath>
#include <cstring>

/*! Fast Math
 * This class contains a vector implementation for any size.
 * SIMD/unrolling is only supported by 3D vectors.
 * Define `FM_USE_SIMD` to always use SIMD instead of unrolling.
 * The third `Vec` template argument is used to force SIMD as well.
 */
namespace fm
{

	/*! Storage classes
	 *  These classes define the different access types like `rgb`, `xyz` and etc.
	 */
	namespace storage
	{
		/*! Fallback storage structure */
		template<class T, int R>
		struct Vec
		{
			union
			{
				T data[R];
			};
		};

		/*! 4D vector storage structure */
		template<class T>
		struct Vec<T, 4>
		{
		public:
			union
			{
				T data[4];
				struct { T x, y, z, w; };
				struct { T r, g, b, a; };
			};
		};

		/*! 3D vector storage structure */
		template<class T>
		struct Vec<T, 3>
		{
		public:
			union
			{
				T data[3];
				struct { T x, y, z; };
				struct { T r, g, b; };
			};
		};

		/*! 2D vector storage structure */
		template<class T>
		struct Vec<T, 2>
		{
			union
			{
				T data[2];
				struct { T x, y; };
			};
		};
	}

	/*! Main Vector class
	 *  Supports automatic unrolling for 3D vectors.
	 *  and SIMD for 3D vectors.
	 */
	template<class T = float, unsigned int R = 3, bool SIMD = false>
	class Vec : public storage::Vec<T, R>
	{
	public:
		using storage::Vec<T, R>::data;

		// Constructors for multiple dimensions
		constexpr Vec(T x, T y) : storage::Vec<T, R>{ x, y } {}
		constexpr Vec(T x, T y, T z) : storage::Vec<T, R>{ x, y, z } {}
		constexpr Vec(T x, T y, T z, T w) : storage::Vec<T, R>{ x, y, z, w } {}

		/*! Default constructor initializes the data to 0 */
		constexpr Vec()
		{
			for (decltype(R) i = 0; i < R; i++)
			{
				data[i] = 0;
			}
		}

		/*! Constructs a vector from an array */
		constexpr explicit Vec(T data[R])
		{
			std::memcpy(this->data, data, sizeof(T) * R);
		}

		~Vec() = default;

		/*!
		 * Allow accessing the data directly using the [] operator
		 * (Can serve as a replacement for .x, .y and etc)
		 */
		constexpr T& operator[] (int i)
		{
			return data[i];
		}

		/*! Addition assignment operator */
		constexpr Vec& operator+=(const Vec& rhs)
		{
			for (decltype(R) i = 0; i < R; i++)
			{
				data[i] += rhs.data[i];
			}
			return *this;
		}

		/*! Subtraction assignment operator */
		constexpr Vec& operator-=(const Vec& rhs)
		{
			for (decltype(R) i = 0; i < R; i++)
			{
				data[i] -= rhs.data[i];
			}
			return *this;
		}

		/*! Multiplication assignment operator */
		constexpr Vec& operator*=(const Vec& rhs)
		{
			if constexpr (R == 3)
			{
				data[0] *= rhs.data[0];
				data[1] *= rhs.data[1];
				data[2] *= rhs.data[2];
				return *this;
			}
			else if constexpr (R == 4)
			{
				data[0] *= rhs.data[0];
				data[1] *= rhs.data[1];
				data[2] *= rhs.data[2];
				data[3] *= rhs.data[3];
				return *this;
			} else {
				for (decltype(R) i = 0; i < R; i++)
				{
					data[i] *= rhs.data[i];
				}
				return *this;
			}
		}

		/*! Multiplication operator */
		constexpr Vec operator*(const T& scalar)
		{
			Vec r = *this;
			for (decltype(R) i = 0; i < R; i++)
			{
				r.data[i] *= scalar;
			}
			return r;
		}

		/*! Addition operator */
		constexpr friend Vec operator+(Vec lhs, const Vec& rhs)
		{
			lhs += rhs;
			return lhs;
		}

		/*! Subtraction operator */
		constexpr friend Vec operator-(Vec lhs, const Vec& rhs)
		{
			lhs -= rhs;
			return lhs;
		}

		/*! Multiply operator */
		constexpr friend Vec operator*(Vec lhs, const Vec& rhs)
		{
			lhs *= rhs;
			return lhs;
		}

		/*! Equal to operator */
		constexpr bool operator==(const Vec& other) const
		{
			for (decltype(R) i = 0; i < R; i++)
			{
				if (data[i] != other.data[i])
				{
					return false;
				}
			}

			return true;
		}

		/*! Not equal to operator */
		constexpr bool operator!=(const Vec& other) const
		{
			for (decltype(R) i = 0; i < R; i++)
			{
				if (data[i] != other.data[i])
				{
					return true;
				}
			}

			return false;
		}

		/*! Returns the square root length of the vector. */
		constexpr T SqrtLength()
		{
			T retval = 0;

			for (decltype(R) i = 0; i < R; i++)
			{
				retval += data[i] * data[i];
			}

			return retval;
		}

		/*! Returns the length/magnitude of the vector. */
		constexpr T Length()
		{
			return std::sqrt(SqrtLength());
		}

		/*! Returns a normalized version of itself. */
		constexpr Vec Normalized()
		{
			Vec retval;
			const T l = Length();

			for (decltype(R) i = 0; i < R; i++)
			{
				retval.data[i] = data[i] / l;
			}

			return retval;
		}

		/*! Returns a normalized version of `v`. */
		constexpr static Vec Normalize(Vec v)
		{
			Vec retval;
			const T l = v.Length();

			for (decltype(R) i = 0; i < R; i++)
			{
				retval.data[i] = v.data[i] / l;
			}

			return retval;
		}

		/*! 3D Dot product between itself and another vector. */
		constexpr T Dot(const Vec& other) const
		{
			static_assert(R > 2, "`Dot` is only valid for 3D vectors");

			T retval = 0;

			for (decltype(R) i = 0; i < R; i++)
			{
				retval += data[i] * other.data[i];
			}

			return retval;
		}

		/*! 3D Cross product between itself and another vector. */
		constexpr Vec Cross(const Vec& other) const
		{
			static_assert(R > 2, "`Cross` is only valid for 3D vectors");

			Vec retval;

			retval.data[0] = data[1] * other.data[2] - data[2] * other.data[1];
			retval.data[1] = data[2] * other.data[0] - data[0] * other.data[2];
			retval.data[2] = data[0] * other.data[1] - data[1] * other.data[0];

			return retval;
		}
	};

	// float typedefs.
	using vec = Vec<float, 3>;
	using vec2 = Vec<float, 2>;
	using vec3 = Vec<float, 3>;
	using vec4 = Vec<float, 4>;

	// double typedefs.
	using dvec = Vec<double, 3>;
	using dvec2 = Vec<double, 2>;
	using dvec3 = Vec<double, 3>;
	using dvec4 = Vec<double, 4>;

	// int typedefs.
	using ivec = Vec<int, 3>;
	using ivec2 = Vec<int, 2>;
	using ivec3 = Vec<int, 3>;
	using ivec4 = Vec<int, 4>;

} /* fm */