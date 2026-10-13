#include "PluginEditor.h"
#include <cmath>

namespace
{
    const juce::Colour bg       (0xff101216);
    const juce::Colour panel    (0xff181c22);
    const juce::Colour panel2   (0xff20252d);
    const juce::Colour border   (0xff333b47);
    const juce::Colour yellow   (0xffffc928);
    const juce::Colour cyan     (0xff29b6f6);
    const juce::Colour pale     (0xffd9e0e8);
    const juce::Colour muted    (0xff8f9baa);
    const juce::Colour green    (0xff66d17a);
    const juce::Colour red      (0xffff5e63);

    float paramValue (const juce::AudioProcessorValueTreeState& state, const char* id)
    {
        return state.getRawParameterValue (id)->load (std::memory_order_relaxed);
    }

    void drawPanel (juce::Graphics& g, juce::Rectangle<int> r, float radius = 9.0f)
    {
        g.setColour (panel);
        g.fillRoundedRectangle (r.toFloat(), radius);
        g.setColour (border);
        g.drawRoundedRectangle (r.toFloat().reduced (0.5f), radius, 1.0f);
    }
}

RajvikDynamicsProAudioProcessorEditor::RajvikDynamicsProAudioProcessorEditor (RajvikDynamicsProAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setResizable (true, true);
    setResizeLimits (1120, 830, 1800, 1200);
    setSize (1280, 850);

    brandLabel.setText ("RAJVIK", juce::dontSendNotification);
    brandLabel.setFont (juce::Font (juce::FontOptions (27.0f, juce::Font::bold)));
    brandLabel.setColour (juce::Label::textColourId, yellow);
    addAndMakeVisible (brandLabel);

    subtitleLabel.setText ("DYNAMICS PRO  /  ADVANCED COMPRESSOR", juce::dontSendNotification);
    subtitleLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
    subtitleLabel.setColour (juce::Label::textColourId, muted);
    addAndMakeVisible (subtitleLabel);

    configureCombo (styleCombo, juce::StringArray {
        "Clean", "Classic", "Opto", "Vocal", "Mastering", "Bus", "Punch", "Pumping",
        "Versatile", "Smooth", "Upward", "TTM", "Op-El", "Vari-Mu"
    });
    configureCombo (characterCombo, juce::StringArray { "Clean", "Tube", "Diode", "Bright" });
    configureCombo (detectorCombo, juce::StringArray { "Peak detector", "RMS detector" });
    configureCombo (sidechainSourceCombo, juce::StringArray { "Internal sidechain", "External sidechain" });
    styleAttachment = std::make_unique<ComboAttachment> (audioProcessor.parameters, "style", styleCombo);
    characterAttachment = std::make_unique<ComboAttachment> (audioProcessor.parameters, "character", characterCombo);
    detectorAttachment = std::make_unique<ComboAttachment> (audioProcessor.parameters, "detector", detectorCombo);
    sidechainSourceAttachment = std::make_unique<ComboAttachment> (audioProcessor.parameters, "sidechainSource", sidechainSourceCombo);

    styleLabel.setText ("COMPRESSION STYLE", juce::dontSendNotification);
    presetLabel.setText ("PRESET", juce::dontSendNotification);
    characterLabel.setText ("CHARACTER", juce::dontSendNotification);
    for (auto* label : { &styleLabel, &presetLabel, &characterLabel })
    {
        label->setColour (juce::Label::textColourId, muted);
        label->setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        addAndMakeVisible (*label);
    }
    addAndMakeVisible (styleCombo);
    addAndMakeVisible (characterCombo);

    configureCombo (presetCombo, juce::StringArray {
        "Default", "Punchy Kick", "Smooth Vocal", "Bus Glue", "EDM Pump", "Mastering Gentle", "Upward Detail"
    });
    presetCombo.onChange = [this] { applyPreset (presetCombo.getSelectedItemIndex()); };
    presetCombo.setSelectedItemIndex (0, juce::dontSendNotification);
    addAndMakeVisible (presetCombo);

    for (auto* button : { &autoThresholdButton, &autoReleaseButton, &autoGainButton,
                          &sidechainEnabledButton, &sidechainListenButton, &characterPreButton })
    {
        button->setColour (juce::ToggleButton::textColourId, pale);
        button->setColour (juce::ToggleButton::tickColourId, yellow);
        button->setColour (juce::ToggleButton::tickDisabledColourId, muted);
        addAndMakeVisible (*button);
    }
    autoThresholdAttachment = std::make_unique<ButtonAttachment> (audioProcessor.parameters, "autoThreshold", autoThresholdButton);
    autoReleaseAttachment = std::make_unique<ButtonAttachment> (audioProcessor.parameters, "autoRelease", autoReleaseButton);
    autoGainAttachment = std::make_unique<ButtonAttachment> (audioProcessor.parameters, "autoGain", autoGainButton);
    sidechainEnabledAttachment = std::make_unique<ButtonAttachment> (audioProcessor.parameters, "sidechainEnabled", sidechainEnabledButton);
    sidechainListenAttachment = std::make_unique<ButtonAttachment> (audioProcessor.parameters, "sidechainListen", sidechainListenButton);
    characterPreAttachment = std::make_unique<ButtonAttachment> (audioProcessor.parameters, "characterPre", characterPreButton);

    for (auto* button : { &saveAButton, &saveBButton, &compareButton, &resetButton, &helpButton })
    {
        button->setColour (juce::TextButton::buttonColourId, panel2);
        button->setColour (juce::TextButton::buttonOnColourId, yellow.withAlpha (0.28f));
        button->setColour (juce::TextButton::textColourOffId, pale);
        button->setColour (juce::TextButton::textColourOnId, yellow);
        addAndMakeVisible (*button);
    }
    snapshotA = audioProcessor.parameters.copyState();
    snapshotB = audioProcessor.parameters.copyState();
    saveAButton.onClick = [this] { snapshotA = audioProcessor.parameters.copyState(); saveAButton.setButtonText ("A Saved"); };
    saveBButton.onClick = [this] { snapshotB = audioProcessor.parameters.copyState(); saveBButton.setButtonText ("B Saved"); };
    compareButton.onClick = [this]
    {
        showingA = ! showingA;
        const auto& state = showingA ? snapshotA : snapshotB;
        if (state.isValid()) audioProcessor.parameters.replaceState (state.createCopy());
        compareButton.setButtonText (showingA ? "A Active" : "B Active");
    };
    resetButton.onClick = [this] { resetParameters(); };
    helpButton.onClick = [this]
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::Info,
            "RAJVIK Dynamics Pro", "RAJVIK Dynamics Pro is an original compressor implementation.\n\n"
            "Choose a style, set Threshold and Ratio, then adjust Attack/Release.\n"
            "Use the sidechain section to filter the detector. External sidechain requires your DAW to enable and route the plugin's sidechain input.\n\n"
            "The 14 styles are inspired by common compressor behaviors; they are not copies of another company's proprietary algorithms.");
    };

    addKnob ("inputGain", "INPUT");
    addKnob ("threshold", "THRESHOLD");
    addKnob ("ratio", "RATIO");
    addKnob ("attack", "ATTACK");
    addKnob ("release", "RELEASE");
    addKnob ("mix", "MIX");
    addKnob ("outputGain", "OUTPUT");
    addKnob ("knee", "KNEE");
    addKnob ("range", "RANGE");
    addKnob ("hold", "HOLD");
    addKnob ("lookahead", "LOOKAHEAD");
    addKnob ("drive", "DRIVE");
    addKnob ("sidechainLevel", "SC LEVEL");
    addKnob ("sidechainHPF", "SC HPF");
    addKnob ("sidechainLPF", "SC LPF");
    for (int i = 1; i <= 6; ++i)
        addKnob ("scBand" + juce::String (i), "EQ " + juce::String (i));

    startTimerHz (30);
}

void RajvikDynamicsProAudioProcessorEditor::configureCombo (juce::ComboBox& combo, const juce::StringArray& items)
{
    combo.addItemList (items, 1);
    combo.setColour (juce::ComboBox::backgroundColourId, panel2);
    combo.setColour (juce::ComboBox::textColourId, pale);
    combo.setColour (juce::ComboBox::outlineColourId, border);
    combo.setColour (juce::ComboBox::arrowColourId, yellow);
    combo.setJustificationType (juce::Justification::centredLeft);
}

void RajvikDynamicsProAudioProcessorEditor::addKnob (const juce::String& id, const juce::String& text)
{
    auto knob = std::make_unique<Knob>();
    knob->id = id;
    knob->slider = std::make_unique<juce::Slider>();
    knob->slider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob->slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 19);
    knob->slider->setColour (juce::Slider::rotarySliderFillColourId, yellow);
    knob->slider->setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff4c5664));
    knob->slider->setColour (juce::Slider::thumbColourId, juce::Colour (0xffffe28a));
    knob->slider->setColour (juce::Slider::textBoxTextColourId, pale);
    knob->slider->setColour (juce::Slider::textBoxBackgroundColourId, panel2);
    knob->slider->setColour (juce::Slider::textBoxOutlineColourId, border);
    knob->label = std::make_unique<juce::Label>();
    knob->label->setText (text, juce::dontSendNotification);
    knob->label->setJustificationType (juce::Justification::centred);
    knob->label->setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    knob->label->setColour (juce::Label::textColourId, pale);
    knob->attachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, id, *knob->slider);
    addAndMakeVisible (*knob->slider);
    addAndMakeVisible (*knob->label);
    knobs.push_back (std::move (knob));
}

void RajvikDynamicsProAudioProcessorEditor::paintKnobPanel (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title)
{
    drawPanel (g, r);
    g.setColour (yellow);
    g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    g.drawText (title, r.reduced (12, 6).removeFromTop (18), juce::Justification::centredLeft);
}

void RajvikDynamicsProAudioProcessorEditor::paintCompressorDisplay (juce::Graphics& g, juce::Rectangle<int> outer)
{
    drawPanel (g, outer);
    auto content = outer.reduced (14, 12);
    const auto metersWidth = 94;
    auto meters = content.removeFromRight (metersWidth);
    auto plot = content.reduced (4, 2);
    g.setColour (juce::Colour (0xff11151b));
    g.fillRoundedRectangle (plot.toFloat(), 5.0f);

    for (int i = 0; i <= 6; ++i)
    {
        const float y = plot.getY() + plot.getHeight() * i / 6.0f;
        g.setColour (juce::Colour (0xff29303a));
        g.drawHorizontalLine (juce::roundToInt (y), static_cast<float> (plot.getX()), static_cast<float> (plot.getRight()));
        g.setFont (juce::Font (juce::FontOptions (9.0f)));
        g.setColour (muted);
        g.drawText (juce::String (-i * 10), plot.getX() + 2, juce::roundToInt (y) - 7, 28, 14, juce::Justification::centredLeft);
    }
    for (int i = 1; i < 10; ++i)
    {
        const float x = plot.getX() + plot.getWidth() * i / 10.0f;
        g.setColour (juce::Colour (0xff222933));
        g.drawVerticalLine (juce::roundToInt (x), static_cast<float> (plot.getY()), static_cast<float> (plot.getBottom()));
    }

    // Scrolling input/output traces read from the DSP, rather than made-up animation.
    juce::Path inPath, outPath;
    const int start = audioProcessor.getScopeWriteIndex();
    for (int i = 0; i < RajvikDynamicsProAudioProcessor::scopeSize; ++i)
    {
        const int index = (start + i) % RajvikDynamicsProAudioProcessor::scopeSize;
        const float x = plot.getX() + plot.getWidth() * static_cast<float> (i) / (RajvikDynamicsProAudioProcessor::scopeSize - 1);
        const float in = audioProcessor.getInputScopeSample (index);
        const float out = audioProcessor.getOutputScopeSample (index);
        const float yIn = plot.getCentreY() - in * plot.getHeight() * 0.34f;
        const float yOut = plot.getCentreY() - out * plot.getHeight() * 0.34f;
        if (i == 0) { inPath.startNewSubPath (x, yIn); outPath.startNewSubPath (x, yOut); }
        else { inPath.lineTo (x, yIn); outPath.lineTo (x, yOut); }
    }
    g.setColour (juce::Colour (0xffaeb8c5).withAlpha (0.70f));
    g.strokePath (inPath, juce::PathStrokeType (1.1f));
    g.setColour (cyan.withAlpha (0.90f));
    g.strokePath (outPath, juce::PathStrokeType (1.2f));

    // Actual static transfer curve from current threshold / ratio / knee settings.
    const float threshold = paramValue (audioProcessor.parameters, "threshold");
    float ratio = juce::jmax (1.0f, paramValue (audioProcessor.parameters, "ratio"));
    const float knee = paramValue (audioProcessor.parameters, "knee");
    const int style = juce::roundToInt (paramValue (audioProcessor.parameters, "style"));
    if (style == 2) ratio = 1.0f + (ratio - 1.0f) * 0.72f;
    if (style == 4) ratio = 1.0f + (ratio - 1.0f) * 0.65f;
    if (style == 9) ratio = 1.0f + (ratio - 1.0f) * 0.80f;
    if (style == 12) ratio = 1.0f + (ratio - 1.0f) * 0.82f;
    if (style == 13) ratio = 1.0f + (ratio - 1.0f) * 0.75f;
    juce::Path transfer;
    for (int i = 0; i <= 120; ++i)
    {
        const float inputDb = -60.0f + 60.0f * static_cast<float> (i) / 120.0f;
        const float over = inputDb - threshold;
        float gainChange = 0.0f;
        if (style == 10 && over < 0.0f)
            gainChange = juce::jmin (paramValue (audioProcessor.parameters, "range"), (-over) * (1.0f - 1.0f / ratio));
        else if (style == 10)
            gainChange = 0.0f;
        else if (style == 11 && over < 0.0f)
            gainChange = juce::jmin (paramValue (audioProcessor.parameters, "range"), (-over) * (1.0f - 1.0f / ratio));
        else if (knee > 0.001f && std::abs (over) < knee * 0.5f)
        {
            const float shifted = over + knee * 0.5f;
            gainChange = (1.0f / ratio - 1.0f) * shifted * shifted / (2.0f * knee);
        }
        else if (over >= 0.0f)
            gainChange = over * (1.0f / ratio - 1.0f);
        const float outDb = inputDb + gainChange;
        const float x = plot.getX() + plot.getWidth() * static_cast<float> (i) / 120.0f;
        const float y = plot.getBottom() - (outDb + 60.0f) / 60.0f * plot.getHeight();
        if (i == 0) transfer.startNewSubPath (x, y); else transfer.lineTo (x, y);
    }
    g.setColour (yellow);
    g.strokePath (transfer, juce::PathStrokeType (2.0f));

    const int thresholdX = plot.getX() + juce::roundToInt ((threshold + 60.0f) / 60.0f * plot.getWidth());
    g.setColour (yellow.withAlpha (0.8f));
    g.drawVerticalLine (thresholdX, static_cast<float> (plot.getY()), static_cast<float> (plot.getBottom()));

    auto legend = outer.reduced (16, 0).withY (outer.getBottom() - 22).withHeight (16);
    g.setFont (juce::Font (juce::FontOptions (10.0f)));
    g.setColour (muted);
    g.drawText ("INPUT", legend.removeFromLeft (54), juce::Justification::centredLeft);
    g.setColour (cyan);
    g.drawText ("OUTPUT", legend.removeFromLeft (62), juce::Justification::centredLeft);
    g.setColour (yellow);
    g.drawText ("TRANSFER / THRESHOLD", legend.removeFromLeft (210), juce::Justification::centredLeft);

    g.setColour (pale);
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    g.drawText ("IN", meters.removeFromTop (15), juce::Justification::centred);
    const auto meterTop = meters.getY() + 8;
    const auto meterHeight = meters.getHeight() - 42;
    auto drawMeter = [&g, meterTop, meterHeight] (int x, float db, juce::Colour colour)
    {
        juce::Rectangle<int> track (x, meterTop, 17, meterHeight);
        g.setColour (juce::Colour (0xff0a0d10));
        g.fillRoundedRectangle (track.toFloat(), 4.0f);
        const float n = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
        auto fill = track;
        fill.setY (track.getBottom() - juce::roundToInt (track.getHeight() * n));
        fill.setHeight (juce::roundToInt (track.getHeight() * n));
        g.setColour (colour);
        g.fillRoundedRectangle (fill.toFloat(), 3.0f);
    };
    drawMeter (outer.getRight() - 77, audioProcessor.getInputLevelDb(), green);
    drawMeter (outer.getRight() - 51, audioProcessor.getOutputLevelDb(), cyan);
    g.setColour (pale);
    g.setFont (juce::Font (juce::FontOptions (9.0f)));
    g.drawText (juce::String (audioProcessor.getInputLevelDb(), 1), outer.getRight() - 88, outer.getBottom() - 20, 43, 12, juce::Justification::centred);
    g.drawText (juce::String (audioProcessor.getOutputLevelDb(), 1), outer.getRight() - 53, outer.getBottom() - 20, 43, 12, juce::Justification::centred);
    g.setColour (red);
    g.drawText ("GR " + juce::String (audioProcessor.getGainReductionDb(), 1) + " dB",
                outer.getRight() - 86, outer.getY() + 5, 78, 16, juce::Justification::centredRight);
}

void RajvikDynamicsProAudioProcessorEditor::paintSidechainEQ (juce::Graphics& g, juce::Rectangle<int> r)
{
    drawPanel (g, r);
    g.setColour (yellow);
    g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    g.drawText ("SIDECHAIN EQ  /  DETECTOR SHAPING", r.reduced (12, 6).removeFromTop (18), juce::Justification::centredLeft);

    auto graph = r.withTrimmedLeft (185).withTrimmedRight (470).withTrimmedTop (30).withTrimmedBottom (18);
    g.setColour (juce::Colour (0xff11151b));
    g.fillRoundedRectangle (graph.toFloat(), 5.0f);
    for (int i = 0; i <= 4; ++i)
    {
        float y = graph.getY() + graph.getHeight() * i / 4.0f;
        g.setColour (juce::Colour (0xff2a3039));
        g.drawHorizontalLine (juce::roundToInt (y), static_cast<float> (graph.getX()), static_cast<float> (graph.getRight()));
    }
    for (int i = 1; i <= 8; ++i)
    {
        float x = graph.getX() + graph.getWidth() * i / 9.0f;
        g.setColour (juce::Colour (0xff252c35));
        g.drawVerticalLine (juce::roundToInt (x), static_cast<float> (graph.getY()), static_cast<float> (graph.getBottom()));
    }

    const juce::Colour colors[6] = { yellow, green, cyan, juce::Colour (0xff408dff), juce::Colour (0xffbf62f1), juce::Colour (0xffff7b28) };
    auto freqToX = [&graph] (float f)
    {
        const float n = (std::log10 (f) - std::log10 (20.0f)) / (std::log10 (20000.0f) - std::log10 (20.0f));
        return graph.getX() + n * graph.getWidth();
    };
    auto gainToY = [&graph] (float db)
    {
        const float n = (12.0f - db) / 24.0f;
        return graph.getY() + n * graph.getHeight();
    };
    juce::Path curve;
    const float hpf = paramValue (audioProcessor.parameters, "sidechainHPF");
    const float lpf = paramValue (audioProcessor.parameters, "sidechainLPF");
    for (int point = 0; point <= 180; ++point)
    {
        const float n = static_cast<float> (point) / 180.0f;
        const float frequency = 20.0f * std::pow (1000.0f, n);
        float responseDb = 0.0f;
        for (int band = 0; band < 6; ++band)
        {
            const auto suffix = juce::String (band + 1);
            const float centre = paramValue (audioProcessor.parameters, ("scFreq" + suffix).toRawUTF8());
            const float gain = paramValue (audioProcessor.parameters, ("scBand" + suffix).toRawUTF8());
            const float q = paramValue (audioProcessor.parameters, ("scQ" + suffix).toRawUTF8());
            const float octaveDistance = std::log2 (frequency / centre);
            const float width = 0.55f / juce::jmax (q, 0.2f);
            responseDb += gain * std::exp (-0.5f * octaveDistance * octaveDistance / (width * width));
        }
        if (paramValue (audioProcessor.parameters, "sidechainEnabled") > 0.5f)
        {
            responseDb += 20.0f * std::log10 (frequency / std::sqrt (frequency * frequency + hpf * hpf));
            responseDb += 20.0f * std::log10 (lpf / std::sqrt (frequency * frequency + lpf * lpf));
        }
        responseDb = juce::jlimit (-12.0f, 12.0f, responseDb);
        const float x = graph.getX() + n * graph.getWidth();
        const float y = gainToY (responseDb);
        if (point == 0) curve.startNewSubPath (x, y); else curve.lineTo (x, y);
    }
    g.setColour (yellow.withAlpha (0.92f));
    g.strokePath (curve, juce::PathStrokeType (2.0f));
    for (int i = 0; i < 6; ++i)
    {
        const auto suffix = juce::String (i + 1);
        const float db = paramValue (audioProcessor.parameters, ("scBand" + suffix).toRawUTF8());
        const float freq = paramValue (audioProcessor.parameters, ("scFreq" + suffix).toRawUTF8());
        g.setColour (colors[i]);
        g.fillEllipse (freqToX (freq) - 4.5f, gainToY (db) - 4.5f, 9.0f, 9.0f);
    }
    g.setColour (muted);
    g.setFont (juce::Font (juce::FontOptions (9.0f)));
    g.drawText ("20 Hz", graph.getX(), graph.getBottom() + 1, 45, 14, juce::Justification::left);
    g.drawText ("1 kHz", graph.getCentreX() - 20, graph.getBottom() + 1, 45, 14, juce::Justification::centred);
    g.drawText ("20 kHz", graph.getRight() - 45, graph.getBottom() + 1, 45, 14, juce::Justification::right);
}

void RajvikDynamicsProAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);
    g.setColour (juce::Colour (0xff252b34));
    g.drawHorizontalLine (55, 16.0f, static_cast<float> (getWidth() - 16));

    const auto w = getWidth();
    paintCompressorDisplay (g, { 16, 72, w - 32, 270 });
    paintKnobPanel (g, { 16, 350, w - 32, 238 }, "MAIN COMPRESSOR CONTROLS");
    paintSidechainEQ (g, { 16, 596, w - 32, getHeight() - 650 });

    // Footer: live telemetry from the DSP.
    const int footerY = getHeight() - 31;
    g.setColour (border);
    g.drawHorizontalLine (footerY - 4, 18.0f, static_cast<float> (w - 18));
    g.setColour (muted);
    g.setFont (juce::Font (juce::FontOptions (10.0f)));
    g.drawText ("REAL-TIME DSP", 24, footerY, 100, 18, juce::Justification::centredLeft);
    g.drawText ("Input " + juce::String (audioProcessor.getInputLevelDb(), 1) + " dB", 150, footerY, 115, 18, juce::Justification::centredLeft);
    g.drawText ("Output " + juce::String (audioProcessor.getOutputLevelDb(), 1) + " dB", 275, footerY, 125, 18, juce::Justification::centredLeft);
    g.drawText ("Gain reduction " + juce::String (audioProcessor.getGainReductionDb(), 1) + " dB", 420, footerY, 210, 18, juce::Justification::centredLeft);
}

void RajvikDynamicsProAudioProcessorEditor::resized()
{
    const int w = getWidth();
    brandLabel.setBounds (20, 9, 150, 37);
    subtitleLabel.setBounds (170, 15, 245, 25);

    const int buttonLeftEdge = w - 320;
    const int comboX = juce::jmax (420, juce::roundToInt (w * 0.34f));
    const int availableComboWidth = juce::jmax (280, buttonLeftEdge - comboX - 20);
    const int styleWidth = juce::jmin (170, juce::roundToInt (availableComboWidth * 0.39f));
    const int presetWidth = juce::jmin (155, juce::roundToInt (availableComboWidth * 0.35f));
    const int characterWidth = juce::jlimit (80, 150, availableComboWidth - styleWidth - presetWidth - 16);
    styleLabel.setBounds (comboX, 4, styleWidth, 14);
    styleCombo.setBounds (comboX, 20, styleWidth, 28);
    const int presetX = comboX + styleWidth + 8;
    presetLabel.setBounds (presetX, 4, presetWidth, 14);
    presetCombo.setBounds (presetX, 20, presetWidth, 28);
    const int characterX = presetX + presetWidth + 8;
    characterLabel.setBounds (characterX, 4, characterWidth, 14);
    characterCombo.setBounds (characterX, 20, characterWidth, 28);

    saveAButton.setBounds (w - 306, 20, 55, 27);
    saveBButton.setBounds (w - 246, 20, 55, 27);
    compareButton.setBounds (w - 186, 20, 52, 27);
    resetButton.setBounds (w - 126, 20, 62, 27);
    helpButton.setBounds (w - 60, 20, 40, 27);

    // Five large central controls, with input/output and timing-shaping controls kept compact.
    const int mainY = 378;
    const int mainH = 194;
    const int left = 25;
    const int right = w - 25;
    const int gap = 3;
    const int usable = right - left;
    const int primaryW = juce::jmin (135, (usable - 530) / 5);
    const int primaryIndices[5] = { 1, 2, 3, 4, 5 }; // Threshold, Ratio, Attack, Release, Mix
    for (int j = 0; j < 5; ++j)
    {
        auto& k = *knobs[static_cast<size_t> (primaryIndices[j])];
        const int x = left + j * (primaryW + gap);
        k.label->setBounds (x, mainY, primaryW, 21);
        k.slider->setBounds (x + 1, mainY + 20, primaryW - 2, mainH - 22);
    }
    const int smallIndices[7] = { 0, 6, 7, 8, 9, 10, 11 }; // Input, Output, Knee, Range, Hold, Lookahead, Drive
    const int smallStart = left + 5 * primaryW + 4 * gap + 10;
    const int smallW = (right - smallStart - 6 * gap) / 7;
    for (int j = 0; j < 7; ++j)
    {
        auto& k = *knobs[static_cast<size_t> (smallIndices[j])];
        const int x = smallStart + j * (smallW + gap);
        k.label->setBounds (x, mainY + 4, smallW, 21);
        k.slider->setBounds (x + 1, mainY + 25, smallW - 2, mainH - 27);
    }

    const int sideY = 622;
    const int sideH = getHeight() - 704;
    const int leftSide = 28;
    const int sideControlW = 58;
    for (int i = 12; i < 15; ++i)
    {
        auto& k = *knobs[static_cast<size_t> (i)];
        const int x = leftSide + (i - 12) * 58;
        k.label->setBounds (x - 2, sideY + 48, sideControlW + 4, 18);
        k.slider->setBounds (x, sideY + 66, sideControlW, juce::jmax (82, sideH - 62));
    }

    const int eqStart = w - 470;
    const int eqW = 66;
    for (int i = 15; i < 21; ++i)
    {
        auto& k = *knobs[static_cast<size_t> (i)];
        const int x = eqStart + (i - 15) * eqW;
        k.label->setBounds (x, sideY + 38, eqW - 2, 18);
        k.slider->setBounds (x + 2, sideY + 57, eqW - 5, juce::jmax (80, sideH - 55));
    }

    sidechainSourceCombo.setBounds (29, 614, 164, 25);
    detectorCombo.setBounds (29, 644, 164, 25);
    sidechainListenButton.setBounds (29, getHeight() - 78, 88, 22);
    sidechainEnabledButton.setBounds (124, getHeight() - 78, 118, 22);
    autoThresholdButton.setBounds (w - 468, 603, 130, 20);
    autoReleaseButton.setBounds (w - 335, 603, 115, 20);
    autoGainButton.setBounds (w - 220, 603, 100, 20);
    characterPreButton.setBounds (w - 116, 603, 70, 20);
}

void RajvikDynamicsProAudioProcessorEditor::applyPreset (int index)
{
    auto set = [this] (const char* id, float value)
    {
        if (auto* param = dynamic_cast<juce::RangedAudioParameter*> (audioProcessor.parameters.getParameter (id)))
            param->setValueNotifyingHost (param->getNormalisableRange().convertTo0to1 (value));
    };

    // Start each preset from known defaults so values from a previous preset do not leak through.
    if (index == 0)
    {
        resetParameters();
        return;
    }
    for (auto* parameter : audioProcessor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter))
            ranged->setValueNotifyingHost (ranged->getDefaultValue());

    // Presets set actual host parameters; the dropdown is not just a graphic.
    switch (index)
    {
        case 1: // Punchy Kick
            set ("style", 6.0f); set ("threshold", -22.0f); set ("ratio", 5.0f); set ("attack", 28.0f);
            set ("release", 85.0f); set ("knee", 2.0f); set ("range", 18.0f); set ("mix", 100.0f); set ("outputGain", 1.5f); break;
        case 2: // Smooth Vocal
            set ("style", 3.0f); set ("threshold", -24.0f); set ("ratio", 3.0f); set ("attack", 8.0f);
            set ("release", 140.0f); set ("knee", 10.0f); set ("range", 12.0f); set ("mix", 100.0f); set ("autoGain", 1.0f); break;
        case 3: // Bus Glue
            set ("style", 5.0f); set ("threshold", -18.0f); set ("ratio", 2.0f); set ("attack", 25.0f);
            set ("release", 180.0f); set ("knee", 12.0f); set ("range", 6.0f); set ("mix", 100.0f); break;
        case 4: // EDM Pump
            set ("style", 7.0f); set ("threshold", -28.0f); set ("ratio", 8.0f); set ("attack", 1.0f);
            set ("release", 110.0f); set ("knee", 0.0f); set ("range", 30.0f); set ("mix", 100.0f); break;
        case 5: // Mastering Gentle
            set ("style", 4.0f); set ("threshold", -12.0f); set ("ratio", 1.5f); set ("attack", 40.0f);
            set ("release", 260.0f); set ("knee", 18.0f); set ("range", 3.0f); set ("mix", 100.0f); set ("autoGain", 1.0f); break;
        case 6: // Upward detail
            set ("style", 10.0f); set ("threshold", -30.0f); set ("ratio", 2.0f); set ("attack", 15.0f);
            set ("release", 220.0f); set ("knee", 6.0f); set ("range", 9.0f); set ("mix", 65.0f); break;
        default:
            resetParameters(); break;
    }
}

void RajvikDynamicsProAudioProcessorEditor::resetParameters()
{
    for (auto* parameter : audioProcessor.getParameters())
    {
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter))
            ranged->setValueNotifyingHost (ranged->getDefaultValue());
    }
    presetCombo.setSelectedItemIndex (0, juce::dontSendNotification);
}


juce::Rectangle<int> RajvikDynamicsProAudioProcessorEditor::getSidechainGraphBounds() const
{
    // Matches the graph inset used in paintSidechainEQ().
    return { 201, 626, juce::jmax (100, getWidth() - 687), juce::jmax (60, getHeight() - 698) };
}

void RajvikDynamicsProAudioProcessorEditor::setParameterFromUI (const juce::String& id, float actualValue)
{
    if (auto* parameter = dynamic_cast<juce::RangedAudioParameter*> (audioProcessor.parameters.getParameter (id)))
        parameter->setValueNotifyingHost (parameter->getNormalisableRange().convertTo0to1 (actualValue));
}

void RajvikDynamicsProAudioProcessorEditor::mouseDown (const juce::MouseEvent& event)
{
    const auto graph = getSidechainGraphBounds().toFloat();
    if (! graph.contains (event.position))
    {
        selectedEQBand = -1;
        return;
    }

    const float x = event.position.x;
    const float y = event.position.y;
    float nearest = 1.0e9f;
    selectedEQBand = -1;
    for (int i = 0; i < 6; ++i)
    {
        const auto suffix = juce::String (i + 1);
        const float freq = paramValue (audioProcessor.parameters, ("scFreq" + suffix).toRawUTF8());
        const float gain = paramValue (audioProcessor.parameters, ("scBand" + suffix).toRawUTF8());
        const float n = (std::log10 (freq) - std::log10 (20.0f)) / (std::log10 (20000.0f) - std::log10 (20.0f));
        const float nodeX = graph.getX() + n * graph.getWidth();
        const float nodeY = graph.getY() + ((12.0f - gain) / 24.0f) * graph.getHeight();
        const float distance = std::sqrt ((x - nodeX) * (x - nodeX) + (y - nodeY) * (y - nodeY));
        if (distance < nearest) { nearest = distance; selectedEQBand = i; }
    }
    if (nearest > 24.0f)
    {
        selectedEQBand = -1;
        return;
    }
    const auto suffix = juce::String (selectedEQBand + 1);
    for (const auto& id : { juce::String ("scFreq" + suffix), juce::String ("scBand" + suffix) })
        if (auto* parameter = audioProcessor.parameters.getParameter (id)) parameter->beginChangeGesture();
    mouseDrag (event);
}

void RajvikDynamicsProAudioProcessorEditor::mouseDrag (const juce::MouseEvent& event)
{
    if (selectedEQBand < 0)
        return;
    const auto graph = getSidechainGraphBounds().toFloat();
    const float x = juce::jlimit (graph.getX(), graph.getRight(), event.position.x);
    const float y = juce::jlimit (graph.getY(), graph.getBottom(), event.position.y);
    const float xNorm = (x - graph.getX()) / graph.getWidth();
    const float yNorm = (y - graph.getY()) / graph.getHeight();
    const float frequency = 20.0f * std::pow (1000.0f, xNorm);
    const float gain = 12.0f - 24.0f * yNorm;
    const auto suffix = juce::String (selectedEQBand + 1);
    setParameterFromUI ("scFreq" + suffix, frequency);
    setParameterFromUI ("scBand" + suffix, gain);
}

void RajvikDynamicsProAudioProcessorEditor::mouseUp (const juce::MouseEvent&)
{
    if (selectedEQBand >= 0)
    {
        const auto suffix = juce::String (selectedEQBand + 1);
        for (const auto& id : { juce::String ("scFreq" + suffix), juce::String ("scBand" + suffix) })
            if (auto* parameter = audioProcessor.parameters.getParameter (id)) parameter->endChangeGesture();
    }
    selectedEQBand = -1;
}

void RajvikDynamicsProAudioProcessorEditor::timerCallback()
{
    repaint();
}
