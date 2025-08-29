#pragma once

#include <JuceHeader.h>
#include "AudioRecorder.h"
#include "LibraryManager.h"

//==============================================================================
class DarkLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DarkLookAndFeel();
    
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                            const juce::Colour& backgroundColour,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override;
};

//==============================================================================
class LibraryComponent : public juce::Component, 
                        public juce::TableListBoxModel,
                        private juce::ChangeListener
{
public:
    LibraryComponent(LibraryManager& library);
    ~LibraryComponent() override;

    // TableListBoxModel
    int getNumRows() override;
    void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    void selectedRowsChanged(int lastRowSelected) override;

    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    
    std::function<void(int)> onSelectionChanged;
    std::function<void(int)> onRecordingRemove;
    std::function<void(const Recording&)> onRecordingEdit;
    std::function<void(const Recording&)> onRecordingExport;
    
    juce::TextEditor searchBox;

private:
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void updateContent();
    void showContextMenu(int rowNumber, const juce::MouseEvent& e);

    LibraryManager& libraryManager;
    juce::TableListBox table;
    juce::Label searchLabel;
    juce::Array<Recording> currentRecordings;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibraryComponent)
};

//==============================================================================
class WaveformComponent : public juce::Component, private juce::Timer, private juce::ChangeListener
{
public:
    WaveformComponent();
    ~WaveformComponent() override;
    
    void setAudioFile(const juce::File& file);
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void loadAudioFile();

    juce::AudioFormatManager formatManager;
    juce::AudioThumbnailCache cache;
    juce::AudioThumbnail thumbnail;
    juce::File currentFile;
    bool needsToLoadFile = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformComponent)
};

//==============================================================================
class MetadataEditorComponent : public juce::Component
{
public:
    MetadataEditorComponent(Recording recording);
    ~MetadataEditorComponent() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    Recording getEditedRecording() const;
    
    std::function<void()> onSave;
    std::function<void()> onCancel;

private:
    Recording originalRecording;
    
    juce::Label nameLabel, artistLabel, tagsLabel, genreLabel, trackLabel;
    juce::TextEditor nameEditor, artistEditor, tagsEditor, genreEditor, trackEditor;
    juce::TextButton saveButton, cancelButton;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MetadataEditorComponent)
};

//==============================================================================
/*
    Professional audio capture application with iTunes-style dark interface
*/
class MainComponent : public juce::AudioAppComponent, private juce::ChangeListener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    //==============================================================================
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void setupUI();
    void updateRecordingStatus();
    void onRecordingComplete(const Recording& recording);
    void onLibrarySelectionChanged(int selectedIndex);
    void onRecordingRemove(int index);
    void onRecordingExport(const Recording& recording);
    void showMetadataEditor(const Recording& recording);
    void importAudioFiles();
    void importAudioFolder();
    void exportAudioFile(const Recording& recording);
    
    // UI Components
    juce::TextButton recordButton;
    juce::TextButton importFilesButton;
    juce::TextButton importFolderButton;
    juce::Label statusLabel;
    juce::Label titleLabel;
    std::unique_ptr<LibraryComponent> libraryComponent;
    std::unique_ptr<WaveformComponent> waveformComponent;
    
    // Core functionality
    std::unique_ptr<AudioRecorder> audioRecorder;
    std::unique_ptr<LibraryManager> libraryManager;
    
    // Look and feel
    DarkLookAndFeel darkLookAndFeel;
    
    // State
    int selectedRecordingIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};