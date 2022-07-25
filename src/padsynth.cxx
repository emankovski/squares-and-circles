// Copyright (C)2022 - Eduard Heidt
//
// Author: Eduard Heidt (eh2k@gmx.de)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// See http://creativecommons.org/licenses/MIT/ for more information.
//

#include "stmlib/stmlib.h"
#include "stmlib/dsp/filter.h"
#include "machine.h"
#include "base/SampleEngine.hxx"

#include "fft/fft4g.h"

// https://zynaddsubfx.sourceforge.io/doc/PADsynth/PADsynth.htm

/*
    PADsynth algorithm
    By: Nasca O. Paul, Tg. Mures, Romania

    This implementation and the algorithm are released under Public Domain
    Feel free to use it into your projects or your products ;-)
*/

using namespace machine;

static constexpr int N = (1 << 14);
static constexpr int samplerate = machine::SAMPLE_RATE;
static constexpr int number_harmonics = 16;

struct PadSynthEngine : public SampleEngine
{
    float A[number_harmonics]; // Amplitude of the harmonics
    float buffer[N];

    const tsample_spec<float> _wavetable[1] = {
        {"> pad", buffer, N, machine::SAMPLE_RATE, 0},
    };

    float f = 440;
    float bw = 60.f;
    float bwscale = 1.0f;

    float vca = 0;

    PadSynthEngine() : SampleEngine(&_wavetable[0], 0, LEN_OF(_wavetable))
    {
        param[0].init("VCA", &vca, 1);

        SampleEngine::loop = 2;

        for (int i = 0; i < number_harmonics; i++)
            A[i] = 0.0f;

        A[1] = 1.0f; // default, the first harmonic has the amplitude 1.0

        for (int i = 1; i < number_harmonics; i++)
            A[i] = 1.0 / i;

        gen_pad(f, bw, bwscale);

        // float f1 = 440;
        // for (int i = 1; i < number_harmonics; i++)
        // {
        //     A[i] = 1.0 / i;
        //     float formants = exp(-pow((i * f1 - 600.0) / 150.0, 2.0)) + exp(-pow((i * f1 - 900.0) / 250.0, 2.0)) +
        //                      exp(-pow((i * f1 - 2200.0) / 200.0, 2.0)) + exp(-pow((i * f1 - 2600.0) / 250.0, 2.0)) +
        //                      exp(-pow((i * f1) / 3000.0, 2.0)) * 0.1;
        //     A[i] *= formants;
        // };
    };

    void process(const ControlFrame &frame, OutputFrame &of) override
    {
        SampleEngine::process(frame, of);
        for(int i = 0; i < machine::FRAME_BUFFER_SIZE; i++)
            ((float*)of.out)[i] *= vca;
    }

    float relF(int n)
    {
        return (n * (1.0 + n * 0.1));
    };

    void setharmonic(int n, float value)
    {
        if ((n < 1) || (n >= number_harmonics))
            return;
        A[n] = value;
    };

    float getharmonic(int n)
    {
        if ((n < 1) || (n >= number_harmonics))
            return 0.0;
        return A[n];
    };

    float profile(float fi, float bwi)
    {
        float x = fi / bwi;
        x *= x;
        if (x > 14.71280603)
            return 0.0; // this avoids computing the e^(-x^2) where it's results are very close to zero
        return expf(-x) / bwi;
    };

    int fft_ip[N / 2] = {}; // 2+sqrt(N/2);
    float fft_w[N / 2] = {};

    void normalize(float *smp)
    {
        int i;
        float max = 0.0;
        for (i = 0; i < N; i++)
            if (fabsf(smp[i]) > max)
                max = fabsf(smp[i]);
        if (max < 1e-5)
            max = 1e-5;
        for (i = 0; i < N; i++)
            smp[i] /= max * 1.4142;
    };

    void gen_pad(float f, float bw, float bwscale)
    {
        float *freq_amp = buffer; // new float[N / 2];     // Amplitude spectrum

        int i, nh;

        for (i = 0; i < N / 2; i++)
            freq_amp[2 * i] = 0.0; // default, all the frequency amplitudes are zero

        for (nh = 1; nh < number_harmonics; nh++)
        {                // for each harmonic
            float bw_Hz; // bandwidth of the current harmonic measured in Hz
            float bwi;
            float fi;
            float rF = f * relF(nh);

            bw_Hz = (powf(2.0, bw / 1200.0) - 1.0) * f * powf(relF(nh), bwscale);

            bwi = bw_Hz / (2.0f * samplerate);
            fi = rF / samplerate;
            for (i = 0; i < N / 2; i++)
            { // here you can optimize, by avoiding to compute the profile for the full frequency (usually it's zero or very close to zero)
                float hprofile = profile((i / (float)N) - fi, bwi);
                freq_amp[2 * i] += hprofile * A[nh];
            };
        };

        // void IFFT(float *freq_amp, float *freq_phase, float *smp) >>>

        // fft_ip[0] = 0;
        float *a = &buffer[0];

        // rdft(N, 1, a, ip, w);

        for (int i = 0; i < N / 2; i++)
        {
            auto phase = FRND() * 2.0 * M_PI;
            a[(2 * i)] = freq_amp[2 * i] * cosf(phase);
            a[(2 * i + 1)] = freq_amp[2 * i] * sinf(phase);
        };

        // std::fill(smp, &smp[N], 0);
        rdft(N, -1, a, fft_ip, fft_w);

        constexpr float kScaling = 2.f / N;
        for (int j = 0; j < N; j++)
        {
            a[j] *= kScaling;
        }

        // << IFFT

        normalize(buffer);
    };

    float FRND()
    {
        return ((float)rand() / (RAND_MAX + 1.0));
    };
};

void init_padsynth()
{
    machine::add<PadSynthEngine>(SYNTH, "PadSynth");
}

MACHINE_INIT(init_padsynth);