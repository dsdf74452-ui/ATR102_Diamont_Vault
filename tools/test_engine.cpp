// Test offline del motor (sin JUCE):  g++ -O2 -std=c++17 -I../Source test_engine.cpp ../Source/TapeEngine.cpp -o test_engine
// (c) 2026 Diamont Vault(R)
#include "TapeEngine.h"
#include <cmath>
#include <complex>
#include <cstdio>
#include <vector>

using namespace atr;
static const double PI = 3.14159265358979323846;

static std::vector<float> run (Settings s, const std::vector<float>& in, double sr)
{
    TapeEngine e; e.prepare (sr); e.setSettings (s);
    std::vector<float> l (in), r (in);
    const int B = 512;
    for (size_t i = 0; i < in.size(); i += B)
    {
        const int n = int (std::min<size_t> (B, in.size() - i));
        e.process (l.data() + i, r.data() + i, n);
    }
    return l;
}

// amplitud de un armonico via DFT sobre ventana entera (frecuencia ajustada a bin)
static double mag (const std::vector<float>& x, double f, double sr, size_t from, size_t len)
{
    std::complex<double> a = 0;
    for (size_t i = 0; i < len; ++i)
        a += double (x[from + i]) * std::polar (1.0, -2.0 * PI * f * double (i) / sr);
    return 2.0 * std::abs (a) / double (len);
}

static std::vector<float> sine (double f, double ampDb, double sr, size_t n)
{
    std::vector<float> v (n);
    for (size_t i = 0; i < n; ++i) v[i] = float (std::pow (10.0, ampDb / 20.0) * std::sin (2 * PI * f * double (i) / sr));
    return v;
}

static Settings clean()
{
    Settings s; s.noise = false; s.wfOn = false; s.headBump = false; return s;
}

int main()
{
    const double sr = 48000.0;
    printf ("Latencia reportada: %d muestras\n", TapeEngine::latencySamples());

    // 0. Impulso: posicion del pico
    {
        std::vector<float> imp (4096, 0.f); imp[100] = 0.001f;
        auto y = run (clean(), imp, sr);
        size_t pk = 0; for (size_t i = 0; i < y.size(); ++i) if (std::fabs (y[i]) > std::fabs (y[pk])) pk = i;
        printf ("Impulso: pico en %zu (esperado %d)\n", pk, 100 + TapeEngine::latencySamples());
    }

    // 1. Respuesta en frecuencia (-40 dBFS, lineal)
    printf ("\nRespuesta en frecuencia (15 ips, 456, NAB, -40 dBFS)  [dB re 1 kHz]\n");
    for (int bump = 0; bump < 2; ++bump)
    {
        auto s = clean(); s.headBump = bump;
        const double fs[] { 30, 50, 60, 100, 200, 1000, 5000, 10000, 16000, 20000 };
        const size_t N = 48000 * 2, skip = 24000;
        double ref = 0;
        printf (bump ? " bump ON : " : " bump OFF: ");
        std::vector<double> out;
        for (double f : fs) { auto y = run (s, sine (f, -40, sr, N), sr); out.push_back (mag (y, f, sr, skip, 24000)); if (f == 1000) ref = out.back(); }
        for (size_t i = 0; i < out.size(); ++i) printf ("%g:%+.2f ", fs[i], 20 * std::log10 (out[i] / ref));
        printf ("\n");
    }
    printf ("\nGanancia pequena senal a 1 kHz (-40 dBFS): ");
    { auto y = run (clean(), sine (1000, -40, sr, 96000), sr);
      printf ("%+.3f dB\n", 20 * std::log10 (mag (y, 1000, sr, 24000, 24000) / std::pow (10.0, -2.0))); }

    // 2. THD vs nivel
    printf ("\nTHD 1 kHz (H2+H3+H4+H5) vs nivel de entrada, 15 ips:\n");
    const char* names[] { "456", "499", "GP9" };
    for (int tp = 0; tp < 3; ++tp)
    {
        printf (" cinta %s: ", names[tp]);
        for (double lvl : { -18.0, -12.0, -6.0, -3.0, 0.0 })
        {
            auto s = clean(); s.tape = tp; s.cal = 2;     // +6 dB cal
            auto y = run (s, sine (1000, lvl, sr, 96000), sr);
            const double f1 = mag (y, 1000, sr, 24000, 48000);
            double h = 0; for (int k = 2; k <= 5; ++k) { const double m = mag (y, 1000.0 * k, sr, 24000, 48000); h += m * m; }
            printf ("%+.0fdBFS:%.2f%%  ", lvl, 100.0 * std::sqrt (h) / f1);
        }
        printf ("\n");
    }

    // 3. Alias: 9 kHz a 0 dBFS con cal +6 (armonicos 27k, 45k se pliegan)
    {
        auto s = clean(); s.cal = 2;
        auto y = run (s, sine (9000, 0, sr, 96000), sr);
        const double f1 = mag (y, 9000, sr, 24000, 48000);
        // 27 kHz se pliega a 21 kHz; 45 kHz a 3 kHz
        printf ("\nAlias de 9 kHz @0dBFS: 21k:%.1f dBc  3k:%.1f dBc\n",
                20 * std::log10 (mag (y, 21000, sr, 24000, 48000) / f1),
                20 * std::log10 (mag (y, 3000, sr, 24000, 48000) / f1));
    }

    // 4. Ruido en silencio
    printf ("\nSuelo de ruido (silencio, rms dBFS, de-enfasis incluido):\n");
    for (int w = 0; w < 4; ++w)
    {
        Settings s; s.wfOn = false; s.width = w;
        auto y = run (s, std::vector<float> (96000, 0.f), sr);
        double e = 0; for (size_t i = 24000; i < y.size(); ++i) e += double (y[i]) * y[i];
        printf (" ancho %d: %.1f dBFS  ", w, 10 * std::log10 (e / double (y.size() - 24000)));
    }
    printf ("\n");

    // 5. Wow/flutter: desviacion de pitch por demodulacion I/Q de un tono de 3 kHz
    {
        Settings s = clean(); s.wfOn = true; s.wowFlutter = 1.0f;
        const size_t N = 48000 * 8;
        auto y = run (s, sine (3000, -20, sr, N), sr);
        const size_t W = 480;                       // media movil 10 ms (pasabajos de la demod)
        std::vector<std::complex<double>> z (N);
        for (size_t i = 0; i < N; ++i) z[i] = double (y[i]) * std::polar (1.0, -2.0 * PI * 3000.0 * double (i) / sr);
        std::vector<double> ph;
        for (size_t i = 48000; i + W < N; i += 48)
        {
            std::complex<double> a = 0; for (size_t k = 0; k < W; ++k) a += z[i + k];
            ph.push_back (std::arg (a));
        }
        double fmaxDev = 0;
        for (size_t i = 1; i < ph.size(); ++i)
        {
            double d = ph[i] - ph[i - 1]; while (d > PI) d -= 2 * PI; while (d < -PI) d += 2 * PI;
            const double fi = d / (2 * PI) / (48.0 / sr);      // Hz de desviacion
            fmaxDev = std::max (fmaxDev, std::fabs (fi));
        }
        printf ("\nWow/flutter 100%%: desviacion pico de pitch = %.3f%% (3 kHz -> +-%.2f Hz)\n",
                100.0 * fmaxDev / 3000.0, fmaxDev);
    }
    return 0;
}
