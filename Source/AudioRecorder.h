#pragma once

#include <JuceHeader.h>
#include "LoopbackCapture.h"

//==============================================================================
struct Recording
{
    juce::String uid;
    juce::String name;
    juce::File file;
    double durationInSeconds = 0.0;
    juce::StringArray tags;
    juce::Time timestamp;
    double sampleRate = 0.0;
    int numChannels = 0;
    
    // Methods for serialization
    juce::ValueTree toValueTree() const;
    static Recording fromValueTree(const juce::ValueTree& vt);
    static juce::String generateUID();
};

//==============================================================================
class AudioRecorder : public juce::Thread
{
public:
    AudioRecorder();
    ~AudioRecorder() override;

    // Recording control
    bool startLoopbackRecording();
    void stopRecording();
    bool isRecording() const;
    
    // Callbacks
    std::function<void(const juce::String&)> onStatusChanged;
    std::function<void(const Recording&)> onRecordingComplete;
    std::function<void(const juce::String&)> onError;

private:
    void run() override;
    void handleIncomingAudioData(const float** channelData, int numChannels, int numFrames, double sampleRate);
    bool createAudioWriter(double sampleRate, int numChannels);
    void finalizeRecording();

    LoopbackCapture loopbackCapture;
    std::unique_ptr<juce::AudioFormatWriter> audioWriter;
    juce::File currentRecordingFile;
    juce::String currentRecordingUID;
    
    std::atomic<bool> recording{ false };
    std::atomic<double> recordingSampleRate{ 0.0 };
    std::atomic<int> recordingChannels{ 0 };
    std::atomic<juce::int64> samplesRecorded{ 0 };
    juce::Time recordingStartTime;
    
    juce::AudioFormatManager formatManager;
    juce::File recordingsDirectory;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioRecorder)
};