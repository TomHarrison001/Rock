#pragma once

namespace Rock
{
	struct Vector3
	{
		float x;
		float y;
		float z;

		Vector3() = default;
		Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
		Vector3(float x) : Vector3(x, x, x) {}

		std::string toString() const
		{
			std::stringstream ss;
			ss << "{" << this->x << ", " << this->y << ", " << this->z << "}";
			return ss.str();
		}

		Vector3 operator-() const
		{
			return { this->x * -1.f, this->y * -1.f, this->z * -1.f };
		};

		Vector3 operator-(Vector3& other) const
		{
			return { this->x - other.x, this->y - other.y, this->z - other.z };
		};

		void operator-=(Vector3& other)
		{
			this->x -= other.x;
			this->y -= other.y;
			this->z -= other.z;
		};

		Vector3 operator+(Vector3& other) const
		{
			return { this->x + other.x, this->y + other.y, this->z + other.z };
		};

		void operator+=(Vector3& other)
		{
			this->x += other.x;
			this->y += other.y;
			this->z += other.z;
		};

		Vector3 operator*(float n) const
		{
			return { this->x * n, this->y * n, this->z * n };
		}

		Vector3 operator*(Vector3& other) const
		{
			return { this->x * other.x, this->y * other.y, this->z * other.z };
		}

		void operator*=(float n)
		{
			this->x *= n;
			this->y *= n;
			this->z *= n;
		}

		void operator*=(Vector3& other)
		{
			this->x *= other.x;
			this->y *= other.y;
			this->z *= other.z;
		}

		Vector3 operator/(float n) const
		{
			if (n == 0)
				return { 0.f };
			return { this->x / n, this->y / n, this->z / n };
		}

		Vector3 operator/(Vector3& other) const
		{
			return {
				(other.x == 0.f) ? 0.f : this->x / other.x,
				(other.y == 0.f) ? 0.f : this->y / other.y,
				(other.z == 0.f) ? 0.f : this->z / other.z
			};
		}

		void operator/=(float n)
		{
			this->x = (n == 0.f) ? 0.f : this->x / n;
			this->y = (n == 0.f) ? 0.f : this->y / n;
			this->z = (n == 0.f) ? 0.f : this->z / n;
		}

		void operator/=(Vector3& other)
		{
			this->x = (other.x == 0.f) ? 0.f : this->x / other.x;
			this->y = (other.y == 0.f) ? 0.f : this->y / other.y;
			this->z = (other.z == 0.f) ? 0.f : this->z / other.z;
		}

		float operator[](const uint32_t index) const
		{
			return (&x)[index];
		}

		bool operator==(const Vector3& other) const
		{
			return this->x == other.x && this->y == other.y && this->z == other.z;
		}

		float length() const
		{
			return sqrt(this->x * this->x + this->y * this->y + this->z * this->z);
		}

		Vector3 normalise() const
		{
			float len = this->length();
			return { this->x / len, this->y / len, this->z / len };
		}

		float distance(Vector3& v) const
		{
			return Vector3(this->x - v.x, this->y - v.y, this->z - v.z).length();
		}

		float dot(Vector3& v) const
		{
			return this->x * v.x + this->y * v.y + this->z * v.z;
		}

		Vector3 cross(Vector3& v) const
		{
			return {
				this->y * v.z - this->z * v.y,
				this->z * v.x - this->x * v.z,
				this->x * v.y - this->y * v.x
			};
		}

		void clamp(Vector3& min, Vector3& max)
		{
			this->x = Rock::clamp(this->x, min.x, max.x);
			this->y = Rock::clamp(this->y, min.y, max.y);
			this->z = Rock::clamp(this->z, min.z, max.z);
		}

		void clamp(float min, float max)
		{
			this->clamp(Vector3(min), Vector3(max));
		}

		void clamp(int min, int max)
		{
			this->clamp(static_cast<float>(min), static_cast<float>(max));
		}
	};

	// a x b = n • |a||b| sin x
	static bool areParallel(const Vector3& v1, Vector3& v2)
	{
		return v1.cross(v2).length() < 0.0001f;
	}

	// a • b = |a||b| cos x
	static bool areOrthogonal(const Vector3& v1, Vector3& v2)
	{
		return abs(v1.dot(v2)) < 0.0001f;
	}
}
