#pragma once

#include <cmath>

namespace fm
{

	namespace storage
	{
		template<class T, int R>
		struct Vec
		{
			union
			{
				T data[R];
			};
		};

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

	template<class T = float, unsigned int R = 3>
	class Vec : public storage::Vec<T, R>
	{
	public:
		using storage::Vec<T, R>::data;

		constexpr Vec(T x, T y) : storage::Vec<T, R>{ x, y } {}
		constexpr Vec(T x, T y, T z) : storage::Vec<T, R>{ x, y, z } {}
		constexpr Vec(T x, T y, T z, T w) : storage::Vec<T, R>{ x, y, z, w } {}

		constexpr Vec()
		{
			for (decltype(R) i = 0; i < R; i++)
			{
				data[i] = 0;
			}
		}

		constexpr explicit Vec(T data[R])
		{
			memcpy(this->data, data, sizeof(T) * R);
		}

		~Vec() = default;

		constexpr T& operator[] (int i)
		{
			return data[i];
		}

		constexpr Vec& operator+=(const Vec& rhs)
		{
			for (decltype(R) i = 0; i < R; i++)
			{
				data[i] += rhs.data[i];
			}
			return *this;
		}

		constexpr Vec& operator-=(const Vec& rhs)
		{
			for (decltype(R) i = 0; i < R; i++)
			{
				data[i] -= rhs.data[i];
			}
			return *this;
		}

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

		constexpr Vec operator*(const T& scalar)
		{
			Vec r = *this;
			for (decltype(R) i = 0; i < R; i++)
			{
				r.data[i] *= scalar;
			}
			return r;
		}

		constexpr friend Vec operator+(Vec lhs, const Vec& rhs)
		{
			lhs += rhs;
			return lhs;
		}

		constexpr friend Vec operator-(Vec lhs, const Vec& rhs)
		{
			lhs -= rhs;
			return lhs;
		}

		constexpr friend Vec operator*(Vec lhs, const Vec& rhs)
		{
			lhs *= rhs;
			return lhs;
		}

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

		constexpr T SqrtLength()
		{
			T retval = 0;

			for (decltype(R) i = 0; i < R; i++)
			{
				retval += data[i] * data[i];
			}

			return retval;
		}

		constexpr T Length()
		{
			return std::sqrt(SqrtLength());
		}

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

		constexpr T Dot(const Vec& other) const
		{
			T retval = 0;

			for (decltype(R) i = 0; i < R; i++)
			{
				retval += data[i] * other.data[i];
			}

			return retval;
		}

		// 3D Cross product.
		constexpr Vec Cross(const Vec& other) const
		{
			Vec retval;

			retval.data[0] = data[1] * other.data[2] - data[2] * other.data[1];
			retval.data[1] = data[2] * other.data[0] - data[0] * other.data[2];
			retval.data[2] = data[0] * other.data[1] - data[1] * other.data[0];

			return retval;
		}
	};

	// typedefs
	using vec = Vec<float, 3>;
	using vec2 = Vec<float, 2>;
	using vec3 = Vec<float, 3>;
	using vec4 = Vec<float, 4>;

	using dvec = Vec<double, 3>;
	using dvec2 = Vec<double, 2>;
	using dvec3 = Vec<double, 3>;
	using dvec4 = Vec<double, 4>;

	using ivec = Vec<int, 3>;
	using ivec2 = Vec<int, 2>;
	using ivec3 = Vec<int, 3>;
	using ivec4 = Vec<int, 4>;

} /* fm */