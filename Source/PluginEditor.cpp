#include "PluginEditor.h"

RajvikDynamicsProAudioProcessorEditor::RajvikDynamicsProAudioProcessorEditor (RajvikDynamicsProAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setResizable (true, true);
    setResizeLimits (620, 420, 1400, 900);
    setSize (900, 580);

    titleLabel.setText ("RAJVIK", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions (28.0f, juce::Font::bold)));
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xffffcc33));
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText ("DYNAMICS PRO  |  STEREO COMPRESSOR", juce::dontSendNotification);
    subtitleLabel.setFont (juce::Font (juce::FontOptions (13.0f)));
    subtitleLabel.setColour (juce::Label::textColourId, juce::Colour (0xffc7c7c7));
    addAndMakeVisible (subtitleLabel);

    configureSlider (threshold, thresholdLabel, "THRESHOLD");
    configureSlider (ratio, ratioLabel, "RATIO");
    configureSlider (attack, attackLabel, "ATTACK");
    configureSlider (release, releaseLabel, "RELEASE");
    configureSlider (mix, mixLabel, "MIX");
    configureSlider (output, outputLabel, "OUTPUT");

    thresholdAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "threshold", threshold);
    ratioAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "ratio", ratio);
    attackAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "attack", attack);
    releaseAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "release", release);
    mixAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "mix", mix);
    outputAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "output", output);
}

void RajvikDynamicsProAudioProcessorEditor::configureSlider (juce::Slider& s, juce::Label& l, const juce::String& name)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 84, 22);
    s.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffffcc33));
    s.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff474747));
    s.setColour (juce::Slider::thumbColourId, juce::Colour (0xffffdd66));
    s.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff292929));
    addAndMakeVisible (s);

    l.setText (name, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
    l.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    l.setColour (juce::Label::textColourId, juce::Colour (0xffd5d5d5));
    addAndMakeVisible (l);
}

void RajvikDynamicsProAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff171719));
    auto bounds = getLocalBounds().toFloat();

    g.setColour (juce::Colour (0xff242427));
    g.fillRoundedRectangle (bounds.reduced (8.0f), 12.0f);

    g.setColour (juce::Colour (0xff343438));
    g.drawRoundedRectangle (bounds.reduced (8.0f), 12.0f, 1.0f);

    auto graph = getLocalBounds().reduced (28, 86).removeFromTop (145).toFloat();
    g.setColour (juce::Colour (0xff202024));
    g.fillRoundedRectangle (graph, 6.0f);
    g.setColour (juce::Colour (0xff34343a));
    for (int i = 1; i < 6; ++i)
    {
        const float y = graph.getY() + graph.getHeight() * i / 6.0f;
        g.drawHorizontalLine (juce::roundToInt (y), graph.getX(), graph.getRight());
    }
    g.setColour (juce::Colour (0xffffcc33));
    juce::Path curve;
    curve.startNewSubPath (graph.getX(), graph.getBottom() - 8.0f);
    curve.cubicTo (graph.getX() + graph.getWidth() * 0.25f, graph.getBottom() - 12.0f,
                   graph.getX() + graph.getWidth() * 0.35f, graph.getY() + graph.getHeight() * 0.62f,
                   graph.getX() + graph.getWidth() * 0.52f, graph.getY() + graph.getHeight() * 0.48f);
    curve.cubicTo (graph.getX() + graph.getWidth() * 0.7f, graph.getY() + graph.getHeight() * 0.34f,
                   graph.getX() + graph.getWidth() * 0.82f, graph.getY() + graph.getHeight() * 0.2f,
                   graph.getRight(), graph.getY() + 4.0f);
    g.strokePath (curve, juce::PathStrokeType (2.0f));
}

void RajvikDynamicsProAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (24);
    auto header = area.removeFromTop (42);
    titleLabel.setBounds (header.removeFromLeft (170));
    subtitleLabel.setBounds (header);

    area.removeFromTop (190);
    auto row = area.removeFromTop (area.getHeight() - 20);
    const int width = row.getWidth() / 6;
    juce::Component* sliders[] = { &threshold, &ratio, &attack, &release, &mix, &output };
    juce::Component* labels[] = { &thresholdLabel, &ratioLabel, &attackLabel, &releaseLabel, &mixLabel, &outputLabel };

    for (int i = 0; i < 6; ++i)
    {
        auto cell = row.removeFromLeft (i == 5 ? row.getWidth() : width);
        labels[i]->setBounds (cell.removeFromTop (24));
        sliders[i]->setBounds (cell.reduced (4));
    }
}
