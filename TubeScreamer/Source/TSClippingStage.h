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

	TSClippingStage()
	{
	};

	void setSampleRate(temp sampleRate)
	{
		fs = sampleRate;
	}

	void setDistortion(temp distortion)
	{
		r2 = 51e3 + distortion;
		A[1][1] = -1.0f / (r2 * c2);
		updateStateSpaceArrays();
	}

	void setDiodeParameters(temp saturationCurrent, temp thermalVoltage, temp idealityFactor)
	{
		Is = saturationCurrent;
		Vt = thermalVoltage;
		Ni = idealityFactor;
	}

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
		cap = Ni * Vt * acoshf(-Ni * Vt / (2 * Is * K_));
	}


	// Main Process
	temp process(temp in, bool useLut)
	{
		// Input
		const temp p = matTool.multiply1x3by3x1(G_, x) + H_ * in;
		temp iv = 0.0;
		temp ad = 0.0f;
		if (useLut)
		{
			ad = lookUp(pLut, adLut, p);
			if (fabsf(p-pPrev) > 1.0e-8f)
				iv = (ad - adPrev) / (p - pPrev);
			else
				iv = lookUp(pLut, iLut, 0.5 * (p + pPrev));
		}
		else
		{
			v = Ni * Vt * asinhf(p / (2 * Is * K_));
			v = cappedNewton(v, p);
			iv = (v - p) / K_;
		}

		// update state variable
		matTool.copyTo(x_prev, x);
		temp x_int[3][1];
		matTool.multiply3x3by3x1(A_, x_prev, x_int);
		for (int i = 0; i < 3; i++)
		{
			x[i][0] = x_int[i][0] + (B_[i][0] * in) + (C_[i][0] * iv);
		}

		// output
		float out = matTool.multiply1x3by3x1(D_, x_prev) + E_ * in + F_ * iv;

		// fail safe
		if (out > 5.0f)
		{
			out = 0.0f;
			v = 0.0f;
			for (int i = 0; i < 3; i++)
			{
				x[i][0] = 0.0f;
				x_prev[i][0] = 0.0f;
			}
		}

		pPrev = p;
		adPrev = ad;
		return out;
	}

	void makeLookUpTable(size_t numPoints, temp fs, temp pmax, temp distortion)
	{
		N = numPoints;
		pLut = new float[N];
		iLut = new float[N];
		adLut = new float[N];

		setSampleRate(fs/1.5);
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
				ad += 0.5 * dP * (iLut[i] + iLut[i-1]);
			adLut[i] = ad;
		}

		// Adjust offset
		temp ad0 = lookUp(pLut, adLut, 0.0);
		for (int i = 0; i < N; i++)
			adLut[i] -= ad0;
		
	}

	temp lookUp(temp* x, temp* y, temp xq)
	{
		int indices[4] = { -1, 0, 1, 2 };
		temp indBet = (xq - x[0]) / (x[1] - x[0]);
		int indBetFloored = floorf(indBet);
		
		for (int i = 0; i < 4; i++)
			indices[i] += indBetFloored;


		if (indices[3] > (N-1))
			for (int i = 0; i < 4; i++)
				indices[i] -= indices[3] - N + 1;

		if (indices[0] < 0)
			for (int i = 0; i < 4; i++)
				indices[3-i] -= indices[0];
		

		temp alpha = indBet - indices[2] + 0.5;

		temp P[4] = { (alpha + 0.5) * (alpha - 0.5) * (alpha - 1.5) / -6.0 ,
		(alpha + 1.5)* (alpha - 0.5)* (alpha - 1.5) / 2.0,
		(alpha + 0.5)* (alpha - 1.5)* (alpha + 1.5) / -2.0,
		(alpha + 0.5)* (alpha - 0.5)* (alpha + 1.5) / 6.0 };

		temp yq = 0.0;

		for (int i = 0; i < 4; i++)
			yq += P[i] * y[indices[i]];

		return yq;
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
		temp arg = y / (Vt * Ni);

		if (fabsf(arg) < 5)
		{
			temp sinhy = FastMathApproximations::sinh<temp>(arg);
			return p + (2.0f * K_ * Is * sinhy) - y;
		}
		else
		{
			return p + (2.0f * K_ * Is * sinhf(arg)) - y;
		}
	}

	/*Jacobian*/
	temp dfunc(temp y)
	{
		temp arg = y / (Vt * Ni);

		if (fabsf(arg) < 5)
		{
			temp coshy = FastMathApproximations::cosh<temp>(arg);
			return (2.0f * K_ * (Is / (Vt * Ni)) * coshy) - 1.0f;
		}
		else
		{
			return (2.0f * K_ * (Is / (Vt * Ni)) * coshf(arg)) - 1.0f;
		}
	}

	// Sample Rate
	temp fs;

	// state variable
	temp x[3][1] = { { 0.0f }, { 0.0f }, { 0.0f } };
	temp x_prev[3][1] = { { 0.0f }, { 0.0f }, { 0.0f } };

	// voltage across diodes
	temp v = 0.0f;

	// anti-derivs
	temp adPrev = 0.0f;
	temp pPrev = 0.0f;

	// Circuit parameters
	temp r1 = 10e3f;
	temp r2 = 51e3 + 500e3;
	temp r3 = 4.7e3f;
	temp c1 = 1e-6f;
	temp c2 = 51e-12f;
	temp c3 = 47e-9f;
	temp Is = 2.52e-9f;						
	temp Vt = 25.85e-3f;							
	temp Ni = 1.752f;								

	Matrices<temp> matTool;

	// State space arrays
	temp A[3][3] = { {-1.0f / (r1 * c1), 0.0f, 0.0f},
					{ -1.0f / (r3 * c2), -1.0f / (r2 * c2), -1.0f / (r3 * c2) },
					{-1.0f / (r3 * c3), 0.0f, -1.0f / (r3 * c3)} };

	temp B[3][1] = { {1.0f / (r1 * c1)},{ 1.0f / (r3 * c2)},{ 1.0f / (r3 * c3) } };
	temp C[3][1] = { {0.0f}, {-1.0f/c2}, {0.0f} };
	temp D[3] = { -1.0f, 1.0f, 0.0f };
	temp E = 1.0f;
	temp G[3] = { 0.0f, 1.0f, 0.0f };

	temp A_[3][3], B_[3][1], C_[3][1], D_[3], E_, F_, G_[3], H_, I[3][3], K_, Z[3][3];

	// Newton raphson parameters
	temp cap;// = Ni * Vt * acoshf(-Ni * Vt / (2 * Is * K_));
	const temp tol = 1e-7f;						// tolerance
	const unsigned int maxIters = 50;  // maximum number of iterations
	const unsigned int maxSubIter = 5;

	// look-up table
	temp* pLut;
	temp* iLut;
	temp* adLut;
	size_t N;
};

template<class temp>
class LookUpTable
{
public:
	LookUpTable(size_t numPoints)
	{
		N = numPoints;
		p = new float[N];
		f = new float[N];
		ad1 = new float[N];
	}

	void generate(temp fs, temp pmax, temp distortion)
	{
		ts.setSampleRate(fs);
		ts.setDistortion(distortion);

		temp dP = 2.0 * pmax / (temp)(N - 1);
		temp p0 = -1.0 * pmax;
		temp p1 = p0;
		temp y = 0.0;
		
		for (int i = 0; i < N; i++)
		{
			p0 = -pmax + i * dP;
			p[i] = p0;
			y = ts.dampedNewton(y, p0);
			f[i] = (y - p0) / model.K_;
			DBG(f[i]);
		}
	}

private:
	temp* p;
	temp* f;
	temp* ad1;
	size_t N;
	TSClippingStage<temp> ts;
};
#endif // !TSClippingStage_h