#pragma once
#ifndef Matrices_h
#define Matrices_h
#include "JuceHeader.h"
using namespace std;
using namespace juce;
using namespace dsp;
template <class temp>

class Matrices
{
public:

	void contents(Matrix<temp> a)
	{
		int numRows = a.getNumRows();
		int numCols = a.getNumColumns();

		for (int i = 0; i < numRows; i++)
		{
			for (int j = 0; j < numCols; j++)
			{
				DBG(a(i, j));
			}
		}
	}

	Matrix<temp> cofactor(Matrix<temp> a)
	{
		int size = a.getNumRows();
		Matrix<temp> out{ (size_t)size,(size_t)size };

		for (int i = 0; i < size; i++)
		{
			for (int j = 0; j < size; j++)
			{
				int elNum = size * i + j;
				out(i, j) = pow(-1, elNum) * a(i, j);
			}
		}
		return out;
	}

	Matrix<temp> transpose(Matrix<temp> a)
	{
		int size = a.getNumRows();
		Matrix<temp> out{ (size_t)size,(size_t)size };

		for (int i = 0; i < size; i++)
		{
			for (int j = 0; j < size; j++)
			{
				int elNum = size * i + j;
				out(i, j) = a(j, i);
			}
		}
			return out;
	}

	temp det2(Matrix<temp> a)
	{
		return a(0, 0) * a(1, 1) - a(0, 1) * a(1, 0);
	}

	temp det3(Matrix<temp> a)
	{
		temp det1 = a(1, 1) * a(2, 2) - a(1, 2) * a(2, 1);
		temp det2 = a(1, 0) * a(2, 2) - a(1, 2) * a(2, 0);
		temp det3 = a(1, 0) * a(2, 1) - a(1, 1) * a(2, 0);
		return a(0, 0) * det1 - a(0, 1) * det2 + a(0, 2) * det3;
	}

	Matrix<temp> inv3(Matrix<temp> a)
	{
		// Matrices for adjoint and inverse
		Matrix<temp> b{ (size_t)3, (size_t)3 };
		Matrix<temp> c{ (size_t)3, (size_t)3 };

		// calculate matrix of minors
		b(0,0) = a(1, 1) * a(2, 2) - a(1, 2) * a(2, 1);
		b(0,1) = a(1, 0) * a(2, 2) - a(1, 2) * a(2, 0);
		b(0,2) = a(1, 0) * a(2, 1) - a(1, 1) * a(2, 0);
		b(1,0) = a(0, 1) * a(2, 2) - a(0, 2) * a(2, 1);
		b(1,1) = a(0, 0) * a(2, 2) - a(0, 2) * a(2, 0);
		b(1,2) = a(0, 0) * a(2, 1) - a(0, 1) * a(2, 0);
		b(2,0) = a(0, 1) * a(1, 2) - a(0, 2) * a(1, 1);
		b(2,1) = a(0, 0) * a(1, 2) - a(0, 2) * a(1, 0);
		b(2,2) = a(0, 0) * a(1, 1) - a(0, 1) * a(1, 0);

		// cofactors and transpose
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				c(i, j) = pow(-1, 3 * i + j) * b(j, i);

		// divide by determinant
		temp det = a(0, 0) * b(0, 0) - a(0, 1) * b(0, 1) + a(0, 2) * b(0, 2);
		return c * (1.0f/det);
	}

	temp mat2float(Matrix<temp> a)
	{
		return a(0, 0);
	}

	Matrix<temp> flat2mat(temp a)
	{
		Matrix<temp> A{ (size_t)1, (size_t)1 };
		A(0, 0) = a;
		return A;
	}

	// Homemade Matrix Class
/*Copies b to a*/
	void copyTo(temp a[3][1], temp b[3][1])
	{
		for (int i = 0; i < 3; i++)
		{
			a[i][0] = b[i][0];
		}
	}
	// Multiply 3x3 by 3x1  - returns 3x1
	void multiply3x3by3x1(temp a[3][3], temp b[3][1], temp c[3][1])
	{
		for (int i = 0; i < 3; i++)
		{
			c[i][0] = 0.0f;
			for (int j = 0; j < 3; j++)
				c[i][0] += a[i][j] * b[j][0];
		}
	}

	void multiply3x3by3x3(temp a[3][3], temp b[3][3], temp c[3][3])
	{

		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				c[i][j] = 0.0f;
				for (int k = 0; k < 3; k++)
					c[i][j] += a[i][k] * b[k][j];
			}
		}
	}

	/*adds b to a*/
	void add3x1s(temp a[3][1], temp b[3][1])
	{
		for (int i = 0; i < 3 i++)
			a[i][0] += b[i][0];
	}

	// Multiply 1x3 by 3x1 - returns scalar
	temp multiply1x3by3x1(temp a[3], temp b[3][1])
	{
		temp c = 0.0f;

		for (int i = 0; i < 3; i++)
		{
			c += a[i] * b[i][0];
		}
		return c;
	}

	void multiply1x3by3x3(temp a[3], temp b[3][3], temp c[3])
	{
		for (int i = 0; i < 3; i++)
		{
			c[i] = 0.0f;
				for (int j = 0; j < 3; j++)
				{
					c[i] += a[j] * b[j][i];
				}
		}
	}

	void invert3x3(temp a[3][3])
	{
		// Matrices for adjoint and inverse
		temp b[3][3];

		// calculate matrix of minors
		b[0][0] = a[1][1] * a[2][2] - a[1][2] * a[2][1];
		b[0][1] = a[1][0] * a[2][2] - a[1][2] * a[2][0];
		b[0][2] = a[1][0] * a[2][1] - a[1][1] * a[2][0];
		b[1][0] = a[0][1] * a[2][2] - a[0][2] * a[2][1];
		b[1][1] = a[0][0] * a[2][2] - a[0][2] * a[2][0];
		b[1][2] = a[0][0] * a[2][1] - a[0][1] * a[2][0];
		b[2][0] = a[0][1] * a[1][2] - a[0][2] * a[1][1];
		b[2][1] = a[0][0] * a[1][2] - a[0][2] * a[1][0];
		b[2][2] = a[0][0] * a[1][1] - a[0][1] * a[1][0];

		temp det = a[0][0] * b[0][0] - a[0][1] * b[0][1] + a[0][2] * b[0][2];

		// cofactors and transpose
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
			{
				a[i][j] = powf(-1, 3 * i + j) * b[j][i];
				a[i][j] /= det;
			}		
	}
};



#endif // !Matrices_h



