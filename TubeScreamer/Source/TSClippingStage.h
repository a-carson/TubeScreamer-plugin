#pragma once
#ifndef TSClippingStage_h
#define TSClippingStage_h
#include "JuceHeader.h"
#include "Matrices.h"

#include <cmath>
using namespace juce;
using namespace dsp;

template<class temp>

class TSClippingStage
{
public:

	/*Enumerator class for setting the clipping type*/
	enum class ClippingType
	{
		symmetric,
		asymmetric
	};

	TSClippingStage(ClippingType type)
	{
		clippingType = type;
	};


	/*Set sample rate*/
	void setSampleRate(temp sampleRate)
	{
		fs = sampleRate;
	}

	/*Set distortion amount of pedal*/
	void setDistortion(temp distortion)
	{
		r2 = 51e3 + distortion* 500e3;
		A[1][1] = -1.0f / (r2 * c2);
		updateStateSpaceArrays();
	}

	/*Set diode parameters*/
	void setDiodeParameters(temp saturationCurrent, temp thermalVoltage, temp idealityFactor)
	{
		Is = saturationCurrent;
		Vt = thermalVoltage;
		Ni = idealityFactor;
	}

	/*Updates state space arrays*/
	void updateStateSpaceArrays()
	{
		temp AplusI[3][3];

		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				if (i == j)
					I[i][i] = 2.0f * fs;
				else
					I[i][j] = 0.0f;

				Z[i][j] = I[i][j] - A[i][j];
				AplusI[i][j] = A[i][j] + I[i][j];
			}
		}

		// Matrix multiplications
		matTool.invert3x3(Z);						// Z
		matTool.multiply3x3by3x3(AplusI, Z, A_);	// A_
		matTool.multiply3x3by3x1(Z, B, B_);			// B_
		matTool.multiply3x3by3x1(Z, C, C_);			// C_
		matTool.multiply1x3by3x3(D, Z, D_);			// D_
		E_ = E + matTool.multiply1x3by3x1(D, B_);	// E_
		F_ = matTool.multiply1x3by3x1(D, C_);		// F_
		matTool.multiply1x3by3x3(G, Z, G_);			// G_
		H_ = B_[1][0];								// H_
		K_ = C_[1][0];								// K_		

		// Scalar multiplication
		for (int i = 0; i < 3; i++)
		{
			B_[i][0] *= 2.0f;
			C_[i][0] *= 2.0f;
			D_[i] *= 2.0f * fs;
			G_[i] *= 2.0f * fs;
		}

		// update Newton cap
		cap = capFunc(K_);
	}

	/*Generates an N size look-up table*/
	void makeLookUpTable(size_t numPoints, temp sampleRate, temp pmax, temp distortion)
	{
		N = numPoints;
		pLut = new temp[N];
		iLut = new temp[N];
		adLut = new temp[N];

		setSampleRate(sampleRate);
		setDistortion(distortion);

		temp dP = 2.0 * pmax / (temp)(N - 1);
		temp p0 = -1.0 * pmax;
		temp p1 = p0;
		temp y = 0.0;

		// f(p) look-up table
		for (int i = 0; i < N; i++)
		{
			p0 = -pmax + i * dP;
			pLut[i] = p0;
			y = cappedNewton(y, p0);
			iLut[i] = (y - p0) / K_;
		}

		// Trapezoid Integration - ad(p) look-up table
		temp ad = 0.0;
		p1 = -pmax;
		temp i0 = lookUp(pLut, iLut, 0.0);
		for (int i = 0; i < N; i++)
		{
			iLut[i] -= i0;

			if (i > 0)
				ad += 0.5 * dP * (iLut[i] + iLut[i - 1]);
			adLut[i] = ad;
		}

		// Adjust offset
		temp ad0 = lookUp(pLut, adLut, 0.0);
		for (int i = 0; i < N; i++)
		{
			adLut[i] -= ad0;
		}


	}

	/*Cubic Lagrange look-up*/
	temp lookUp(temp* x, temp* y, temp xq)
	{
		int indices[4] = { -1, 0, 1, 2 };
		temp indBet = (xq - x[0]) / (x[1] - x[0]);
		int indBetFloored = floorf(indBet);

		for (int i = 0; i < 4; i++)
			indices[i] += indBetFloored;


		if (indices[3] > (N - 1))
			for (int i = 0; i < 4; i++)
				indices[i] -= indices[3] - N + 1;

		if (indices[0] < 0)
			for (int i = 0; i < 4; i++)
				indices[3 - i] -= indices[0];


		temp alpha = indBet - indices[2] + 0.5;

		temp P[4] = { (alpha + 0.5) * (alpha - 0.5) * (alpha - 1.5) / -6.0 ,
		(alpha + 1.5) * (alpha - 0.5) * (alpha - 1.5) / 2.0,
		(alpha + 0.5) * (alpha - 1.5) * (alpha + 1.5) / -2.0,
		(alpha + 0.5) * (alpha - 0.5) * (alpha + 1.5) / 6.0 };

		temp yq = 0.0;

		for (int i = 0; i < 4; i++)
		{
			int ind = indices[i];
			yq += P[i] * y[ind];
		}

		return yq;
	}

	/*Regular process - without any aliasing mitigation*/
	temp process(temp in, bool useLut)
	{
		// Input
		const temp p = matTool.multiply1x3by3x1(G_, x) + H_ * in;

		// Solve non-linearity
		temp iv = 0.0;
		if (useLut)
		{
			iv = lookUp(pLut, iLut, p);
		}
		else
		{
			v = newIterate(p);
			v = cappedNewton(v, p);
			iv = (v - p) / K_;
		}

		// State update
		temp xTemp[3][1];
		matTool.multiply3x3by3x1(A_, xPrev, xTemp);
		for (int i = 0; i < 3; i++)
		{
			x[i][0] = xTemp[i][0] + (B_[i][0] * in) + (C_[i][0] * iv);
		}

		// Calculate output
		float out = matTool.multiply1x3by3x1(D_, xPrev) + E_ * in + F_ * iv;

		matTool.copyTo(xPrev, x);
		return out;
	}

	/*Process with first order anti-derivative anti-aliasing*/
	temp antiAliasedProcess(temp in)
	{
		// Input
		const temp p = matTool.multiply1x3by3x1(G_, x) + H_ * in;
		temp iv = 0.0;
		temp ad = lookUp(pLut, adLut, p);

		if (fabsf(p - pPrev) > 1.0e-8)
			iv = (ad - adPrev) / (p - pPrev);
		else
			iv = lookUp(pLut, iLut, 0.5 * (p + pPrev));

		// update state variable

		temp xCombined[3][1] = { {0.0}, {0.0}, {0.0} };
		matTool.add3x1s(xCombined, xPrev);
		matTool.add3x1s(xCombined, x2Prev);
		temp inCombined = 0.5 * (in + inPrev);


		temp xTemp[3][1];
		matTool.multiply3x3by3x1(A_, xCombined, xTemp);
		for (int i = 0; i < 3; i++)
		{
			x[i][0] = 0.5 * xTemp[i][0] + (B_[i][0] * inCombined) + (C_[i][0] * iv);
		}

		// output
		temp out = 0.5 * matTool.multiply1x3by3x1(D_, xCombined) + E_ * inCombined + F_ * iv;


		matTool.copyTo(x2Prev, xPrev);
		matTool.copyTo(xPrev, x);
		inPrev = in;
		pPrev = p;
		adPrev = ad;
		return out;
	}

	/*Sets the clipping type*/
	void setClippingType(ClippingType type)
	{
		clippingType = type;
	}

	private:
	/*Capped Newtons method*/
	temp cappedNewton(temp y, temp p)
	{
		temp res, J, step;
		temp cond = 1.0f;
		unsigned int iter = 0;

		while ((cond > tol) && (iter < maxIters))
		{
			// Compute residual
			res = func(y, p);
			J = dfunc(y);
			step = res / J;

			// Cap step if necessary
			if (step > cap)
			{
				step = cap;
			}
			if (step < -1.0f * cap)
			{
				step = -1.0f * cap;
			}

			// Newton step
			y -= step;
			iter++;
			cond = fabsf(step);
		}
		return y;
	}

	/*Damped Newtons method*/
	temp dampedNewton(temp y, temp p)
	{
		temp res = func(y, p);
		temp J = dfunc(y);
		temp step = res / J;
		temp cond = fabsf(step);
		temp res_old = res;
		temp y_old = y;
		unsigned int iter = 0;

		while ((cond > tol) && (iter < maxIters))
		{
			// Newton step
			y = y_old - step;

			// Compute residual
			res = func(y, p);

			// Damping ----------------------------------
			temp damper = 1.0f;
			unsigned int subIter = 0;

			while (((fabsf(res) > fabsf(res_old) || isnan(fabsf(res)) || isinf(fabsf(res))) && (subIter < maxSubIter)))
			{
				damper *= 0.5f;
				y = y_old - damper * step;
				res = func(y, p);
				subIter++;
			}

			J = dfunc(y);
			step = res / J;

			y_old = y;
			res_old = res;
			iter++;
			cond = fabsf(step);
		}

		return y;
	}

	/*Symmetric clipping function*/
	temp func(temp y, temp p)
	{
		if (clippingType == ClippingType::symmetric)
			return p + (2.0 * K_ * Is * sinh(y / (Vt * Ni))) - y;
		else
			return p + K_ * Is * (exp(y / (Vt * Ni)) - exp(-y / (2.0 * Vt * Ni))) - y;
	}

	/*Jacobian*/
	temp dfunc(temp y)
	{
		if (clippingType == ClippingType::symmetric)
			return (2.0 * K_ * (Is / (Vt * Ni)) * cosh(y / (Vt * Ni))) - 1.0;
		else
			return K_ * (Is / (Vt * Ni)) * (exp(y / (Vt * Ni)) + 0.5 * exp(-y / (2.0 * Vt * Ni))) - 1.0;
	}

	temp capFunc(temp Q)
	{
		if (clippingType == ClippingType::symmetric)
			return Ni * Vt * acosh(-Ni * Vt / (2.0 * Is * Q));
		else
			return fabs(-2.0 * Ni * Vt * log(-2.0 * Ni * Vt / (Q * Is)));
	}

	temp newIterate(temp p)
	{
		if (clippingType == ClippingType::symmetric)
			return Ni * Vt * asinh(p / (2.0 * Is * K_));
		else
		{
			if (p < 0)
				return -2.0 * Ni * Vt * log(1.0 + p / (K_ * Is));
			else
				return Ni * Vt * log(1.0 - p / (K_ * Is));
		}
	}
	// Sample Rate
	temp fs;

	// state variable
	temp x[3][1] = { { 0.0 }, { 0.0 }, { 0.0 } };
	temp xPrev[3][1] = { { 0.0 }, { 0.0 }, { 0.0 } };
	temp x2Prev[3][1] = { { 0.0 }, { 0.0 }, { 0.0 } };

	// voltage across diodes
	temp v = 0.0;

	// anti-derivs
	temp adPrev = 0.0;
	temp pPrev = 0.0;
	temp inPrev = 0.0;

	// Circuit parameters
	temp r1 = 10.0e3;
	temp r2 = 51.0e3 + 500.0e3;
	temp r3 = 4.7e3;
	temp c1 = 1.0e-6;
	temp c2 = 51e-12;
	temp c3 = 47e-9;
	temp Is = 2.52e-9;						
	temp Vt = 25.85e-3;							
	temp Ni = 1.752;								

	Matrices<temp> matTool;

	// State space arrays
	temp A[3][3] = { {-1.0 / (r1 * c1), 0.0, 0.0},
					{ -1.0 / (r3 * c2), -1.0 / (r2 * c2), -1.0 / (r3 * c2) },
					{-1.0 / (r3 * c3), 0.0, -1.0 / (r3 * c3)} };

	temp B[3][1] = { {1.0 / (r1 * c1)},{ 1.0 / (r3 * c2)},{ 1.0 / (r3 * c3) } };
	temp C[3][1] = { {0.0}, {-1.0/c2}, {0.0} };
	temp D[3] = { -1.0, 1.0, 0.0 };
	temp E = 1.0;
	temp G[3] = { 0.0, 1.0, 0.0 };

	temp A_[3][3], B_[3][1], C_[3][1], D_[3], E_, F_, G_[3], H_, I[3][3], K_, Z[3][3];

	// Newton raphson parameters
	temp cap;
	const temp tol = 1e-7;			   // tolerance
	const unsigned int maxIters = 50;  // maximum number of iterations
	const unsigned int maxSubIter = 5;

	// look-up table
	temp* pLut;
	temp* iLut;
	temp* adLut;
	size_t N;

	ClippingType clippingType;
};

#endif // !TSClippingStage_h