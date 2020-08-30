/*-----------------------------------------------------------------------
 ALIASING REDUCTION IN VIRTUAL ANALOGUE MODELLING
 Alistair Carson 2020
 MSc Acoustics & Music Technology
 University of Edinburgh
--------------------------------------------------------------------*/
#pragma once
#ifndef Matrices_h
#define Matrices_h
using namespace std;
template <class temp>

/*
DIY Matrix class for 3x3 matrices and 3x1 vectors
*/
class Matrices
{
public:
	/*Copies b to a, 3x1 vector*/
	void copyTo(temp a[3][1], temp b[3][1])
	{
		for (int i = 0; i < 3; i++)
		{
			a[i][0] = b[i][0];
		}
	}

	/*
	Multiply 3x3 by 3x1, returns 3x1.
	c = a x b
	*/
	void multiply3x3by3x1(temp a[3][3], temp b[3][1], temp c[3][1])
	{
		for (int i = 0; i < 3; i++)
		{
			c[i][0] = 0.0f;
			for (int j = 0; j < 3; j++)
				c[i][0] += a[i][j] * b[j][0];
		}
	}

	/*
	Multiply 3x3 by 3x3, returns 3x3
	c = a xb
	*/
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

	/*Adds b to a*/
	void add3x1s(temp a[3][1], temp b[3][1])
	{
		for (int i = 0; i < 3; i++)
			a[i][0] += b[i][0];
	}

	/*
	Multiply 1x3 by 3x1, returns scalar
	c = a x b
	*/ 
	temp multiply1x3by3x1(temp a[3], temp b[3][1])
	{
		temp c = 0.0f;

		for (int i = 0; i < 3; i++)
		{
			c += a[i] * b[i][0];
		}
		return c;
	}

	/*
	Multiply 1x3 by 3x3, returns 1x3
	c = a x b
	*/
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

	/*Inverts a 3x3 matrix inplace*/
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

	/*
	Adds vectors of specific length
	*/
	temp* addVectors(temp* a, temp* b, size_t length)
	{
		temp *c = a;
		for (int i = 0; i < length; i++)
		{
			c[i] = a[i] + b[i];
		}

		return c;
	}
};



#endif // !Matrices_h