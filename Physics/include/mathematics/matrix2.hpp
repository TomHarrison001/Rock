#pragma once

namespace Rock
{
	class Matrix2
	{
	private:
		Vector2 m_rows[2];
	public:
		Matrix2() = default;
		Matrix2(float x)
		{
			m_rows[0] = Vector2(x);
			m_rows[1] = Vector2(x);
		}
		Matrix2(Vector2 x, Vector2 y)
		{
			m_rows[0] = x;
			m_rows[1] = y;
		}
		Matrix2(float x1, float y1, float x2, float y2)
		{
			m_rows[0] = Vector2(x1, y1);
			m_rows[1] = Vector2(x2, y2);
		}
	public:
		Vector2 getRow(uint32_t index) const
		{
			return Vector2(m_rows[index][0], m_rows[index][1]);
		}

		Vector2 getColumn(uint32_t index) const
		{
			return Vector2(m_rows[0][index], m_rows[1][index]);
		}

		Matrix2 operator-() const
		{
			return Matrix2(-m_rows[0], -m_rows[1]);
		}

		Matrix2 operator-(Matrix2& other) const
		{
			return Matrix2(this->m_rows[0] - other.getRow(0),
				this->m_rows[1] - other.getRow(1));
		}

		void operator-=(Matrix2& other)
		{
			this->m_rows[0] -= other.getRow(0);
			this->m_rows[1] -= other.getRow(1);
		}

		Matrix2 operator+(Matrix2& other) const
		{
			return Matrix2(this->m_rows[0] + other.getRow(0),
				this->m_rows[1] + other.getRow(1));
		}

		void operator+=(Matrix2& other)
		{
			this->m_rows[0] += other.getRow(0);
			this->m_rows[1] += other.getRow(1);
		}

		Matrix2 operator*(float& n) const
		{
			return Matrix2(this->m_rows[0] * n,
				this->m_rows[1] * n);
		}

		Matrix2 operator*(Matrix2& other) const
		{
			return Matrix2(
				this->m_rows[0][0] * other.getRow(0)[0] + this->m_rows[0][1] * other.getRow(1)[0],
				this->m_rows[0][0] * other.getRow(0)[1] + this->m_rows[0][1] * other.getRow(1)[1],
				this->m_rows[1][0] * other.getRow(0)[0] + this->m_rows[1][1] * other.getRow(1)[0],
				this->m_rows[1][0] * other.getRow(0)[1] + this->m_rows[1][1] * other.getRow(1)[1]
			);
		}

		void operator*=(float& n)
		{
			this->m_rows[0] *= n;
			this->m_rows[1] *= n;
		}

		Vector2 operator[](const uint32_t index) const
		{
			return m_rows[index];
		}

		bool operator==(Matrix2& other) const
		{
			return this->m_rows[0] == other.getRow(0) && this->m_rows[1] == other.getRow(1);
		}

		Matrix2 getTranspose() const
		{
			return Matrix2(
				m_rows[0][0], m_rows[1][0],
				m_rows[0][1], m_rows[1][1]
			);
		}

		float getDeterminant() const
		{
			return m_rows[0][0] * m_rows[1][1] - m_rows[0][1] * m_rows[1][0];
		}
	};
}
