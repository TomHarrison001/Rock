#pragma once

namespace Rock
{
	class Matrix3
	{
	private:
		Vector3 m_rows[3];
	public:
		Matrix3() = default;
		Matrix3(float x)
		{
			m_rows[0] = Vector3(x);
			m_rows[1] = Vector3(x);
			m_rows[2] = Vector3(x);
		}
		Matrix3(Vector3 x, Vector3 y, Vector3 z)
		{
			m_rows[0] = x;
			m_rows[1] = y;
			m_rows[2] = z;
		}
		Matrix3(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3)
		{
			m_rows[0] = Vector3(x1, y1, z1);
			m_rows[1] = Vector3(x2, y2, z2);
			m_rows[2] = Vector3(x3, y3, z3);
		}
	public:
		Vector3 getRow(uint32_t index) const
		{
			return Vector3(m_rows[index][0], m_rows[index][1], m_rows[index][2]);
		}

		Vector3 getColumn(uint32_t index) const
		{
			return Vector3(m_rows[0][index], m_rows[1][index], m_rows[2][index]);
		}

		Matrix3 operator-() const
		{
			return Matrix3(-m_rows[0], -m_rows[1], -m_rows[2] );
		}

		Matrix3 operator-(Matrix3& other) const
		{
			return Matrix3(this->m_rows[0] - other.getRow(0),
				this->m_rows[1] - other.getRow(1),
				this->m_rows[2] - other.getRow(2));
		}

		void operator-=(Matrix3& other)
		{
			this->m_rows[0] -= other.getRow(0);
			this->m_rows[1] -= other.getRow(1);
			this->m_rows[2] -= other.getRow(2);
		}

		Matrix3 operator+(Matrix3& other) const
		{
			return Matrix3(this->m_rows[0] + other.getRow(0),
				this->m_rows[1] + other.getRow(1),
				this->m_rows[2] + other.getRow(2));
		}

		void operator+=(Matrix3& other)
		{
			this->m_rows[0] += other.getRow(0);
			this->m_rows[1] += other.getRow(1);
			this->m_rows[2] += other.getRow(2);
		}

		Matrix3 operator*(float& n) const
		{
			return Matrix3(this->m_rows[0] * n,
				this->m_rows[1] * n,
				this->m_rows[2] * n);
		}

		Matrix3 operator*(Matrix3& other) const
		{
			return Matrix3(
				this->m_rows[0][0] * other.getRow(0)[0] + this->m_rows[0][1] * other.getRow(1)[0] + this->m_rows[0][2] * other.getRow(2)[0],
				this->m_rows[0][0] * other.getRow(0)[1] + this->m_rows[0][1] * other.getRow(1)[1] + this->m_rows[0][2] * other.getRow(2)[1],
				this->m_rows[0][0] * other.getRow(0)[2] + this->m_rows[0][1] * other.getRow(1)[2] + this->m_rows[0][2] * other.getRow(2)[2],
				this->m_rows[1][0] * other.getRow(0)[0] + this->m_rows[1][1] * other.getRow(1)[0] + this->m_rows[1][2] * other.getRow(2)[0],
				this->m_rows[1][0] * other.getRow(0)[1] + this->m_rows[1][1] * other.getRow(1)[1] + this->m_rows[1][2] * other.getRow(2)[1],
				this->m_rows[1][0] * other.getRow(0)[2] + this->m_rows[1][1] * other.getRow(1)[2] + this->m_rows[1][2] * other.getRow(2)[2],
				this->m_rows[2][0] * other.getRow(0)[0] + this->m_rows[2][1] * other.getRow(1)[0] + this->m_rows[2][2] * other.getRow(2)[0],
				this->m_rows[2][0] * other.getRow(0)[1] + this->m_rows[2][1] * other.getRow(1)[1] + this->m_rows[2][2] * other.getRow(2)[1],
				this->m_rows[2][0] * other.getRow(0)[2] + this->m_rows[2][1] * other.getRow(1)[2] + this->m_rows[2][2] * other.getRow(2)[2]
			);
		}

		void operator*=(float& n)
		{
			this->m_rows[0] *= n;
			this->m_rows[1] *= n;
			this->m_rows[2] *= n;
		}

		Vector3 operator[](const uint32_t index) const
		{
			return m_rows[index];
		}

		bool operator==(Matrix3& other) const
		{
			return this->m_rows[0] == other.getRow(0) && this->m_rows[1] == other.getRow(1) && this->m_rows[2] == other.getRow(2);
		}

		Matrix3 getTranspose() const
		{
			return Matrix3(
				m_rows[0][0], m_rows[1][0], m_rows[2][0],
				m_rows[0][1], m_rows[1][1], m_rows[2][1],
				m_rows[0][2], m_rows[1][2], m_rows[2][2]
			);
		}

		float getDeterminant() const
		{
			return (
				m_rows[0][0] * (m_rows[1][1] * m_rows[2][2] - m_rows[2][1] * m_rows[1][2]) -
				m_rows[0][1] * (m_rows[1][0] * m_rows[2][2] - m_rows[2][0] * m_rows[1][2]) +
				m_rows[0][2] * (m_rows[1][0] * m_rows[2][1] - m_rows[2][0] * m_rows[1][1])
			);
		}
	};
}
