#pragma once

#include "vector.hpp"

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

	static Vector clamp(Vector v, float min, float max)
	{
		Vector result(0, 0, 0);
		if (min == max)
			return Vector(min, min, min);
		if (min > max)
			throw new std::runtime_error("Min is greater than max.");
		
		float values[3] = {v.x, v.y, v.z};
		for (int i = 0; i < 3; i++)
		{
			if (values[i] < min)
				values[i] = min;
			if (values[i] > max)
				values[i] = max;
		}
		return Vector(values[0], values[1], values[2]);
	}

	static Vector clamp(Vector v, Vector min, Vector max)
	{
		Vector result(0, 0, 0);
		if (min == max)
			return min;
		if (min.x > max.x || min.y > max.y || min.z > max.z)
			throw new std::runtime_error("Min is greater than max.");
		
		float values[3] = {v.x, v.y, v.z};
		float minValues[3] = {min.x, min.y, min.z};
		float maxValues[3] = {max.x, max.y, max.z};
		for (int i = 0; i < 3; i++)
		{
			if (values[i] < minValues[i])
				values[i] = minValues[i];
			if (values[i] > maxValues[i])
				values[i] = maxValues[i];
		}
		return Vector(values[0], values[1], values[2]);
	}
}
