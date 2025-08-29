#pragma once

#include <JuceHeader.h>
#include "AudioRecorder.h"

//==============================================================================
class LibraryManager : public juce::ChangeBroadcaster
{
public:
    LibraryManager();
    ~LibraryManager();

    // Library operations
    void addRecording(const Recording& recording);
    void removeRecording(int index);
    void updateRecording(int index, const Recording& recording);
    
    // Import operations
    bool importAudioFile(const juce::File& file, bool copyToLibrary = false);
    void importAudioFiles(const juce::Array<juce::File>& files, bool copyToLibrary = false);
    bool importFolder(const juce::File& folder, bool recursive = false, bool copyToLibrary = false);
    
    // Access
    int getNumRecordings() const { return recordings.size(); }
    const Recording* getRecording(int index) const;
    const juce::Array<Recording>& getAllRecordings() const { return recordings; }
    juce::Array<Recording> getFilteredRecordings(const juce::String& filter = {}) const;
    
    // Persistence
    void saveLibrary();
    void loadLibrary();
    
    // Search and filter
    juce::Array<int> findRecordingsByTag(const juce::String& tag) const;
    juce::Array<int> searchRecordings(const juce::String& searchTerm) const;

private:
    juce::Array<Recording> recordings;
    juce::File libraryFile;
    juce::File recordingsDirectory;
    juce::AudioFormatManager formatManager;
    
    void ensureDirectoriesExist();
    Recording createRecordingFromFile(const juce::File& file);
    bool isAudioFile(const juce::File& file);
    juce::StringArray extractTagsFromFilename(const juce::String& filename);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibraryManager)
};