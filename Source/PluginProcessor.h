// (c) 2026 Diamont Vault(R). All rights reserved.
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <vector>
#include "TapeEngine.h"

namespace ID
{
inline constexpr const char* power = "power";   inline constexpr const char* bypass = "bypass";
inline constexpr const char* speed = "speed";   inline constexpr const char* tape = "tape";
inline constexpr const char* width = "width";   inline constexpr const char* cal = "cal";
inline constexpr const char* eq = "eq";         inline constexpr const char* route = "route";
inline constexpr const char* mono = "mono";     inline constexpr const char* flip = "flip";
inline constexpr const char* wfOn = "wfOn";     inline constexpr const char* noise = "noise";
inline constexpr const char* bump = "bump";     inline constexpr const char* link = "link";
inline constexpr const char* meterPeak = "meterPeak"; inline constexpr const char* meterInput = "meterInput";
inline constexpr const char* inGain = "inGain"; inline constexpr const char* recL = "recL";
inline constexpr const char* recR = "recR";     inline constexpr const char* repL = "repL";
inline constexpr const char* repR = "repR";     inline constexpr const char* bias = "bias";
inline constexpr const char* calTilt = "calTilt"; inline constexpr const char* level = "level";
inline constexpr const char* wf = "wf";
}

class AtrProcessor : public juce::AudioProcessor
{
public:
    enum class Transport { Stop, Play, Rec, FF, Rew, Pause };

    AtrProcessor();
    ~AtrProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "ATR-102 MASTERING TAPE RECORDER PRO"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- interfaz con el editor (hilo de mensajes)
    void requestTransport (Transport t);
    Transport getTransport() const noexcept { return transport.load(); }
    float getReelTarget() const noexcept { return reelTarget.load(); }      // -8..8
    bool  audioIsRunning() const noexcept;                                  // hubo bloques recientes
    float getVuDb (int ch) const noexcept { return vuDb[ch & 1].load(); }
    double getSampleRate() const noexcept { return currentSr; }

    static int numPresets();
    static juce::String presetName (int i);
    void loadPreset (int index);
    int  getPresetIndex() const;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParams();
    float raw (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    atr::Settings readSettings() const;
    void setParam (const char* id, float plainValue);
    void updateMeters (const juce::AudioBuffer<float>& b, bool useInputGain);

    atr::TapeEngine engine;
    double currentSr = 48000.0;

    std::atomic<Transport> transport { Transport::Play };
    std::atomic<float> reelTarget { 0.f };
    std::atomic<float> vuDb[2] { { -40.f }, { -40.f } };
    std::atomic<juce::uint32> lastBlockMs { 0 };

    // audio thread
    juce::AudioBuffer<float> dryBuf;
    std::vector<float> dryRing[2];
    int dryPos = 0, dryLen = 1;
    float mix = 0.f;
    bool engineIdle = true;
    bool lastPower = false;
    double vuMs[2] { 0.0, 0.0 };      // integrador cuadratico (VU)
    double peakEnv[2] { 0.0, 0.0 };   // envolvente de pico

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AtrProcessor)
};
