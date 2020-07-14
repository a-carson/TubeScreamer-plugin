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
		//temp c[3][1];
		//matTool.multiply3x3by3x1(a, b, c);
		//temp d[3] = {1, 1, 1};
		//temp e = matTool.multiply1x3by3x1(d, c);
		//matTool.invert3x3(a);


		//// Clear Arrays
		//A.clear();
		//B.clear();
		//C.clear();
		//D.clear();
		//G.clear();
		//I.clear();
		//x.clear();
		//x_prev.clear();
		//y = 0;

		//// Identity matrix
		//for (int i = 0; i < 3; i++)
		//{
		//	for (int j = 0; j < 3; j++)
		//	{
		//		if (i == j)
		//			I(i, j) = 1.0f;
		//	}
		//}

		//// Update non-zero terms in State Space Arrays
		//// A
		//A(0, 0) = 1.0f / (r1 * c1);
		//A(1, 0) = 1.0f / (r3 * c2);
		//A(1, 1) = 1.0f / (r2 * c2);
		//A(1, 2) = 1.0f / (r3 * c2);
		//A(2, 0) = 1.0f / (r3 * c3);
		//A(2, 2) = 1.0f / (r3 * c3);
		//A *= -1.0f;

		//// B
		//B(0, 0) = 1.0f / (r1 * c1);
		//B(1, 0) = 1.0f / (r3 * c2);
		//B(2, 0) = 1.0f / (r3 * c3);

		//// C
		//C(1, 0) = -1.0f / c2;

		//// D
		//D(0, 0) = -1.0f;
		//D(0, 1) = 1.0f;

		//// G
		//G(0, 1) = 1.0f;

	};

	void setSampleRate(temp sampleRate)
	{
		fs = sampleRate;
	}

	//void setDistortion(temp distortion)
	//{
	//	dist = distortion;
	//	r2 = 51e3 + dist * 500e3;
	//	A(1, 1) = -1.0f / (r2 * c2);
	//	updateStateSpaceArrays();
	//}

	void setDiodeParameters(temp saturationCurrent, temp thermalVoltage, temp idealityFactor)
	{
		Is = saturationCurrent;
		Vt = thermalVoltage;
		Ni = idealityFactor;
		cap = Ni * Vt * acoshf(-Ni * Vt / (2 * Is * K_));
	}

	void updateStateSpaceArrays()
	{
		// Trapezoid Arrays
		/*Z = (I * (2.0f * fs)) - A;
		Z = matTool.inv3(Z);
		A_ = (I * (2.0f * fs) + A) * Z;
		B_ = A_ * B + B;
		C_ = A_ * C + C;
		D_ = D * Z;
		G_ = G * Z;

		E_ = matTool.mat2float(D * Z * B) + E;
		F_ = matTool.mat2float(D * Z * C);
		H_ = matTool.mat2float(G * Z * B);
		K_ = matTool.mat2float(G * Z * C);
		matTool.contents(A_);
		DBG('\n');
		matTool.contents(B_);
		DBG('\n');
		matTool.contents(C_);
		DBG('\n');
		matTool.contents(D_);
		DBG('\n');
		DBG(E_);
		DBG('\n');
		DBG(F_);
		DBG('\n');
		matTool.contents(G_);
		DBG('\n');
		DBG(H_);
		DBG(K_);*/

		//cap = Ni * Vt * acoshf(-Ni * Vt / (2 * Is * K_));
	}

	// i(v)
	temp iv(temp v)
	{
		return 2.0f * Is * FastMathApproximations::sinh<temp>(v / (Ni * Vt));
	}

	// Non linear function
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

	// Derivative of Non-linear function
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


	// Main Process
	temp process(temp in)
	{
		unsigned int iter = 0;
		//temp p = in;
		const temp p = matTool.multiply1x3by3x1(G_, x) + H_ * in;
		y = Ni * Vt * asinhf(p / (2 * Is * K_));
		temp res = func(y, p);
		temp J = dfunc(y);
		temp step = res / J;
		temp cond = fabsf(step);
		temp res_old = res;
		temp y_old = y;

		while ((cond > tol) && (iter < maxIters))
		{
			// Cap step size if necessary
			if (step > cap)
			{
				step = cap;
			}
			if (step < -1.0f * cap)
			{
				step = -1.0f * cap;
			}

			// Newton step
			y = y_old - step;

			// Check argument
			temp arg = y / (Ni * Vt);

			// Compute residual
			res = func(y, p);


			// Damping ----------------------------------
			temp damper = 1.0f;
			unsigned int subIter = 0;

			//while (((fabsf(res) > fabsf(res_old) || isnan(fabsf(res)) || isinf(fabsf(res))) && (subIter < maxSubIter)))
			//{
			//	damper *= 0.5f;
			//	y = y_old - damper * step;
			//	res = func(y, p);
			//	subIter++;
			//}

			J = dfunc(y);
			step = res / J;

			y_old = y;
			res_old = res;
			iter++;
			cond = fabsf(step);
		}

		temp i_cur = (y - p) / K_;


		//if (y > 1.0f)
		//{
		//	i = 0.0;
		//	y = 0.0f;
		//	x.clear();
		//}


		//if (iter >= maxIters)
		//{
		//	i = 0.0f;
		//	y = 0.0f;
		//	x.clear();
		//}

		// update state variable
		matTool.copyTo(x_prev, x);
		temp x_int[3][1];
		matTool.multiply3x3by3x1(A_, x_prev, x_int);
		for (int i = 0; i < 3; i++)
		{
			x[i][0] = x_int[i][0] + (B_[i][0] * in) + (C_[i][0] * i_cur);
		}
		float out = matTool.multiply1x3by3x1(D_, x_prev) + E_ * in + F_ * i_cur;
		//x = A_ * x_prev + (B_ * in) + (C_ * i);
		//float out = matTool.mat2float(D_ * x_prev) + E_ * in + F_ * i;
		//float out = y;

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
	temp r2 = 51e3 + 0.5f * 500e3;
	temp r3 = 4.7e3f;
	temp c1 = 1e-6f;
	temp c2 = 51e-12f;
	temp c3 = 47e-9f;
	temp Is = 2.52e-9f;								// saturation current
	temp Vt = 25.85e-3f;								// thermal voltage
	temp Ni = 1.752f;								// ideality factor

	Matrices<temp> matTool;

	// State space arrays
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