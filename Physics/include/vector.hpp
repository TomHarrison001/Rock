#pragma once

#include <string>
#include <sstream>

namespace Rock
{
	class Vector
	{
	public:
		float x;
		float y;
		float z;

		Vector(float x, float y, float z) : x(x), y(y), z(z) { }
		Vector(float x, float y) : Vector(x, y, 0.f) { }
		Vector(float x) : Vector(x, 0.f, 0.f) { }

		std::string toString() const const
		{
			std::stringstream ss;
			ss << "{" << this->x << ", " << this->y << ", " << this->z << "}";
			return ss.str();
		}

		Vector operator-() const
		{
			return Vector(this->x * -1.f, this->y * -1.f, this->z * -1.f);
		};

		Vector operator-(Vector& other) const
		{
			return Vector(this->x - other.x, this->y - other.y, this->z - other.z);
		};

		void operator-=(Vector& other)
		{
			this->x -= other.x;
			this->y -= other.y;
			this->z -= other.z;
		};

		Vector operator+(Vector& other) const
		{
			return Vector(this->x + other.x, this->y + other.y, this->z + other.z);
		};

		void operator+=(Vector& other)
		{
			this->x += other.x;
			this->y += other.y;
			this->z += other.z;
		};

		Vector operator*(Vector& other) const
		{
			return Vector(this->x * other.x, this->y * other.y, this->z * other.z);
		}

		Vector operator*(float& n) const
		{
			return Vector(this->x * n, this->y * n, this->z * n);
		}

		void operator*=(Vector& other)
		{
			this->x *= other.x;
			this->y *= other.y;
			this->z *= other.z;
		}

		void operator*=(float n)
		{
			this->x *= n;
			this->y *= n;
			this->z *= n;
		}

		Vector operator/(Vector& other) const
		{
			return Vector(
				(other.x == 0.f) ? 0.f : this->x / other.x,
				(other.y == 0.f) ? 0.f : this->y / other.y,
				(other.z == 0.f) ? 0.f : this->z / other.z
			);
		}

		Vector operator/(float n) const
		{
			if (n == 0)
				return Vector(0.f);
			return Vector(this->x / n, this->y / n, this->z / n);
		}

		void operator/=(Vector& other)
		{
			this->x = (other.x == 0.f) ? 0.f : this->x / other.x;
			this->y = (other.y == 0.f) ? 0.f : this->y / other.y;
			this->z = (other.z == 0.f) ? 0.f : this->z / other.z;
		}

		void operator/=(float n)
		{
			this->x = (n == 0.f) ? 0.f : this->x / n;
			this->y = (n == 0.f) ? 0.f : this->y / n;
			this->z = (n == 0.f) ? 0.f : this->z / n;
		}

		bool operator==(const Vector& other) const
		{
			return this->x == other.x && this->y == other.y && this->z == other.z;
		}

		float length()
		{
			return sqrt(this->x * this->x + this->y * this->y + this->z * this->z);
		}

		Vector normalise()
		{
			float len = this->length();
			return Vector(this->x / len, this->y / len, this->z / len);
		}

		float distance(Vector v)
		{
			return Vector(this->x - v.x, this->y - v.y, this->z - v.z).length();
		}

		float dot(Vector v)
		{
			return this->x * v.x + this->y * v.y + this->z * v.z;
		}

		Vector cross(Vector v)
		{
			return Vector(
				this->y * v.z - this->z * v.y,
				this->z * v.x - this->x * v.z,
				this->x * v.y - this->y * v.x
			);
		}
	};
}
