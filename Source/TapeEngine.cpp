// (c) 2026 Diamont Vault(R). All rights reserved.
#include "TapeEngine.h"
#include <algorithm>
#include <cmath>

namespace atr
{
namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr float  kFluxRef = 7.9432823f;   // 0 VU = -18 dBFS  <->  flujo de referencia = 1.0

inline float dbToLin (float db) { return std::pow (10.f, db * 0.05f); }

struct TapeVoicing
{
    const char* name;
    float satDb;     // margen de saturacion sobre el nivel de referencia (dB)
    float noiseDb;   // + = mas ruido
    float hfScale;   // escala del ancho de banda
    float bumpDb;    // head bump (dB)
};

// Voicings iniciales. NO son mediciones; calibrar contra una maquina real.
constexpr TapeVoicing kTapes[kNumTapes] = {
    { "456",   13.0f,  0.0f, 1.00f, 2.0f },
    { "499",   15.0f, -0.5f, 1.00f, 1.5f },
    { "GP9",   18.0f, -2.0f, 1.05f, 1.2f },
    { "250",   14.0f,  0.0f, 0.95f, 2.0f },
    { "468",   12.5f,  1.0f, 0.97f, 2.5f },
    { "911",   13.5f,  0.0f, 1.00f, 2.2f },
    { "LPR90", 12.0f,  1.5f, 0.90f, 2.8f },
    { "996",   16.0f, -1.0f, 1.03f, 1.8f },
};

inline float xorshiftUniform (uint32_t& s)     // -1..1
{
    s ^= s << 13; s ^= s >> 17; s ^= s << 5;
    return float (int32_t (s)) * (1.0f / 2147483648.0f);
}

double besselI0 (double x)
{
    double sum = 1.0, term = 1.0;
    for (int k = 1; k < 50; ++k)
    {
        term *= (x / (2.0 * k)) * (x / (2.0 * k));
        sum += term;
    }
    return sum;
}
} // namespace

// ------------------------------------------------------------------ Biquad
void Biquad::highShelf (double sr, double f, double gainDb, double S)
{
    const double A = std::pow (10.0, gainDb / 40.0);
    const double w0 = 2.0 * kPi * f / sr, c = std::cos (w0), sn = std::sin (w0);
    const double alpha = sn / 2.0 * std::sqrt ((A + 1.0 / A) * (1.0 / S - 1.0) + 2.0);
    const double tsa = 2.0 * std::sqrt (A) * alpha;
    setCoeffs (A * ((A + 1) + (A - 1) * c + tsa),
               -2.0 * A * ((A - 1) + (A + 1) * c),
               A * ((A + 1) + (A - 1) * c - tsa),
               (A + 1) - (A - 1) * c + tsa,
               2.0 * ((A - 1) - (A + 1) * c),
               (A + 1) - (A - 1) * c - tsa);
}

void Biquad::peak (double sr, double f, double q, double gainDb)
{
    const double A = std::pow (10.0, gainDb / 40.0);
    const double w0 = 2.0 * kPi * f / sr, c = std::cos (w0);
    const double alpha = std::sin (w0) / (2.0 * q);
    setCoeffs (1 + alpha * A, -2 * c, 1 - alpha * A, 1 + alpha / A, -2 * c, 1 - alpha / A);
}

void Biquad::lowPass (double sr, double f, double q)
{
    const double w0 = 2.0 * kPi * f / sr, c = std::cos (w0);
    const double alpha = std::sin (w0) / (2.0 * q);
    setCoeffs ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + alpha, -2 * c, 1 - alpha);
}

void Biquad::highPass (double sr, double f, double q)
{
    const double w0 = 2.0 * kPi * f / sr, c = std::cos (w0);
    const double alpha = std::sin (w0) / (2.0 * q);
    setCoeffs ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + alpha, -2 * c, 1 - alpha);
}

// -------------------------------------------------------------- HalfbandOS
HalfbandOS::HalfbandOS()
{
    const int M = (kTaps - 1) / 2;
    const double beta = 9.0, i0b = besselI0 (beta);
    double sum = 0.0;
    std::array<double, kTaps> d {};
    for (int k = 0; k < kTaps; ++k)
    {
        const double n = k - M;
        const double arg = 0.5 * n * kPi;
        const double sinc = (n == 0.0) ? 1.0 : std::sin (arg) / arg;
        const double r = double (k) / double (kTaps - 1) * 2.0 - 1.0;
        const double w = besselI0 (beta * std::sqrt (std::max (0.0, 1.0 - r * r))) / i0b;
        d[size_t (k)] = 0.5 * sinc * w;
        sum += d[size_t (k)];
    }
    for (int k = 0; k < kTaps; ++k) h[size_t (k)] = float (d[size_t (k)] / sum);
}

void HalfbandOS::reset()
{
    upBuf.fill (0.f); dnBuf.fill (0.f); upPos = dnPos = 0;
}

void HalfbandOS::up (float in, float& o0, float& o1)
{
    upPos = (upPos == 0 ? kTaps - 1 : upPos - 1);
    upBuf[size_t (upPos)] = upBuf[size_t (upPos + kTaps)] = in;
    const float* b = &upBuf[size_t (upPos)];
    float a0 = 0.f, a1 = 0.f;
    for (int k = 0; 2 * k < kTaps; ++k)      a0 += h[size_t (2 * k)] * b[k];
    for (int k = 0; 2 * k + 1 < kTaps; ++k)  a1 += h[size_t (2 * k + 1)] * b[k];
    o0 = 2.f * a0;
    o1 = 2.f * a1;
}

float HalfbandOS::down (float i0, float i1)
{
    dnPos = (dnPos == 0 ? kTaps - 1 : dnPos - 1);
    dnBuf[size_t (dnPos)] = dnBuf[size_t (dnPos + kTaps)] = i0;
    const float* b = &dnBuf[size_t (dnPos)];
    float acc = 0.f;
    for (int j = 0; j < kTaps; ++j) acc += h[size_t (j)] * b[j];
    dnPos = (dnPos == 0 ? kTaps - 1 : dnPos - 1);
    dnBuf[size_t (dnPos)] = dnBuf[size_t (dnPos + kTaps)] = i1;
    return acc;
}

// --------------------------------------------------------------- TapeEngine
void TapeEngine::prepare (double sampleRate)
{
    sr = sampleRate;
    for (size_t c = 0; c < ch.size(); ++c) ch[c].rng = 0x1234567u + uint32_t (c) * 7919u;
    reset();
    rebuild();
    for (int c = 0; c < 2; ++c) { smRec[c] = recGain[c]; smRep[c] = repGain[c]; }
}

void TapeEngine::reset()
{
    for (auto& c : ch)
    {
        c.recEq.reset(); c.repEq.reset(); c.hfLoss.reset(); c.bump.reset();
        c.calShelf.reset(); c.dcBlock.reset(); c.os.reset();
        c.dl.fill (0.f); c.wp = 0;
    }
    wowPh = fl1Ph = fl2Ph = 0.0;
    rndMod = 0.f;
}

void TapeEngine::setSettings (const Settings& ns)
{
    s = ns;
    rebuild();
}

void TapeEngine::rebuild()
{
    const int sp = std::clamp (s.speed, 0, 2);
    const int tp = std::clamp (s.tape, 0, kNumTapes - 1);
    const int wd = std::clamp (s.width, 0, 3);
    const auto& tv = kTapes[tp];
    const float ips = kIps[sp];

    // Enfasis (constantes de tiempo): NAB 7.5/15 = 50 us (3183 Hz), 30 ips = 17.5 us (9095 Hz);
    // IEC/CCIR 7.5 = 70 us, 15 = 35 us, 30 = 17.5 us.
    static constexpr double nab[3] { 3183.0, 3183.0, 9095.0 };
    static constexpr double iec[3] { 2274.0, 4547.0, 9095.0 };
    const double eqHz = std::min ((s.eq == 0 ? nab : iec)[sp], sr * 0.45);
    constexpr double emphDb = 9.0;

    static constexpr float calDb[3] { 0.f, 3.f, 6.f };
    const float cg = dbToLin (calDb[std::clamp (s.cal, 0, 2)]);

    static constexpr float speedHead[3] { -2.0f, 0.0f, 1.5f };
    satS = dbToLin (tv.satDb + speedHead[sp] - s.bias * 2.0f);
    biasOff = s.bias * 0.10f;

    for (int c = 0; c < 2; ++c)
    {
        recGain[c] = dbToLin (s.inGainDb + s.recDb[c]) * cg * kFluxRef;
        repGain[c] = dbToLin (s.repDb[c] + s.levelDb) / (cg * kFluxRef);
    }

    static constexpr float widthSnr[4] { -3.f, 0.f, 3.f, 6.f };
    static constexpr float speedSnr[3] { -2.f, 0.f, 2.f };
    const float snr = 62.f + widthSnr[wd] + speedSnr[sp] - tv.noiseDb;
    noiseAmp = s.noise ? dbToLin (-snr) * 1.7320508f : 0.f;   // ruido blanco uniforme -> rms

    static constexpr double fcBase[3] { 13000.0, 19500.0, 27000.0 };
    double fc = fcBase[sp] * tv.hfScale * std::pow (2.0, -double (s.bias) * 0.25);
    if (s.route == 1) fc *= 0.75;                        // cabeza sync: mas perdida de HF
    fc = std::min (fc, sr * 0.45);

    static constexpr float widthBump[4] { 0.8f, 1.0f, 1.1f, 1.2f };
    const double bumpDb = s.headBump ? tv.bumpDb * widthBump[wd] : 0.0;

    static constexpr float wfSpeed[3] { 1.4f, 1.0f, 0.7f };
    const float amt = s.wfOn ? s.wowFlutter : 0.f;
    wowDev = 0.00035f * amt * wfSpeed[sp];
    flDev  = 0.00018f * amt * wfSpeed[sp];

    for (auto& c : ch)
    {
        c.recEq.highShelf (sr, eqHz, +emphDb);
        c.repEq.highShelf (sr, eqHz, -emphDb);
        c.hfLoss.lowPass  (sr, fc, 0.6);
        c.bump.peak       (sr, 3.7 * double (ips), 0.9, bumpDb);
        c.calShelf.highShelf (sr, std::min (8000.0, sr * 0.4), double (s.calTilt) * 3.0);
        c.dcBlock.highPass (sr, 5.0, 0.707);
    }
}

float TapeEngine::readDelay (const Chan& c, float d) const
{
    const float p = float (c.wp) - d;
    const int   ip = int (std::floor (p));
    const float f = p - float (ip);
    auto at = [&c] (int i) { return c.dl[size_t (i & 1023)]; };
    const float xm1 = at (ip - 1), x0 = at (ip), x1 = at (ip + 1), x2 = at (ip + 2);
    const float c1 = 0.5f * (x1 - xm1);
    const float c2 = xm1 - 2.5f * x0 + 2.f * x1 - 0.5f * x2;
    const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
    return ((c3 * f + c2) * f + c1) * f + x0;
}

void TapeEngine::process (float* l, float* r, int n)
{
    float* io[2] { l, r };
    const int nch = r ? 2 : 1;
    const float k = 0.0015f;
    const float tb = std::tanh (biasOff);
    const float norm = 1.f / (1.f - tb * tb);
    const float invS = 1.f / satS;

    const double twoPi = 2.0 * kPi;
    const double wowA = double (wowDev) / (twoPi * 0.55);
    const double fl1A = double (flDev) / (twoPi * 6.2);
    const double fl2A = double (flDev) * 0.6 / (twoPi * 11.3);
    const double rndA = double (flDev) * 0.8 / (twoPi * 5.0);

    for (int i = 0; i < n; ++i)
    {
        wowPh += twoPi * 0.55 / sr;  if (wowPh > twoPi) wowPh -= twoPi;
        fl1Ph += twoPi * 6.2  / sr;  if (fl1Ph > twoPi) fl1Ph -= twoPi;
        fl2Ph += twoPi * 11.3 / sr;  if (fl2Ph > twoPi) fl2Ph -= twoPi;
        rndMod += 0.0013f * (xorshiftUniform (modRng) * 1.7320508f - rndMod);
        const double dSec = wowA * std::sin (wowPh) + fl1A * std::sin (fl1Ph)
                          + fl2A * std::sin (fl2Ph) + rndA * double (rndMod) * 6.0;
        const float delay = float (kModCentre + dSec * sr);

        for (int c = 0; c < nch; ++c)
        {
            auto& st = ch[size_t (c)];
            smRec[c] += k * (recGain[c] - smRec[c]);
            smRep[c] += k * (repGain[c] - smRep[c]);

            float x = io[c][i] * smRec[c];
            x = st.recEq.process (x);

            float u0, u1;
            st.os.up (x, u0, u1);
            u0 = satS * (std::tanh (u0 * invS + biasOff) - tb) * norm;
            u1 = satS * (std::tanh (u1 * invS + biasOff) - tb) * norm;
            x = st.os.down (u0, u1);

            x = st.dcBlock.process (x);
            x = st.bump.process (x);
            if (noiseAmp > 0.f) x += noiseAmp * xorshiftUniform (st.rng);

            st.dl[size_t (st.wp)] = x;
            x = readDelay (st, delay);
            st.wp = (st.wp + 1) & 1023;

            x = st.repEq.process (x);
            x = st.hfLoss.process (x);
            x = st.calShelf.process (x);
            io[c][i] = x * smRep[c];
        }
    }

    // Procesado de canal: mono / flip
    if (r)
    {
        if (s.mono)
            for (int i = 0; i < n; ++i) { const float m = 0.5f * (l[i] + r[i]); l[i] = r[i] = m; }
        else if (s.flip)
            for (int i = 0; i < n; ++i) std::swap (l[i], r[i]);
    }
}
} // namespace atr
