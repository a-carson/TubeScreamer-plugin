/*-----------------------------------------------------------------------
 ALIASING REDUCTION IN VIRTUAL ANALOGUE MODELLING
 Alistair Carson 2020
 MSc Acoustics & Music Technology
 University of Edinburgh
--------------------------------------------------------------------*/
 
#pragma once
#ifndef TSTone_h
#define TSTone_h
#include "JuceHeader.h"

using namespace juce;
using namespace dsp;

template<class temp>

/*
Tube Screamer tone stage class

Calculates the filter coefficients for a specific tone knob position
and processes samples with an IIR filter.

*/
class TSTone
{
public:

	TSTone()
	{
	};

	/*Set sample rate in Hz*/
	void setSampleRate(temp sampleRate)
	{
		c = 2.0f * sampleRate;
	}

	/*Set tone knob position 0 <= tone <= 1*/
	void setTone(temp tone)
	{
		if (tone > 1.0)
			tone = 1.0;

		if (tone < 0.0)
			tone = 0.0;

		// Set Tone Potentiometer
		Rl = tone * 20e3;
		Rr = (1.0 - tone) * 20e3;

		// Combined impedences
		temp wp = 1.0 / (Cs * (Rs * Ri / (Rs + Ri)));
		temp wz = 1.0 / (Cz * (Rz + (Rl * Rr / (Rl + Rr))));
		temp Y = (Rl + Rr) * (Rz + (Rl * Rr / (Rl + Rr)));
		temp X = Rr / ((Rl + Rr) * (Rz + (Rl * Rr / (Rl + Rr))) * Cz);
		temp W = Y / (Rl * Rf + Y);
		temp V = (Rl * Rf + Y) / (Y * Rs * Cs);

		// Laplace domain coefficients
		temp b0 = V * W * wz;
		temp b1 = V;
		temp a0 = wp * wz;
		temp a1 = wp + wz + X;

		// Z-domain coefficients
		b[0] = b0 + b1 * c;
		b[1] = 2.0 * b0;
		b[2] = b0 - b1 * c;
		a[0] = a0 + a1 * c + pow(c, 2);
		a[1] = 2.0 * a0 - 2.0 * pow(c, 2);
		a[2] = a0 - a1 * c + pow(c, 2);

		// Normalise
		for (int i = 0; i < 3; i++)
		{
			b[i] /= a[0];
			a[2-i] /= a[0];
		}

		// Set filter coefficients
		filter.setCoefficients(IIRCoefficients(b[0], b[1], b[2], a[0], a[1], a[2]));
	}

	/*Process sample by sample*/
	temp processSingleSample(temp in)
	{
		auto out = b[0] * in + y1;

		JUCE_SNAP_TO_ZERO(out);

		y1 = b[1] * in - a[1] * out + y2;
		y2 = b[2] * in - a[2] * out;

		return out;
	}

	/*Process block of samples*/
	void processBlock(temp* samples, int numSamples)
	{
		filter.processSamples(samples, numSamples);
	}

private:

	// Circuit parameters
	// Resistors
	temp Rl = 20e3;
	temp Rr = 0.0;
	temp Rf = 1e3;
	temp Ri = 10e3;
	temp Rs = 1e3;
	temp Rz = 220.0;

	// Capacitors
	temp Cz = 0.22e-6;
	temp Cs = 0.22e-6;

	// Filter Coefficients
	temp b[3];
	temp a[3];
	temp y1 = 0.0f;
	temp y2 = 0.0f;
	temp c;				// for bilinear tranform

	IIRFilter filter;
};
#endif // !TSTone_h