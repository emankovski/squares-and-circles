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

// extracted arm_math from AudioAnalyzeFFT1024 / AudioAnalyzeFFT256

#include "machine.h"
#include "base/analyze_fft.hxx"

using namespace machine;

class VizFFT : public Engine
{
  AnalyzeFFT<1024> fft;

public:
  VizFFT() : Engine(AUDIO_PROCESSOR)
  {
  }

  void process(const ControlFrame &frame, OutputFrame &of) override
  {
    auto in = machine::get_aux<int16_t>(AUX_L);

    fft.process(in, machine::FRAME_BUFFER_SIZE);

    of.push(in, machine::FRAME_BUFFER_SIZE);
    of.push(in, machine::FRAME_BUFFER_SIZE);
  }

  void onDisplay(uint8_t *display) override
  {
    fft.display(display);

    Engine::onDisplay(display);
  }
};

void init_fft()
{
  machine::add<VizFFT>(GND, "FFT");
}

MACHINE_INIT(init_fft);
