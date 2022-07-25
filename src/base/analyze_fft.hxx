
// code based on Teensy Audio analyse_fft1024.cpp

#pragma once

#include <arm_math.h>

template <int BITS = 1024>
class AnalyzeFFT
{
    uint16_t output[BITS / 2];
    int16_t buffer[BITS * 2] __attribute__((aligned(4)));

    arm_cfft_radix4_instance_q15 fft_inst;

    uint32_t *p = (uint32_t *)buffer;

    // computes ((a[15:0] * b[15:0]) + (a[31:16] * b[31:16]))
    inline int32_t multiply_16tx16t_add_16bx16b(uint32_t a, uint32_t b)
    {
        int32_t out;
        asm volatile("smuad %0, %1, %2"
                     : "=r"(out)
                     : "r"(a), "r"(b));
        return out;
    }

    // https://www.esp8266.com/viewtopic.php?p=55534
    const uint16_t sqrt_integer_guess_table[33] = {55109, 38968, 27555, 19484, 13778, 9742, 6889, 4871, 3445, 2436, 1723,
                                                   1218, 862, 609, 431, 305, 216, 153, 108, 77, 54, 39, 27, 20, 14, 10,
                                                   7, 5, 4, 3, 2, 1, 0};

    inline uint32_t sqrt_uint32(uint32_t in)
    {
        uint32_t n = sqrt_integer_guess_table[__builtin_clz(in)];
        n = ((in / n) + n) / 2;
        n = ((in / n) + n) / 2;
        n = ((in / n) + n) / 2;
        return n;
    }

public:
    AnalyzeFFT()
    {
        arm_cfft_radix4_init_q15(&fft_inst, BITS, 0, 1);
        p = (uint32_t *)buffer;
    }

    void process(const int16_t *in, size_t len)
    {
        for (size_t i = 0; i < len; i++)
        {
            auto val = in[i];
            *p++ = val / (1 << 4);

            if (p >= (uint32_t *)(&buffer[BITS * 2]))
            {
                p = (uint32_t *)buffer;
                arm_cfft_radix4_q15(&fft_inst, buffer);

                for (int i = 0; i < LEN_OF(output); i++)
                {
                    uint32_t tmp = *((uint32_t *)buffer + i); // real & imag
                    uint32_t magsq = multiply_16tx16t_add_16bx16b(tmp, tmp);
                    output[i] = sqrt_uint32(magsq);
                };
            }
        }
    }

    void display(uint8_t *display)
    {
        char ylim = 57;
        for (int i = 0; i < 128; i++)
        {
            if (BITS == 1024)
            {
                uint32_t f = output[(1 + i * 3)]; // up 33Khz?!

                for (int j = 1; j < 2; j++)
                    f += output[(1 + (i * 3) + j)];

                // f *= (1.0f / UINT16_MAX) ;

                // f = 100.f - 20.f * log10f(f);

                gfx::drawLine(display, i, ylim, i, ylim - f);
            }
            else if (BITS == 256)
            {
                auto f = output[i];
                gfx::drawLine(display, i, ylim, i, ylim - f);
            }
            else
            {
                gfx::drawString(display, 0, 32, "not implemented");
            }
        };
    }
};
