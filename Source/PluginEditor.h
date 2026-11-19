// (c) 2026 Diamont Vault(R). All rights reserved.
#pragma once
#include "PluginProcessor.h"
#include "Layout.h"
#include <functional>
#include <memory>
#include <vector>

// Tapa de perilla dibujada por codigo sobre la perilla del panel, con indicador que gira.
class KnobLook : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
};

class AtrEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit AtrEditor (AtrProcessor&);
    ~AtrEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    struct Btn
    {
        Layout::R rect;
        juce::String label;                 // vacio = zona invisible (power, selectores de medidor)
        std::function<bool()> lit;
        std::function<void()> click;
        bool isPower = false;
    };

    struct Knob
    {
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
        Layout::KnobPos pos {};
        juce::String name, unit;
    };

    void timerCallback() override;
    void addButton (Layout::R r, const juce::String& label, std::function<bool()> lit,
                    std::function<void()> click, bool isPower = false);
    void addKnob (Knob& k, const char* paramId, Layout::KnobPos pos, const juce::String& name,
                  const juce::String& unit, float def);
    void buildButtons();

    bool isOn (const char* id) const;
    int  choice (const char* id) const;
    void setChoice (const char* id, int v);
    void toggle (const char* id);
    void flash (const juce::String& text);
    juce::String displayText() const;
    juce::Rectangle<float> sc (Layout::R r) const;
    float vuToDeg (float vu) const;
    void drawNeedle (juce::Graphics&, int ch, const Layout::R& window, juce::Point<float> pivot) const;

    AtrProcessor& proc;
    KnobLook knobLook;

    juce::Image bgOff, bgOn, reelLsrc, reelRsrc, needleImg, stripPatch;   // originales
    juce::Image bgOffS, bgOnS, reelLS, reelRS;                            // escalados al tamano actual

    Knob kIn, kRecL, kRecR, kRepL, kRepR, kBias, kCal, kLevel, kWow;
    std::vector<Btn> buttons;
    bool syncing = false;

    // animacion
    float speedNow = 0.f, tapeFrac = 0.5f, tapePhase = 0.25f, angL = 0.f, angR = 0.f;
    double tapeTime = 0.0;
    double lastMs = 0.0;
    float needleDeg[2] { Layout::vuRestDeg, Layout::vuRestDeg };
    float needleVel[2] { 0.f, 0.f };
    juce::String flashText;
    juce::uint32 flashUntil = 0;
    juce::uint32 litHash = 0;
    bool debug = false;
    float scale = 0.45f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AtrEditor)
};
