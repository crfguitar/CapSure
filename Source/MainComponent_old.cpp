#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() 
    : thumbnailCache(5),
      thumbnail(512, formatManager, thumbnailCache)
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (600, 400);

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }
    
    // Setup UI
    addAndMakeVisible(recordButton);
    addAndMakeVisible(playButton);
    addAndMakeVisible(statusLabel);
    
    recordButton.setButtonText("Record");
    recordButton.onClick = [this]() { 
        if (!state)
        {
            startRecording();
        }
        else
        {
            stopRecording();
        }
    };
    
    playButton.setButtonText("Play");
    playButton.setEnabled(false);
    playButton.onClick = [this]() {
        if (lastRecording.existsAsFile())
        {
            statusLabel.setText("Playing: " + lastRecording.getFileName(), juce::dontSendNotification);
        }
    };
    
    statusLabel.setText("Ready to record", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    
    formatManager.registerBasicFormats();
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    juce::ignoreUnused(samplesPerBlockExpected, sampleRate);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Just clear the buffer for now - we're not playing anything back yet
    bufferToFill.clearActiveBufferRegion();
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));
    g.drawFittedText ("CapSure - Audio Recording App", getLocalBounds().removeFromTop(50), juce::Justification::centred, 1);
    
    // Draw a simple waveform visualization area
    auto thumbnailBounds = getLocalBounds().reduced(20).removeFromBottom(100);
    g.setColour(juce::Colours::darkgrey);
    g.fillRect(thumbnailBounds);
    
    g.setColour(juce::Colours::lightblue);
    if (thumbnail.getNumChannels() > 0)
    {
        thumbnail.drawChannels(g, thumbnailBounds, 0.0, thumbnail.getTotalLength(), 1.0f);
    }
    else
    {
        g.setColour(juce::Colours::white);
        g.drawText("No audio loaded", thumbnailBounds, juce::Justification::centred);
    }
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(20);
    
    area.removeFromTop(60); // Space for title
    
    auto buttonArea = area.removeFromTop(80);
    recordButton.setBounds(buttonArea.removeFromLeft(100).reduced(5));
    playButton.setBounds(buttonArea.removeFromLeft(100).reduced(5));
    
    area.removeFromTop(10);
    statusLabel.setBounds(area.removeFromTop(30));
    
    // Leave space for waveform at bottom
    area.removeFromBottom(120);
}

void MainComponent::startRecording()
{
    if (!juce::RuntimePermissions::isGranted(juce::RuntimePermissions::recordAudio))
    {
        statusLabel.setText("Recording permission not granted", juce::dontSendNotification);
        return;
    }
    
    auto parentDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    parentDir = parentDir.getChildFile("CapSure Recordings");
    parentDir.createDirectory();
    
    auto now = juce::Time::getCurrentTime();
    auto filename = "Recording_" + now.formatted("%Y%m%d_%H%M%S") + ".wav";
    lastRecording = parentDir.getChildFile(filename);
    
    state = true;
    recordButton.setButtonText("Stop");
    playButton.setEnabled(false);
    statusLabel.setText("Recording to: " + filename, juce::dontSendNotification);
    
    // Here you would start actual recording - for now just simulate
    startTimer(100);
}

void MainComponent::stopRecording()
{
    state = false;
    recordButton.setButtonText("Record");
    playButton.setEnabled(true);
    statusLabel.setText("Recording saved: " + lastRecording.getFileName(), juce::dontSendNotification);
    
    stopTimer();
    
    // Load the thumbnail (simulate by creating empty file)
    if (!lastRecording.existsAsFile())
    {
        lastRecording.create();
    }
    
    thumbnail.setSource(new juce::FileInputSource(lastRecording));
    repaint();
}

void MainComponent::timerCallback()
{
    // Simple timer callback for recording simulation
    // In a real implementation, this would handle audio recording
}