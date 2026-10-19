#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class PanelContent;

// The editor hosts one fixed 1200 x 760 panel and scales it to the window size.
class PotatoKickerEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PotatoKickerEditor (PotatoKickerProcessor&);
    ~PotatoKickerEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int panelW = 1200, panelH = 760;

private:
    void timerCallback() override;

    PotatoKickerProcessor& proc;
    std::unique_ptr<PanelContent> panel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PotatoKickerEditor)
};
