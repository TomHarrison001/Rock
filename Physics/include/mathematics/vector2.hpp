#pragma once

namespace Rock
{
	struct Vector2
	{
		float x;
		float y;

		Vector2() = default;
		Vector2(float x, float y) : x(x), y(y) {}
		Vector2(float x) : Vector2(x, x) {}

		std::string toString() const
		{
			std::stringstream ss;
			ss << "{" << this->x << ", " << this->y << "}";
			return ss.str();
		}

		Vector2 operator-() const
		{
			return { this->x * -1.f, this->y * -1.f };
		};

		Vector2 operator-(Vector2& other) const
		{
			return { this->x - other.x, this->y - other.y };
		};

		void operator-=(Vector2& other)
		{
			this->x -= other.x;
			this->y -= other.y;
		};

		Vector2 operator+(Vector2& other) const
		{
			return { this->x + other.x, this->y + other.y };
		};

		void operator+=(Vector2& other)
		{
			this->x += other.x;
			this->y += other.y;
		};

		Vector2 operator*(Vector2& other) const
		{
			return { this->x * other.x, this->y * other.y };
		}

		Vector2 operator*(float n) const
		{
			return { this->x * n, this->y * n };
		}

		void operator*=(Vector2& other)
		{
			this->x *= other.x;
			this->y *= other.y;
		}

		void operator*=(float n)
		{
			this->x *= n;
			this->y *= n;
		}

		Vector2 operator/(Vector2& other) const
		{
			return {
				(other.x == 0.f) ? 0.f : this->x / other.x,
				(other.y == 0.f) ? 0.f : this->y / other.y
			};
		}

		Vector2 operator/(float n) const
		{
			if (n == 0)
				return { 0.f };
			return { this->x / n, this->y / n };
		}

		void operator/=(Vector2& other)
		{
			this->x = (other.x == 0.f) ? 0.f : this->x / other.x;
			this->y = (other.y == 0.f) ? 0.f : this->y / other.y;
		}

		void operator/=(float n)
		{
			this->x = (n == 0.f) ? 0.f : this->x / n;
			this->y = (n == 0.f) ? 0.f : this->y / n;
		}

		float operator[](const uint32_t index) const
		{
			return (&x)[index];
		}

		bool operator==(const Vector2& other) const
		{
			return this->x == other.x && this->y == other.y;
		}

		float length() const
		{
			return sqrt(this->x * this->x + this->y * this->y);
		}

		Vector2 normalise() const
		{
			float len = this->length();
			return { this->x / len, this->y / len };
		}

		float distance(Vector2& v) const
		{
			return Vector2(this->x - v.x, this->y - v.y).length();
		}

		float dot(Vector2& v) const
		{
			return this->x * v.x + this->y * v.y;
		}

		void clamp(Vector2& min, Vector2& max)
		{
			this->x = Rock::clamp(this->x, min.x, max.x);
			this->y = Rock::clamp(this->y, min.y, max.y);
		}

		void clamp(float min, float max)
		{
			this->clamp(Vector2(min, min), Vector2(max, max));
		}

		void clamp(int min, int max)
		{
			this->clamp(static_cast<float>(min), static_cast<float>(max));
		}
	};

	// a • b = |a||b| cos x
	static bool areParallel(const Vector2& v1, Vector2& v2)
	{
		return abs(v1.dot(v2)) > 0.9999f;
	}

	// a • b = |a||b| cos x
	static bool areOrthogonal(const Vector2& v1, Vector2& v2)
	{
		return abs(v1.dot(v2)) < 0.0001f;
	}
}
