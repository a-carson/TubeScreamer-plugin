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
		dist = distortion;
		r2 = 51e3 + dist;
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
	}


	// Main Process
	temp process(temp in)
	{
		// Input
		const temp p = matTool.multiply1x3by3x1(G_, x) + H_ * in;

		// initial guess
		y = Ni * Vt * asinhf(p / (2 * Is * K_));

		// newtons method
		y = cappedNewton(y, p);
		temp iv = (y - p) / K_;

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
			y = 0.0f;
			for (int i = 0; i < 3; i++)
			{
				x[i][0] = 0.0f;
				x_prev[i][0] = 0.0f;
			}
		}

		return out;
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
	temp func(temp v, temp p)
	{
		temp arg = v / (Vt * Ni);

		if (fabsf(arg) < 5)
		{
			temp sinhy = FastMathApproximations::sinh<temp>(arg);
			return p + (2.0f * K_ * Is * sinhy) - v;
		}
		else
		{
			return p + (2.0f * K_ * Is * sinhf(arg)) - v;
		}
	}

	/*Jacobian*/
	temp dfunc(temp v)
	{
		temp arg = v / (Vt * Ni);

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
	// output variable
	temp y = 0.0f;

	// Pedal Settings
	temp dist = 0.5f;

	// Circuit parameters
	temp r1 = 10e3f;
	temp r2 = 51e3 + 500e3;
	temp r3 = 4.7e3f;
	temp c1 = 1e-6f;
	temp c2 = 51e-12f;
	temp c3 = 47e-9f;
	temp Is = 2.52e-9f;								// saturation current
	temp Vt = 25.85e-3f;								// thermal voltage
	temp Ni = 1.752f;								// ideality factor

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
	temp I[3][3];
	temp Z[3][3];


	temp A_[3][3] = { { 0.997918834547347f,  0.0f ,  0.0f },
	{	-60.490777826210604f,  0.459122653674072f ,  -60.553789053112894f },
	{	-0.089970406484047f ,  0.0f,  0.909935874342532f } };
	temp B_[3][1] = { { 0.002081165452653f},{60.490777826210596f},{0.089970406484047f } };
	temp C_[3][1] = { { 0.0f},{-298023.4178255865f},{0.0f } };
	temp D_[3] = { -31.244348330378976f,   0.729561326837036f,-30.276894526556443 };
	temp E_ = 31.244348330378973f;
	temp F_ = -149011.7089127933f;
	temp G_[3] = { -30.245388913105302f,   0.729561326837036f, -30.276894526556443 };
	temp H_ = 30.245388913105298f;
	temp K_ = -149011.7089127933f;

	// Newton raphson parameters
	temp cap = Ni * Vt * acoshf(-Ni * Vt / (2 * Is * K_));
	const temp tol = 1e-7f;						// tolerance
	const unsigned int maxIters = 50;  // maximum number of iterations
	const unsigned int maxSubIter = 5;
};
#endif // !TSClippingStage_h