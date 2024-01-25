#pragma once

#include <JuceHeader.h>

class MDADetuneAudioProcessor : public juce::AudioProcessor
{
public:
    MDADetuneAudioProcessor();
    ~MDADetuneAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;

    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override;

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String &newName) override;

    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts { *this, nullptr, "Parameters", createParameterLayout() };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void update();
    void resetState();

    float sampleRate;

    static constexpr int BUFMAX = 4096;
    float buf[BUFMAX];
    float win[BUFMAX];

    int buflen;           //buffer length
    float bufres;           //buffer resolution display
    float semi;             //detune display
    int pos0;             //buffer input
    float pos1, dpos1;      //buffer output, rate
    float pos2, dpos2;      //downwards shift
    float wet, dry;         //ouput levels

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MDADetuneAudioProcessor)
};
