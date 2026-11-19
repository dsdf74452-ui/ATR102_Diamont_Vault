// ATR-102 MASTERING TAPE RECORDER PRO
// (c) 2026 Diamont Vault(R). All rights reserved.
//
// Motor de cinta en C++ puro (sin dependencias de JUCE) para poder compilarlo y
// medirlo fuera del plugin (tools/test_engine.cpp).
//
// Cadena por canal:
//   ganancia de grabacion (in + cal + trim REC) -> pre-enfasis (NAB/IEC)
//   -> saturacion tipo tanh asimetrica con sobremuestreo 2x -> head bump
//   -> ruido de cinta -> wow/flutter (linea de retardo modulada)
//   -> de-enfasis -> perdida de HF (gap/spacing, velocidad, bias, ruta)
//   -> trim CAL (HF) -> ganancia de reproduccion -> nivel de salida
//
// IMPORTANTE: es un modelo funcional. Los coeficientes por formulacion de cinta
// son "voicings" iniciales, NO mediciones de una maquina real. Ver docs/CALIBRACION.md.
#pragma once
#include <array>
#include <cstdint>
#include <vector>

namespace atr
{
constexpr int kNumTapes = 8;
constexpr float kIps[3] { 7.5f, 15.0f, 30.0f };

struct Settings
{
    int   speed = 1;          // 0 = 7.5, 1 = 15, 2 = 30 ips
    int   tape  = 0;          // 0..7
    int   width = 1;          // 0 = 1/4", 1 = 1/2", 2 = 1", 3 = 2"
    int   cal   = 0;          // 0 = 0 dB, 1 = +3 dB, 2 = +6 dB sobre nivel de referencia
    int   eq    = 0;          // 0 = NAB, 1 = IEC/CCIR
    int   route = 2;          // 0 = Input (sin cinta), 1 = Sync, 2 = Repro

    float inGainDb   = 0.f;   // trim de entrada (-12..12)
    float recDb[2]   { 0.f, 0.f };   // trim REC por canal (-2..2)
    float repDb[2]   { 0.f, 0.f };   // trim REPRO por canal (-2..2)
    float bias       = 0.f;   // -1 (sub-bias) .. +1 (sobre-bias)
    float calTilt    = 0.f;   // -1..1  => +-3 dB en altas (FLAT = 0)
    float levelDb    = 0.f;   // nivel de salida (-12..12)
    float wowFlutter = 0.5f;  // 0..1

    bool  headBump = true;
    bool  noise    = true;
    bool  wfOn     = true;
    bool  mono     = false;
    bool  flip     = false;
};

class Biquad
{
public:
    void reset() { z1 = z2 = 0.f; }
    void setCoeffs (double b0_, double b1_, double b2_, double a0, double a1, double a2)
    {
        b0 = float (b0_ / a0); b1 = float (b1_ / a0); b2 = float (b2_ / a0);
        a1n = float (a1 / a0); a2n = float (a2 / a0);
    }
    void highShelf (double sr, double f, double gainDb, double S = 0.8);
    void peak      (double sr, double f, double q, double gainDb);
    void lowPass   (double sr, double f, double q);
    void highPass  (double sr, double f, double q);

    inline float process (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1n * y + z2;
        z2 = b2 * x - a2n * y;
        return y;
    }
private:
    float b0 = 1.f, b1 = 0.f, b2 = 0.f, a1n = 0.f, a2n = 0.f, z1 = 0.f, z2 = 0.f;
};

// Sobremuestreo x2 con FIR halfband (Kaiser). Latencia total ida+vuelta = kLatency muestras.
class HalfbandOS
{
public:
    static constexpr int kTaps    = 79;
    static constexpr int kLatency = (kTaps - 1) / 2;   // 39 muestras a la tasa base

    HalfbandOS();
    void reset();
    // in -> dos muestras a 2x (o0, o1)
    void up (float in, float& o0, float& o1);
    // dos muestras a 2x -> una muestra a tasa base
    float down (float i0, float i1);
private:
    std::array<float, kTaps> h {};
    std::array<float, kTaps * 2> upBuf {}, dnBuf {};
    int upPos = 0, dnPos = 0;
};

class TapeEngine
{
public:
    // Retardo fijo del wow/flutter (muestras). Latencia total = esto + HalfbandOS::kLatency.
    static constexpr int kModCentre = 64;
    static int latencySamples() { return kModCentre + HalfbandOS::kLatency; }

    void prepare (double sampleRate);
    void reset();
    void setSettings (const Settings& s);

    // Procesa in-place. r puede ser nullptr (mono).
    void process (float* l, float* r, int numSamples);

private:
    struct Chan
    {
        Biquad recEq, repEq, hfLoss, bump, calShelf, dcBlock;
        HalfbandOS os;
        std::array<float, 1024> dl {};
        int wp = 0;
        uint32_t rng = 1u;
    };

    void rebuild();
    float noiseSample (Chan& c);
    float readDelay (const Chan& c, float delaySamples) const;

    double sr = 48000.0;
    Settings s;
    bool dirty = true;
    std::array<Chan, 2> ch;

    // ganancias derivadas
    float recGain[2] { 1.f, 1.f }, repGain[2] { 1.f, 1.f };
    float smRec[2] { 1.f, 1.f }, smRep[2] { 1.f, 1.f };
    float satS = 4.f, biasOff = 0.f, noiseAmp = 0.f, wowDev = 0.f, flDev = 0.f;

    double wowPh = 0.0, fl1Ph = 0.0, fl2Ph = 0.0;
    float  rndMod = 0.f;
    uint32_t modRng = 12345u;
};
} // namespace atr
