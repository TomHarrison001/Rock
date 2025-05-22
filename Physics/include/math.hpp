#pragma once

namespace Rock
{
	static float clamp(float x, float min, float max)
	{
		if (min == max)
			return min;
		if (min > max)
			throw new std::runtime_error("Min is greater than max.");
		if (x < min)
			return min;
		if (x > max)
			return max;
		return x;
	}

	static int clamp(int x, int min, int max)
	{
		float result = clamp(static_cast<float>(x), static_cast<float>(min), static_cast<float>(max));
		return static_cast<int>(result);
		}
}
