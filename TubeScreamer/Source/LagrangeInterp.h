#pragma once
#ifndef LagrangeInterp_h
#define LagrangeInterp_h
template <class temp>

class LagrangeInterp
{
public:

	LagrangeInterp(size_t order = 3)
	{
		setOrder(order);
	}

	void setOrder(size_t order)
	{
		nNN = order + 1;
		indices = new int[nNN];
		x_ = new temp[nNN];
		y_ = new temp[nNN];
		for (int i = 0; i < nNN; i++)
		{
			indices[i] = 0;
			x_[i] = 0.0;
			y_[i] = 0.0;
		}
	}

	void setTableSize(size_t tableSize)
	{
		L = tableSize;
	}

	temp lookUp(temp* x, temp* y, temp xq)
	{
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
		temp p = 1.0;

		for (int i = 0; i < nNN; i++)
		{
			int ind = indices[i];
			x_[i] = x[ind];
			y_[i] = y[ind];
		}

		for (int i = 0; i < nNN; i++)
		{
			p = 1.0;
			for (int j = 0; j < nNN; j++)
			{
				if (j != i)
					p *= (xq - x_[j]) / (x_[i] - x_[j]);
			}
			yq += y_[i] * p;
		}

		return yq;
	}

private:
	size_t nNN;	// number of nearest neighbours
	int* indices;
	size_t L;
	temp* x_;
	temp* y_;
};

#endif // LagrangeInterp_h