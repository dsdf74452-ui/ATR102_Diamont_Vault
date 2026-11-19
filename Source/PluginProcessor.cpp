// (c) 2026 Diamont Vault(R). All rights reserved.
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
struct Preset
{
    const char* name;
    int speed, tape, width, cal, eq;
    float in, bias, tilt, level, wf;
    bool bump, noise, wfOn;
};

// Puntos de partida para escuchar y calibrar (no son mediciones de maquina real).
const Preset kPresets[] =
{
    { "Init - Reference (flat)",   1, 0, 1, 0, 0,  0.0f,  0.0f, 0.f, 0.f, 0.0f, false, false, false },
    { "Master Glue 15 ips 456",    1, 0, 1, 1, 0,  0.0f,  0.0f, 0.f, 0.f, 0.3f, true,  true,  true  },
    { "Transparent 30 ips 499",    2, 1, 2, 0, 1,  0.0f,  0.1f, 0.f, 0.f, 0.2f, true,  true,  true  },
    { "Warm 7.5 ips 456",          0, 0, 1, 1, 0,  1.5f, -0.2f, 0.f, 0.f, 0.6f, true,  true,  true  },
    { "Drum Bus 15 ips 499 Hot",   1, 1, 1, 2, 0,  2.0f,  0.3f, 0.f, -1.f, 0.35f, true, true,  true  },
    { "Vocal Smooth 15 ips GP9",   1, 2, 1, 1, 1,  3.0f,  0.0f, 0.2f, 0.f, 0.3f, true, true,  true  },
    { "Vintage 7.5 ips LPR90",     0, 6, 0, 2, 0,  1.0f, -0.3f, -0.2f, 0.f, 0.9f, true, true,  true  },
    { "Tape Bus 30 ips 996",       2, 7, 2, 1, 1,  1.5f,  0.1f, 0.f, 0.f, 0.25f, true, true,  true  },
};
constexpr int kNumPresets = int (sizeof (kPresets) / sizeof (kPresets[0]));
}

AtrProcessor::AtrProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createParams())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout AtrProcessor::createParams()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto b = [&p] (const char* id, const char* name, bool def)
    { p.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def)); };
    auto c = [&p] (const char* id, const char* name, StringArray items, int def)
    { p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def)); };
    auto f = [&p] (const char* id, const char* name, float lo, float hi, float def)
    { p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (lo, hi, 0.01f), def)); };

    b (ID::power, "Power", false);
    b (ID::bypass, "Bypass", false);
    c (ID::speed, "Tape Speed", { "7.5 ips", "15 ips", "30 ips" }, 1);
    c (ID::tape, "Tape Type", { "456", "499", "GP9", "250", "468", "911", "LPR90", "996" }, 0);
    c (ID::width, "Tape Width", { "1/4\"", "1/2\"", "1\"", "2\"" }, 1);
    c (ID::cal, "Calibration", { "0 dB", "+3 dB", "+6 dB" }, 0);
    c (ID::eq, "Emphasis EQ", { "NAB", "IEC" }, 0);
    c (ID::route, "Output Routing", { "Input", "Sync", "Repro" }, 2);
    b (ID::mono, "Mono", false);
    b (ID::flip, "Flip L/R", false);
    b (ID::wfOn, "Wow/Flutter On", true);
    b (ID::noise, "Noise On", true);
    b (ID::bump, "Head Bump", true);
    b (ID::link, "Link L/R", true);
    b (ID::meterPeak, "Meter Peak", false);
    b (ID::meterInput, "Meter Input", false);

    f (ID::inGain, "Input Gain", -12.f, 12.f, 0.f);
    f (ID::recL, "Record L", -2.f, 2.f, 0.f);
    f (ID::recR, "Record R", -2.f, 2.f, 0.f);
    f (ID::repL, "Reproduce L", -2.f, 2.f, 0.f);
    f (ID::repR, "Reproduce R", -2.f, 2.f, 0.f);
    f (ID::bias, "Bias", -1.f, 1.f, 0.f);
    f (ID::calTilt, "Cal (HF trim)", -1.f, 1.f, 0.f);
    f (ID::level, "Level", -12.f, 12.f, 0.f);
    f (ID::wf, "Wow/Flutter", 0.f, 1.f, 0.5f);

    return { p.begin(), p.end() };
}

atr::Settings AtrProcessor::readSettings() const
{
    atr::Settings s;
    s.speed = int (raw (ID::speed));
    s.tape  = int (raw (ID::tape));
    s.width = int (raw (ID::width));
    s.cal   = int (raw (ID::cal));
    s.eq    = int (raw (ID::eq));
    s.route = int (raw (ID::route));
    s.inGainDb = raw (ID::inGain);
    s.recDb[0] = raw (ID::recL);  s.recDb[1] = raw (ID::recR);
    s.repDb[0] = raw (ID::repL);  s.repDb[1] = raw (ID::repR);
    s.bias = raw (ID::bias);
    s.calTilt = raw (ID::calTilt);
    s.levelDb = raw (ID::level);
    s.wowFlutter = raw (ID::wf);
    s.headBump = raw (ID::bump) > 0.5f;
    s.noise = raw (ID::noise) > 0.5f;
    s.wfOn = raw (ID::wfOn) > 0.5f;
    s.mono = raw (ID::mono) > 0.5f;
    s.flip = raw (ID::flip) > 0.5f;
    return s;
}

bool AtrProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto& in = l.getMainInputChannelSet();
    const auto& out = l.getMainOutputChannelSet();
    if (in != out) return false;
    return in == juce::AudioChannelSet::stereo() || in == juce::AudioChannelSet::mono();
}

void AtrProcessor::prepareToPlay (double sr, int samplesPerBlock)
{
    currentSr = sr;
    engine.prepare (sr);
    dryLen = atr::TapeEngine::latencySamples();
    setLatencySamples (dryLen);
    for (auto& r : dryRing) r.assign (size_t (dryLen), 0.f);
    dryPos = 0;
    dryBuf.setSize (2, juce::jmax (samplesPerBlock, 1), false, true, true);
    mix = 0.f;
    engineIdle = true;
    vuMs[0] = vuMs[1] = 0.0;
    peakEnv[0] = peakEnv[1] = 0.0;
}

bool AtrProcessor::audioIsRunning() const noexcept
{
    return juce::Time::getMillisecondCounter() - lastBlockMs.load() < 300u;
}

void AtrProcessor::requestTransport (Transport t)
{
    if (raw (ID::power) < 0.5f) return;
    const auto cur = transport.load();
    switch (t)
    {
        case Transport::Rec:   // interlock como en la maquina: REC solo desde PLAY
            if (cur == Transport::Play) transport.store (Transport::Rec);
            else if (cur == Transport::Rec) transport.store (Transport::Play);
            break;
        case Transport::Pause:
            transport.store (cur == Transport::Pause ? Transport::Play : Transport::Pause);
            break;
        default:
            transport.store (t);
            break;
    }
}

void AtrProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int nCh = juce::jmin (buffer.getNumChannels(), 2);
    if (n == 0 || nCh == 0) return;

    lastBlockMs.store (juce::Time::getMillisecondCounter());

    // ---- estado de encendido y transporte
    const bool power = raw (ID::power) > 0.5f;
    if (power && ! lastPower) transport.store (Transport::Play);
    if (! power && lastPower) transport.store (Transport::Stop);
    lastPower = power;
    const auto t = transport.load();

    bool hostPlaying = true;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            hostPlaying = pos->getIsPlaying();

    float reel = 0.f;
    if (power)
    {
        if (t == Transport::Play || t == Transport::Rec) reel = hostPlaying ? 1.f : 0.f;
        else if (t == Transport::FF)  reel = 8.f;
        else if (t == Transport::Rew) reel = -8.f;
    }
    reelTarget.store (reel);

    const bool tapeActive = power && raw (ID::bypass) < 0.5f
                         && (t == Transport::Play || t == Transport::Rec)
                         && int (raw (ID::route)) != 0;

    const bool meterIn = (t == Transport::Rec) || raw (ID::meterInput) > 0.5f;
    const bool peakMode = raw (ID::meterPeak) > 0.5f;
    const double sr = currentSr;
    const double aVu = 1.0 - std::exp (-1.0 / (sr * 0.065));        // ~99 % en 300 ms
    const double peakDecay = std::pow (10.0, -1.0 / (1.7 * sr));    // 20 dB en 1,7 s

    auto meter = [&] (const juce::AudioBuffer<float>& b, float g)
    {
        for (int c = 0; c < nCh; ++c)
        {
            const float* x = b.getReadPointer (c);
            double ms = vuMs[c], pk = peakEnv[c];
            for (int i = 0; i < n; ++i)
            {
                const double v = double (x[i]) * double (g);
                ms += aVu * (v * v - ms);
                pk = juce::jmax (std::abs (v), pk * peakDecay);
            }
            vuMs[c] = ms; peakEnv[c] = pk;
            const double db = peakMode ? 20.0 * std::log10 (juce::jmax (pk, 1.0e-6)) + 18.0
                                       : 10.0 * std::log10 (juce::jmax (ms, 1.0e-12)) + 18.0;
            vuDb[c].store (float (juce::jlimit (-40.0, 12.0, db)));
        }
        if (nCh == 1) vuDb[1].store (vuDb[0].load());
    };

    if (meterIn)
        meter (buffer, juce::Decibels::decibelsToGain (raw (ID::inGain)));

    // ---- camino seco con la misma latencia que el motor
    if (dryBuf.getNumSamples() < n) dryBuf.setSize (2, n, false, false, true);
    int newPos = dryPos;
    for (int c = 0; c < nCh; ++c)
    {
        const float* in = buffer.getReadPointer (c);
        float* d = dryBuf.getWritePointer (c);
        auto& ring = dryRing[c];
        int p = dryPos;
        for (int i = 0; i < n; ++i)
        {
            d[i] = ring[size_t (p)];
            ring[size_t (p)] = in[i];
            if (++p >= dryLen) p = 0;
        }
        newPos = p;
    }
    dryPos = newPos;

    // ---- motor
    const float target = tapeActive ? 1.f : 0.f;
    if (target > 0.f || mix > 0.0005f)
    {
        if (engineIdle) { engine.reset(); engineIdle = false; }
        engine.setSettings (readSettings());
        engine.process (buffer.getWritePointer (0), nCh > 1 ? buffer.getWritePointer (1) : nullptr, n);

        const float a = float (1.0 - std::exp (-1.0 / (sr * 0.02)));
        for (int i = 0; i < n; ++i)
        {
            mix += a * (target - mix);
            for (int c = 0; c < nCh; ++c)
            {
                const float dry = dryBuf.getSample (c, i);
                float* w = buffer.getWritePointer (c);
                w[i] = dry + mix * (w[i] - dry);
            }
        }
    }
    else
    {
        mix = 0.f;
        engineIdle = true;
        for (int c = 0; c < nCh; ++c)
            buffer.copyFrom (c, 0, dryBuf, c, 0, n);
    }

    if (! meterIn) meter (buffer, 1.f);

    for (int c = nCh; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, n);
}

// ------------------------------------------------------------- presets
int AtrProcessor::numPresets() { return kNumPresets; }
juce::String AtrProcessor::presetName (int i) { return kPresets[juce::jlimit (0, kNumPresets - 1, i)].name; }
int AtrProcessor::getPresetIndex() const { return int (apvts.state.getProperty ("preset", 1)); }

void AtrProcessor::setParam (const char* id, float v)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (v));
}

void AtrProcessor::loadPreset (int index)
{
    index = ((index % kNumPresets) + kNumPresets) % kNumPresets;
    const auto& p = kPresets[index];
    setParam (ID::speed, float (p.speed));  setParam (ID::tape, float (p.tape));
    setParam (ID::width, float (p.width));  setParam (ID::cal, float (p.cal));
    setParam (ID::eq, float (p.eq));
    setParam (ID::inGain, p.in);            setParam (ID::bias, p.bias);
    setParam (ID::calTilt, p.tilt);         setParam (ID::level, p.level);
    setParam (ID::wf, p.wf);
    setParam (ID::bump, p.bump ? 1.f : 0.f);
    setParam (ID::noise, p.noise ? 1.f : 0.f);
    setParam (ID::wfOn, p.wfOn ? 1.f : 0.f);
    for (auto* id : { ID::recL, ID::recR, ID::repL, ID::repR }) setParam (id, 0.f);
    apvts.state.setProperty ("preset", index, nullptr);
}

// --------------------------------------------------------------- estado
juce::AudioProcessorEditor* AtrProcessor::createEditor() { return new AtrEditor (*this); }

void AtrProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void AtrProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AtrProcessor(); }
