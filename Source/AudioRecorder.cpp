#include "AudioRecorder.h"

//==============================================================================
// Recording struct implementation
//==============================================================================

juce::ValueTree Recording::toValueTree() const
{
    juce::ValueTree vt("RECORDING");
    vt.setProperty("uid", uid, nullptr);
    vt.setProperty("name", name, nullptr);
    vt.setProperty("file", file.getFullPathName(), nullptr);
    vt.setProperty("duration", durationInSeconds, nullptr);
    vt.setProperty("tags", tags.joinIntoString(","), nullptr);
    vt.setProperty("timestamp", (juce::int64)timestamp.toMilliseconds(), nullptr);
    vt.setProperty("sampleRate", sampleRate, nullptr);
    vt.setProperty("numChannels", numChannels, nullptr);
    return vt;
}

Recording Recording::fromValueTree(const juce::ValueTree& vt)
{
    Recording r;
    if (!vt.hasType("RECORDING")) return r;
    
    r.uid = vt.getProperty("uid").toString();
    r.name = vt.getProperty("name").toString();
    r.file = juce::File(vt.getProperty("file").toString());
    r.durationInSeconds = (double)vt.getProperty("duration");
    r.sampleRate = (double)vt.getProperty("sampleRate");
    r.numChannels = (int)vt.getProperty("numChannels");
    r.timestamp = juce::Time((juce::int64)vt.getProperty("timestamp"));
    
    auto tagsString = vt.getProperty("tags").toString();
    r.tags.addTokens(tagsString, ",", "");
    r.tags.removeEmptyStrings();
    if (r.tags.isEmpty())
        r.tags.add("Untagged");
        
    return r;
}

juce::String Recording::generateUID()
{
    auto now = juce::Time::getCurrentTime();
    return "rec_" + now.formatted("%Y%m%d_%H%M%S") + "_" + 
           juce::String(juce::Random::getSystemRandom().nextInt(1000));
}

//==============================================================================
// AudioRecorder implementation
//==============================================================================

AudioRecorder::AudioRecorder() : Thread("AudioRecorder")
{
    formatManager.registerBasicFormats();
    
    // Set up recordings directory
    auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                          .getChildFile("CapSure");
    recordingsDirectory = appDataDir.getChildFile("Recordings");
    recordingsDirectory.createDirectory();
}

AudioRecorder::~AudioRecorder()
{
    stopRecording();
    stopThread(5000);
}

bool AudioRecorder::startLoopbackRecording()
{
    if (recording.load())
        return false;

    currentRecordingUID = Recording::generateUID();
    currentRecordingFile = recordingsDirectory.getChildFile(currentRecordingUID + ".wav");
    
    // Start the loopback capture
    bool success = loopbackCapture.start([this](const float** channelData, int numChannels, int numFrames, double sampleRate)
    {
        handleIncomingAudioData(channelData, numChannels, numFrames, sampleRate);
    });

    if (success)
    {
        recording = true;
        recordingStartTime = juce::Time::getCurrentTime();
        samplesRecorded = 0;
        startThread();
        
        if (onStatusChanged)
            onStatusChanged("Recording started: " + currentRecordingUID);
    }
    else
    {
        if (onError)
            onError("Failed to start loopback capture");
    }

    return success;
}

void AudioRecorder::stopRecording()
{
    if (!recording.load())
        return;
        
    recording = false;
    loopbackCapture.stop();
    
    // Signal the thread to finalize the recording
    signalThreadShouldExit();
    notify();
    
    if (onStatusChanged)
        onStatusChanged("Recording stopped");
}

bool AudioRecorder::isRecording() const
{
    return recording.load();
}

void AudioRecorder::run()
{
    while (!threadShouldExit())
    {
        if (!recording.load())
        {
            // Recording stopped, finalize it
            finalizeRecording();
            break;
        }
        
        // Just wait for recording to finish
        wait(100);
    }
}

void AudioRecorder::handleIncomingAudioData(const float** channelData, int numChannels, int numFrames, double sampleRate)
{
    if (!recording.load())
        return;

    // Initialize writer if needed
    if (!audioWriter && sampleRate > 0)
    {
        recordingSampleRate = sampleRate;
        recordingChannels = numChannels;
        
        if (!createAudioWriter(sampleRate, numChannels))
        {
            if (onError)
                onError("Failed to create audio writer");
            return;
        }
    }

    // Write audio data
    if (audioWriter)
    {
        // Convert de-interleaved float data to JUCE AudioBuffer format
        juce::AudioBuffer<float> buffer(numChannels, numFrames);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            if (channelData[ch])
                buffer.copyFrom(ch, 0, channelData[ch], numFrames);
        }
        
        audioWriter->writeFromAudioSampleBuffer(buffer, 0, numFrames);
        samplesRecorded += numFrames;
    }
}

bool AudioRecorder::createAudioWriter(double sampleRate, int numChannels)
{
    if (currentRecordingFile == juce::File())
        return false;

    auto fileOutputStream = currentRecordingFile.createOutputStream();
    if (!fileOutputStream)
        return false;

    juce::WavAudioFormat wavFormat;
    audioWriter.reset(wavFormat.createWriterFor(fileOutputStream.release(), 
                                               sampleRate, 
                                               (unsigned int)numChannels, 
                                               16, // 16-bit
                                               {}, 
                                               0));
    
    return audioWriter != nullptr;
}

void AudioRecorder::finalizeRecording()
{
    // Close the audio writer
    audioWriter.reset();
    
    // Calculate duration
    double duration = 0.0;
    if (recordingSampleRate.load() > 0)
        duration = (double)samplesRecorded.load() / recordingSampleRate.load();
    
    // Create recording metadata
    if (currentRecordingFile.existsAsFile() && duration > 0.1) // Only save if > 0.1 seconds
    {
        Recording newRecording;
        newRecording.uid = currentRecordingUID;
        newRecording.name = "Internal Audio " + recordingStartTime.formatted("%H:%M:%S");
        newRecording.file = currentRecordingFile;
        newRecording.durationInSeconds = duration;
        newRecording.timestamp = recordingStartTime;
        newRecording.sampleRate = recordingSampleRate.load();
        newRecording.numChannels = recordingChannels.load();
        newRecording.tags.add("Loopback");
        newRecording.tags.add("Internal");
        
        if (onRecordingComplete)
            onRecordingComplete(newRecording);
    }
    else
    {
        // Delete empty or very short recordings
        if (currentRecordingFile.existsAsFile())
            currentRecordingFile.deleteFile();
    }
    
    // Reset state
    currentRecordingFile = juce::File();
    currentRecordingUID = "";
    recordingSampleRate = 0.0;
    recordingChannels = 0;
    samplesRecorded = 0;
}