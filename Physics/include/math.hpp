#pragma once

namespace Rock
{
	static int clamp(int x, int min, int max)
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
}
