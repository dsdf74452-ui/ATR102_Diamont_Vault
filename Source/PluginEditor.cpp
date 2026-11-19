// (c) 2026 Diamont Vault(R). All rights reserved.
#include "PluginEditor.h"
#include <BinaryData.h>
#include <cmath>

namespace
{
constexpr float kPi = juce::MathConstants<float>::pi;

// Sentido de giro de las bobinas en PLAY (vista frontal). En equipos profesionales con
// la cinta bobinada oxido-afuera ambas bobinas giran en sentido ANTIHORARIO al avanzar.
// JUCE: angulo positivo = horario. Cambia a +1.f si tu referencia es la contraria.
constexpr float kPlayDirection = -1.f;

juce::Image load (const void* d, int n) { return juce::ImageCache::getFromMemory (d, n); }
}

// ------------------------------------------------------------------ perilla
void KnobLook::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                 float startAngle, float endAngle, juce::Slider&)
{
    const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (1.f);
    const auto c = b.getCentre();
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const float ang = startAngle + pos * (endAngle - startAngle);

    // sombra
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (b.translated (r * 0.05f, r * 0.10f));

    // cuerpo oscuro con borde moleteado
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3b3b3b), c.x - r, c.y - r,
                                             juce::Colour (0xff0b0b0b), c.x + r, c.y + r, false));
    g.fillEllipse (c.x - r, c.y - r, r * 2.f, r * 2.f);

    g.setColour (juce::Colour (0xff8a8a8a).withAlpha (0.55f));
    const int teeth = 40;
    for (int i = 0; i < teeth; ++i)
    {
        const float a = (float) i / (float) teeth * 2.f * kPi;
        const juce::Point<float> p0 { c.x + std::sin (a) * r * 0.90f, c.y - std::cos (a) * r * 0.90f };
        const juce::Point<float> p1 { c.x + std::sin (a) * r * 0.99f, c.y - std::cos (a) * r * 0.99f };
        g.drawLine ({ p0, p1 }, juce::jmax (1.f, r * 0.03f));
    }

    // cara plateada
    const float rf = r * 0.80f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff2f2f2), c.x - rf * 0.5f, c.y - rf * 0.6f,
                                             juce::Colour (0xff7d7d7d), c.x + rf * 0.7f, c.y + rf * 0.8f, true));
    g.fillEllipse (c.x - rf, c.y - rf, rf * 2.f, rf * 2.f);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawEllipse (c.x - rf, c.y - rf, rf * 2.f, rf * 2.f, juce::jmax (1.f, r * 0.05f));

    // indicador
    const juce::Point<float> a0 { c.x + std::sin (ang) * rf * 0.30f, c.y - std::cos (ang) * rf * 0.30f };
    const juce::Point<float> a1 { c.x + std::sin (ang) * rf * 0.92f, c.y - std::cos (ang) * rf * 0.92f };
    g.setColour (juce::Colour (0xff141414));
    g.drawLine ({ a0, a1 }, juce::jmax (2.f, r * 0.13f));
    g.setColour (juce::Colour (0xffffc24a));
    g.drawLine ({ a0 + (a1 - a0) * 0.45f, a1 }, juce::jmax (1.2f, r * 0.06f));
}

// ------------------------------------------------------------------ editor
AtrEditor::AtrEditor (AtrProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    bgOff      = load (BinaryData::bg_off_png,      BinaryData::bg_off_pngSize);
    bgOn       = load (BinaryData::bg_on_png,       BinaryData::bg_on_pngSize);
    reelLsrc   = load (BinaryData::reel_left_png,   BinaryData::reel_left_pngSize);
    reelRsrc   = load (BinaryData::reel_right_png,  BinaryData::reel_right_pngSize);
    needleImg  = load (BinaryData::needle_png,      BinaryData::needle_pngSize);
    stripPatch = load (BinaryData::strip_patch_png, BinaryData::strip_patch_pngSize);

    addKnob (kIn,    ID::inGain, Layout::kInGain, "INPUT GAIN", " dB", 0.f);
    addKnob (kRecL,  ID::recL,   Layout::kRecL,   "RECORD L",   " dB", 0.f);
    addKnob (kRecR,  ID::recR,   Layout::kRecR,   "RECORD R",   " dB", 0.f);
    addKnob (kRepL,  ID::repL,   Layout::kRepL,   "REPRO L",    " dB", 0.f);
    addKnob (kRepR,  ID::repR,   Layout::kRepR,   "REPRO R",    " dB", 0.f);
    addKnob (kBias,  ID::bias,   Layout::kBias,   "BIAS",       "",    0.f);
    addKnob (kCal,   ID::calTilt, Layout::kCal,   "CAL (HF)",   "",    0.f);
    addKnob (kLevel, ID::level,  Layout::kLevel,  "LEVEL",      " dB", 0.f);
    addKnob (kWow,   ID::wf,     Layout::kWow,    "WOW/FLUTTER", "",   0.5f);

    // LINK MODE: los trims REC y REPRO de L y R se mueven juntos
    auto linkPair = [this] (Knob& a, Knob& b)
    {
        auto follow = [this] (Knob& from, Knob& to)
        {
            auto prev = from.slider.onValueChange;
            from.slider.onValueChange = [this, &from, &to, prev]
            {
                if (prev) prev();
                if (isOn (ID::link) && ! syncing)
                {
                    syncing = true;
                    to.slider.setValue (from.slider.getValue(), juce::sendNotificationSync);
                    syncing = false;
                }
            };
        };
        follow (a, b); follow (b, a);
    };
    linkPair (kRecL, kRecR);
    linkPair (kRepL, kRepR);

    buildButtons();

    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio ((double) Layout::W / (double) Layout::H);
    getConstrainer()->setMinimumSize (511, 449);
    getConstrainer()->setMaximumSize (2044, 1798);
    setSize (920, 809);
    setWantsKeyboardFocus (true);

    lastMs = juce::Time::getMillisecondCounterHiRes();
    startTimerHz (60);
}

AtrEditor::~AtrEditor()
{
    stopTimer();
    for (auto* k : { &kIn, &kRecL, &kRecR, &kRepL, &kRepR, &kBias, &kCal, &kLevel, &kWow })
        k->slider.setLookAndFeel (nullptr);
}

void AtrEditor::addKnob (Knob& k, const char* id, Layout::KnobPos pos, const juce::String& name,
                         const juce::String& unit, float def)
{
    k.pos = pos; k.name = name; k.unit = unit;
    auto& s = k.slider;
    s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    const float d = pos.deg * kPi / 180.f;
    s.setRotaryParameters (2.f * kPi - d, 2.f * kPi + d, true);
    s.setMouseDragSensitivity (220);
    s.setLookAndFeel (&knobLook);
    addAndMakeVisible (s);
    k.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, s);
    s.setDoubleClickReturnValue (true, def);

    auto* kp = &k;
    s.onValueChange = [this, kp]
    {
        const double v = kp->slider.getValue();
        flash (kp->name + "  " + (v >= 0 && kp->unit.isNotEmpty() ? "+" : "") + juce::String (v, 2) + kp->unit);
    };
}

bool AtrEditor::isOn (const char* id) const { return proc.apvts.getRawParameterValue (id)->load() > 0.5f; }
int  AtrEditor::choice (const char* id) const { return (int) proc.apvts.getRawParameterValue (id)->load(); }

void AtrEditor::setChoice (const char* id, int v)
{
    if (auto* p = proc.apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 ((float) v));
}

void AtrEditor::toggle (const char* id)
{
    if (auto* p = proc.apvts.getParameter (id))
        p->setValueNotifyingHost (isOn (id) ? 0.f : 1.f);
}

void AtrEditor::flash (const juce::String& t)
{
    flashText = t;
    flashUntil = juce::Time::getMillisecondCounter() + 1600u;
}

void AtrEditor::addButton (Layout::R r, const juce::String& label, std::function<bool()> lit,
                           std::function<void()> click, bool isPower)
{
    buttons.push_back ({ r, label, std::move (lit), std::move (click), isPower });
}

void AtrEditor::buildButtons()
{
    using T = AtrProcessor::Transport;
    auto trans = [this] (T t) { return [this, t] { proc.requestTransport (t); }; };
    auto isT   = [this] (T t) { return [this, t] { return proc.getTransport() == t; }; };
    auto choiceBtn = [this] (Layout::R r, const juce::String& label, const char* id, int v)
    {
        addButton (r, label, [this, id, v] { return choice (id) == v; }, [this, id, v] { setChoice (id, v); });
    };
    auto toggleBtn = [this] (Layout::R r, const juce::String& label, const char* id)
    {
        addButton (r, label, [this, id] { return isOn (id); }, [this, id] { toggle (id); });
    };

    // power
    addButton (Layout::powerZone, {}, [this] { return isOn (ID::power); }, [this] { toggle (ID::power); }, true);

    // transporte (fila superior)
    addButton (Layout::tr[0], "REW",   isT (T::Rew),   trans (T::Rew));
    addButton (Layout::tr[1], "STOP",  isT (T::Stop),  trans (T::Stop));
    addButton (Layout::tr[2], "PLAY",  isT (T::Play),  trans (T::Play));
    addButton (Layout::tr[3], "REC",   [this] { return proc.getTransport() == T::Rec; }, trans (T::Rec));
    addButton (Layout::tr[4], "FF",    isT (T::FF),    trans (T::FF));
    addButton (Layout::tr[5], "PAUSE", isT (T::Pause), trans (T::Pause));

    // emphasis EQ / reel control / link mode
    choiceBtn (Layout::eqBtn[0], "NAB", ID::eq, 0);
    choiceBtn (Layout::eqBtn[1], "IEC", ID::eq, 1);
    toggleBtn (Layout::reelBtn[0], "W/F",   ID::wfOn);
    toggleBtn (Layout::reelBtn[1], "NOISE", ID::noise);
    addButton (Layout::linkBtn[0], "LINK",  [this] { return isOn (ID::link); },  [this] { if (auto* p = proc.apvts.getParameter (ID::link)) p->setValueNotifyingHost (1.f); });
    addButton (Layout::linkBtn[1], "INDEP", [this] { return ! isOn (ID::link); }, [this] { if (auto* p = proc.apvts.getParameter (ID::link)) p->setValueNotifyingHost (0.f); });

    // tipo y ancho de cinta, calibracion
    const char* tapes[8] { "456", "499", "GP9", "250", "468", "911", "LPR90", "996" };
    for (int i = 0; i < 8; ++i) choiceBtn (Layout::tapeBtn (i), tapes[i], ID::tape, i);
    const char* widths[4] { "1/4\"", "1/2\"", "1\"", "2\"" };
    for (int i = 0; i < 4; ++i) choiceBtn (Layout::widthBtn (i), widths[i], ID::width, i);
    const char* cals[3] { "0 dB", "+3 dB", "+6 dB" };
    for (int i = 0; i < 3; ++i) choiceBtn (Layout::calBtn (i), cals[i], ID::cal, i);

    // output routing
    choiceBtn (Layout::routeBtn (0), "INPUT", ID::route, 0);
    choiceBtn (Layout::routeBtn (1), "SYNC",  ID::route, 1);
    choiceBtn (Layout::routeBtn (2), "REPRO", ID::route, 2);
    addButton (Layout::routeBtn (3), "STEREO", [this] { return ! isOn (ID::mono); }, [this] { if (auto* p = proc.apvts.getParameter (ID::mono)) p->setValueNotifyingHost (0.f); });
    addButton (Layout::routeBtn (4), "MONO",   [this] { return isOn (ID::mono); },  [this] { if (auto* p = proc.apvts.getParameter (ID::mono)) p->setValueNotifyingHost (1.f); });
    toggleBtn (Layout::routeBtn (5), "FLIP", ID::flip);

    // velocidad
    choiceBtn (Layout::speedBtn[0], "7.5 IPS", ID::speed, 0);
    choiceBtn (Layout::speedBtn[1], "15 IPS",  ID::speed, 1);
    choiceBtn (Layout::speedBtn[2], "30 IPS",  ID::speed, 2);

    // fila inferior: presets, bypass, head bump
    addButton (Layout::lowBtn[0], "PRESET -", [] { return false; },
               [this] { proc.loadPreset (proc.getPresetIndex() - 1); flash ("PRESET: " + AtrProcessor::presetName (proc.getPresetIndex())); });
    addButton (Layout::lowBtn[1], "PRESET +", [] { return false; },
               [this] { proc.loadPreset (proc.getPresetIndex() + 1); flash ("PRESET: " + AtrProcessor::presetName (proc.getPresetIndex())); });
    toggleBtn (Layout::lowBtn[2], "BYPASS", ID::bypass);
    toggleBtn (Layout::lowBtn[3], "BUMP",   ID::bump);

    // selectores de medidor (zonas invisibles sobre las etiquetas impresas)
    auto setP = [this] (const char* id, float v) { return [this, id, v] { if (auto* p = proc.apvts.getParameter (id)) p->setValueNotifyingHost (v); }; };
    for (auto z : { Layout::meterVuZoneL, Layout::meterVuZoneR })     addButton (z, {}, nullptr, setP (ID::meterPeak, 0.f));
    for (auto z : { Layout::meterPeakZoneL, Layout::meterPeakZoneR }) addButton (z, {}, nullptr, setP (ID::meterPeak, 1.f));
    addButton (Layout::meterInZone,  {}, nullptr, setP (ID::meterInput, 1.f));
    addButton (Layout::meterOutZone, {}, nullptr, setP (ID::meterInput, 0.f));
}

juce::Rectangle<float> AtrEditor::sc (Layout::R r) const { return r * scale; }

void AtrEditor::resized()
{
    scale = (float) getWidth() / Layout::W;
    const int w = getWidth(), h = getHeight();
    bgOffS = bgOff.rescaled (w, h, juce::Graphics::highResamplingQuality);
    bgOnS  = bgOn.rescaled  (w, h, juce::Graphics::highResamplingQuality);
    reelLS = reelLsrc.rescaled (juce::roundToInt (Layout::reelLd * scale), juce::roundToInt (Layout::reelLd * scale), juce::Graphics::highResamplingQuality);
    reelRS = reelRsrc.rescaled (juce::roundToInt (Layout::reelRd * scale), juce::roundToInt (Layout::reelRd * scale), juce::Graphics::highResamplingQuality);

    for (auto* k : { &kIn, &kRecL, &kRecR, &kRepL, &kRepR, &kBias, &kCal, &kLevel, &kWow })
    {
        const float d = k->pos.d * scale;
        k->slider.setBounds (juce::Rectangle<float> (k->pos.x * scale - d * 0.5f, k->pos.y * scale - d * 0.5f, d, d).toNearestInt());
    }
    litHash = 0;
}

float AtrEditor::vuToDeg (float vu) const
{
    const auto* t = Layout::vuTable;
    constexpr int n = (int) (sizeof (Layout::vuTable) / sizeof (Layout::vuTable[0]));
    if (vu <= t[0].vu) return t[0].deg;
    for (int i = 1; i < n; ++i)
        if (vu <= t[i].vu)
            return t[i - 1].deg + (t[i].deg - t[i - 1].deg) * (vu - t[i - 1].vu) / (t[i].vu - t[i - 1].vu);
    return t[n - 1].deg;
}

void AtrEditor::drawNeedle (juce::Graphics& g, int ch, const Layout::R& window, juce::Point<float> pivot) const
{
    if (! needleImg.isValid()) return;
    constexpr float hubX = 39.5f, hubY = 337.f;              // eje de giro dentro de needle.png
    const float k = Layout::vuNeedleLen * scale / hubY;
    const float rad = needleDeg[ch] * kPi / 180.f;
    const auto t = juce::AffineTransform::translation (-hubX, -hubY).scaled (k).rotated (rad)
                       .translated (pivot.x * scale, pivot.y * scale);
    g.saveState();
    g.reduceClipRegion (sc (window).toNearestInt());
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImageTransformed (needleImg, t, false);
    g.restoreState();
}

juce::String AtrEditor::displayText() const
{
    if (juce::Time::getMillisecondCounter() < flashUntil) return flashText;
    const juce::String dot = juce::String::charToString ((juce::juce_wchar) 0x2022);
    const juce::String sep = "  " + dot + "  ";
    if (! isOn (ID::power)) return "TIME --:--:--" + sep + "STANDBY";

    const int total = (int) tapeTime;
    const juce::String tm = juce::String::formatted ("%02d:%02d:%02d", total / 3600, (total / 60) % 60, total % 60);
    const char* ips[3] { "7.5 IPS", "15 IPS", "30 IPS" };
    return "TIME " + tm + sep + "SR " + juce::String (proc.getSampleRate() / 1000.0, 1) + "K" + sep
         + (isOn (ID::mono) ? "MONO" : "STEREO") + sep + ips[juce::jlimit (0, 2, choice (ID::speed))];
}

void AtrEditor::paint (juce::Graphics& g)
{
    const bool power = isOn (ID::power);
    g.drawImageAt (power ? bgOnS : bgOffS, 0, 0);

    // botones apagados: restaura el aspecto "off" sobre el panel encendido
    if (power)
        for (auto& b : buttons)
            if (b.label.isNotEmpty() && ! (b.lit && b.lit()))
            {
                g.saveState();
                g.reduceClipRegion (sc (b.rect.expanded (2.f)).toNearestInt());
                g.drawImageAt (bgOffS, 0, 0);
                g.restoreState();
            }

    // bobinas (sobre las bobinas impresas del panel)
    auto drawReel = [&g, this] (const juce::Image& img, float cx, float cy, float ang)
    {
        if (! img.isValid()) return;
        const auto t = juce::AffineTransform::rotation (ang, img.getWidth() * 0.5f, img.getHeight() * 0.5f)
                           .translated (cx * scale - img.getWidth() * 0.5f, cy * scale - img.getHeight() * 0.5f);
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImageTransformed (img, t, false);
    };
    drawReel (reelLS, Layout::reelLx, Layout::reelLy, angL);
    drawReel (reelRS, Layout::reelRx, Layout::reelRy, angR);

    // pantalla inferior: tapa el texto impreso y escribe el estado
    {
        const auto r = sc (Layout::strip);
        g.drawImage (stripPatch, r.getX(), r.getY(), r.getWidth(), r.getHeight(),
                     0, 0, stripPatch.getWidth(), stripPatch.getHeight());
        g.setColour (juce::Colour (0xff3a2f24));
        g.setFont (juce::Font (juce::FontOptions (17.f * scale, juce::Font::bold)));
        g.drawText (displayText(), r, juce::Justification::centred, false);
    }

    // etiquetas de los botones
    g.setFont (juce::Font (juce::FontOptions (16.f * scale, juce::Font::bold)));
    for (auto& b : buttons)
    {
        if (b.label.isEmpty()) continue;
        const bool lit = power && b.lit && b.lit();
        g.setColour (lit ? juce::Colour (0xff3b1d00)
                         : juce::Colour (0xffd8d4cc).withAlpha (power ? 0.88f : 0.50f));
        g.drawText (b.label, sc (b.rect), juce::Justification::centred, false);
    }

    // rotulo del knob W/F
    g.setColour (juce::Colour (0xffd8d4cc).withAlpha (power ? 0.9f : 0.55f));
    g.setFont (juce::Font (juce::FontOptions (15.f * scale, juce::Font::bold)));
    g.drawText ("WOW / FLUTTER", sc ({ Layout::kWow.x - 80.f, Layout::kWow.y + 46.f, 160.f, 22.f }), juce::Justification::centred, false);

    // subrayado de los selectores de medidor
    if (power)
    {
        g.setColour (juce::Colour (0xffffb347));
        const bool pk = isOn (ID::meterPeak), in = isOn (ID::meterInput) || proc.getTransport() == AtrProcessor::Transport::Rec;
        auto line = [&] (float x0, float x1) { g.fillRect (sc ({ x0, 295.f, x1 - x0, 3.f })); };
        if (pk) { line (692.f, 736.f); line (1556.f, 1600.f); } else { line (622.f, 647.f); line (1486.f, 1511.f); }
        if (in) line (836.f, 879.f); else line (893.f, 962.f);
    }

    // agujas VU
    drawNeedle (g, 0, Layout::vuWindowL, Layout::vuPivotL);
    drawNeedle (g, 1, Layout::vuWindowR, Layout::vuPivotR);
}

void AtrEditor::paintOverChildren (juce::Graphics& g)
{
    if (! debug) return;
    g.setColour (juce::Colours::red);
    for (auto& b : buttons) g.drawRect (sc (b.rect), 1.f);
    g.setColour (juce::Colours::cyan);
    for (auto* k : { &kIn, &kRecL, &kRecR, &kRepL, &kRepR, &kBias, &kCal, &kLevel, &kWow })
        g.drawRect (k->slider.getBounds().toFloat(), 1.f);
    g.setColour (juce::Colours::yellow);
    g.drawEllipse (sc ({ Layout::reelLx - Layout::reelLd * 0.5f, Layout::reelLy - Layout::reelLd * 0.5f, Layout::reelLd, Layout::reelLd }), 1.f);
    g.drawEllipse (sc ({ Layout::reelRx - Layout::reelRd * 0.5f, Layout::reelRy - Layout::reelRd * 0.5f, Layout::reelRd, Layout::reelRd }), 1.f);
    g.setColour (juce::Colours::lime);
    g.drawRect (sc (Layout::vuWindowL), 1.f); g.drawRect (sc (Layout::vuWindowR), 1.f);
    g.drawRect (sc (Layout::strip), 1.f);
    g.setFont (14.f);
    g.drawText ("DEBUG - D para ocultar", 8, 6, 220, 16, juce::Justification::left);
}

void AtrEditor::mouseDown (const juce::MouseEvent& e)
{
    const auto p = e.position;
    if (e.mods.isCtrlDown() && sc (Layout::strip).contains (p)) { debug = ! debug; repaint(); return; }

    const bool power = isOn (ID::power);
    for (auto& b : buttons)
        if (sc (b.rect).contains (p))
        {
            if (! power && ! b.isPower) return;
            if (b.click) b.click();
            repaint();
            return;
        }
}

bool AtrEditor::keyPressed (const juce::KeyPress& k)
{
    if (k.getTextCharacter() == 'd' || k.getTextCharacter() == 'D') { debug = ! debug; repaint(); return true; }
    return false;
}

void AtrEditor::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const float dt = juce::jlimit (0.001f, 0.1f, (float) ((now - lastMs) * 0.001));
    lastMs = now;

    const bool power = isOn (ID::power);
    using T = AtrProcessor::Transport;
    const auto t = proc.getTransport();

    // velocidad objetivo de las bobinas
    float target = 0.f;
    if (power)
    {
        if (t == T::FF)  target = 8.f;
        else if (t == T::Rew) target = -8.f;
        else if ((t == T::Play || t == T::Rec) && proc.audioIsRunning()) target = proc.getReelTarget();
    }
    const float tau = (std::abs (target) < 0.01f) ? 0.9f : 0.35f;
    speedNow += (target - speedNow) * (1.f - std::exp (-dt / tau));
    if (std::abs (speedNow) < 0.0005f && target == 0.f) speedNow = 0.f;

    // geometria de los bobinados: la cinta pasa de la bobina izquierda a la derecha
    const float ipsRatio = atr::kIps[juce::jlimit (0, 2, choice (ID::speed))] / 15.f;
    tapeFrac += speedNow * ipsRatio * dt * 0.0015f;
    if (tapeFrac > 0.92f) tapeFrac = 0.08f;
    if (tapeFrac < 0.08f) tapeFrac = 0.92f;
    auto packRadius = [] (float fill) { return std::sqrt (0.12f + fill * (1.f - 0.12f)); };  // 0..1 relativo
    const float rSupply = packRadius (1.f - tapeFrac), rTake = packRadius (tapeFrac);
    const float k = 2.4f * ipsRatio * speedNow;               // rad/s con la bobina a radio medio
    angL += kPlayDirection * k * (0.7f / rSupply) * dt;
    angR += kPlayDirection * k * (0.7f / rTake)   * dt;
    angL = std::fmod (angL, 2.f * kPi);  angR = std::fmod (angR, 2.f * kPi);

    tapeTime = juce::jmax (0.0, tapeTime + (double) (speedNow * dt));

    // agujas: sistema de segundo orden ligeramente subamortiguado
    for (int c = 0; c < 2; ++c)
    {
        const float goal = power ? vuToDeg (proc.getVuDb (c)) : Layout::vuRestDeg;
        const float w = power ? 38.f : 14.f, z = 0.85f;
        const float acc = w * w * (goal - needleDeg[c]) - 2.f * z * w * needleVel[c];
        needleVel[c] += acc * dt;
        needleDeg[c] += needleVel[c] * dt;
    }

    // estado de botones -> repintado completo solo si cambia
    juce::uint32 h = power ? 1u : 0u;
    for (size_t i = 0; i < buttons.size(); ++i)
        if (buttons[i].lit && buttons[i].lit()) h = h * 31u + (juce::uint32) (i + 7);
    h = h * 31u + (isOn (ID::meterPeak) ? 3u : 5u) + (isOn (ID::meterInput) ? 11u : 13u);
    h = h * 31u + (juce::Time::getMillisecondCounter() < flashUntil ? 1u : 0u);
    if (h != litHash) { litHash = h; repaint(); return; }

    // repintado parcial: bobinas, agujas, pantalla
    const auto reelBox = [this] (float cx, float cy, float d) { return sc ({ cx - d * 0.5f - 2.f, cy - d * 0.5f - 2.f, d + 4.f, d + 4.f }).toNearestInt(); };
    if (std::abs (speedNow) > 0.0005f)
    {
        repaint (reelBox (Layout::reelLx, Layout::reelLy, Layout::reelLd));
        repaint (reelBox (Layout::reelRx, Layout::reelRy, Layout::reelRd));
    }
    repaint (sc (Layout::vuWindowL).toNearestInt());
    repaint (sc (Layout::vuWindowR).toNearestInt());
    repaint (sc (Layout::strip).toNearestInt());
}
