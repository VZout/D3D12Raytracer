#pragma once

#include <math.h>
#include <random>

#include "vec.hpp"

namespace fm
{
	inline float cot(float val)
	{
		return(1 / tan(val));
	}

	inline float frandr(float min, float max)
	{
		return min + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (max - min)));
	}

	inline float fmap(float s, float a1, float a2, float b1, float b2)
	{
		return b1 + (s - a1)*(b2 - b1) / (a2 - a1);
	}

	inline fm::vec3 clamp(fm::vec3 in, float min, float max)
	{
		fm::vec3 out = in;

		out.x = std::clamp(in.x, min, max);
		out.y = std::clamp(in.y, min, max);
		out.z = std::clamp(in.z, min, max);

		return out;
	}

	template<typename T = float>
	inline T rads(T deg)
	{
		return deg * 0.0174532925;
	}

	template<typename T = float>
	inline T lerp(T a, T b, float f)
	{
		return a + f * (b - a);
	}

	template<typename T = float, int R = 3>
	inline Vec<T, R> lerp(Vec<T, R> a, Vec<T, R> b, float f)
	{
		Vec<T, R> retval;

		for (auto i = 0; i < R; i++)
		{
			retval.data[i] = a.data[i] + f * (b.data[i] - a.data[i]);
		}

		return retval;
	}
} /* fm */
