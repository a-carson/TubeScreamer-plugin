/*-----------------------------------------------------------------------
 ALIASING REDUCTION IN VIRTUAL ANALOGUE MODELLING
 Alistair Carson 2020
 MSc Acoustics & Music Technology
 University of Edinburgh
--------------------------------------------------------------------*/
#pragma once
#ifndef LagrangeInterp_h
#define LagrangeInterp_h
template <class temp>

/*
Lagrange Look-up/Interpolation class
*/
class LagrangeInterp
{
public:
	/*Constructor*/
	LagrangeInterp(size_t order = 3)
	{
		setOrder(order);
	}

	/*Sets the order of lagrange interpolation*/
	void setOrder(size_t order)
	{
		nNN = order + 1;
		indices = new int[nNN];
		for (int i = 0; i < nNN; i++)
		{
			indices[i] = 0;
		}
	}

	/*Informs the class of the look-up table size*/
	void setTableSize(size_t tableSize)
	{
		L = tableSize;
	}

	/*
	Lagrange look-up function:
	Input params:
	x - x-data
	y - y-data
	xq - query sample
	*/
	temp lookUp(temp* x, temp* y, temp xq)
	{
		// Get nearest neighbours
		temp indBet = (xq - x[0]) / (x[1] - x[0]);
		int indBetFloored = floorf(indBet);

		for (int i = 0; i < nNN; i++)
			indices[i] = indBetFloored + i + 1 - nNN/2;

		if (indices[nNN - 1] > (L - 1))
			for (int i = 0; i < nNN; i++)
				indices[i] -= indices[nNN-1] - L + 1;

		if (indices[0] < 0)
			for (int i = 0; i < nNN; i++)
				indices[nNN - 1 - i] -= indices[0];

		temp yq = 0.0;

		// Lagrange interpolation
		for (int i = 0; i < nNN; i++)
		{
			temp p = 1.0;
			int indI = indices[i];

			for (int j = 0; j < nNN; j++)
			{
				int indJ = indices[j];

				if (j != i)
					p *= (xq - x[indJ]) / (x[indI] - x[indJ]);
			}
			yq += y[indI] * p;
		}

		return yq;
	}

private:
	size_t nNN;		// number of nearest neighbours
	int* indices;	// nearest neighbours array
	size_t L;		// look-up table size
};

#endif // LagrangeInterp_h