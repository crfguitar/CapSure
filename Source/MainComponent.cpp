#include "MainComponent.h"

//==============================================================================
// Dark Look and Feel Implementation
//==============================================================================

DarkLookAndFeel::DarkLookAndFeel()
{
    // Dark theme colors inspired by iTunes/Spotify
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff1e1e1e));
    setColour(juce::DocumentWindow::backgroundColourId, juce::Colour(0xff1e1e1e));
    
    // Text colors
    setColour(juce::Label::textColourId, juce::Colour(0xffffffff));
    setColour(juce::TextEditor::textColourId, juce::Colour(0xffffffff));
    setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff2d2d2d));
    setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff404040));
    
    // Button colors
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d2d2d));
    setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff1db954)); // Spotify green
    setColour(juce::TextButton::textColourOffId, juce::Colour(0xffffffff));
    setColour(juce::TextButton::textColourOnId, juce::Colour(0xff000000));
    
    // Table colors  
    setColour(juce::TableListBox::backgroundColourId, juce::Colour(0xff1a1a1a));
    setColour(juce::TableListBox::outlineColourId, juce::Colour(0xff404040));
    setColour(juce::TableHeaderComponent::backgroundColourId, juce::Colour(0xff2d2d2d));
    setColour(juce::TableHeaderComponent::textColourId, juce::Colour(0xffffffff));
    
    // Scrollbar colors
    setColour(juce::ScrollBar::thumbColourId, juce::Colour(0xff404040));
    setColour(juce::ScrollBar::trackColourId, juce::Colour(0xff1a1a1a));
}

void DarkLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                         const juce::Colour& backgroundColour,
                                         bool shouldDrawButtonAsHighlighted,
                                         bool shouldDrawButtonAsDown)
{
    auto buttonArea = button.getLocalBounds().toFloat();
    auto cornerSize = 4.0f;
    
    juce::Colour baseColour = backgroundColour;
    
    if (shouldDrawButtonAsDown)
        baseColour = baseColour.brighter(0.1f);
    else if (shouldDrawButtonAsHighlighted)
        baseColour = baseColour.brighter(0.05f);
    
    g.setColour(baseColour);
    g.fillRoundedRectangle(buttonArea, cornerSize);
    
    // Add subtle border
    g.setColour(baseColour.brighter(0.2f));
    g.drawRoundedRectangle(buttonArea, cornerSize, 1.0f);
}

//==============================================================================
// Library Component Implementation
//==============================================================================

LibraryComponent::LibraryComponent(LibraryManager& library) : libraryManager(library)
{
    addAndMakeVisible(searchLabel);
    addAndMakeVisible(searchBox);
    addAndMakeVisible(table);
    
    searchLabel.setText("Search:", juce::dontSendNotification);
    searchLabel.setJustificationType(juce::Justification::centredRight);
    
    searchBox.setTextToShowWhenEmpty("Search recordings...", juce::Colours::grey);
    searchBox.onTextChange = [this]() { updateContent(); };
    
    // Setup table
    table.getHeader().addColumn("Name", 1, 200, 100, 400);
    table.getHeader().addColumn("Duration", 2, 80, 60, 120);
    table.getHeader().addColumn("Date", 3, 120, 100, 200);
    table.getHeader().addColumn("Tags", 4, 150, 100, 300);
    
    table.setModel(this);
    table.setMultipleSelectionEnabled(false);
    
    libraryManager.addChangeListener(this);
    updateContent();
}

LibraryComponent::~LibraryComponent()
{
    libraryManager.removeChangeListener(this);
}

int LibraryComponent::getNumRows()
{
    return currentRecordings.size();
}

void LibraryComponent::paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
    {
        g.setColour(juce::Colour(0xff1db954).withAlpha(0.3f));
        g.fillAll();
    }
    else if (rowNumber % 2)
    {
        g.setColour(juce::Colour(0xff252525));
        g.fillAll();
    }
}

void LibraryComponent::paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    if (rowNumber >= currentRecordings.size())
        return;
        
    const auto& recording = currentRecordings.getReference(rowNumber);
    
    g.setColour(rowIsSelected ? juce::Colours::white : juce::Colour(0xffffffff));
    g.setFont(juce::FontOptions(14.0f));
    
    juce::String text;
    switch (columnId)
    {
        case 1: text = recording.name; break;
        case 2: text = juce::String(recording.durationInSeconds, 1) + "s"; break;
        case 3: text = recording.timestamp.formatted("%d/%m %H:%M"); break;
        case 4: text = recording.tags.joinIntoString(", "); break;
    }
    
    g.drawText(text, 2, 0, width - 4, height, juce::Justification::centredLeft, true);
}

void LibraryComponent::selectedRowsChanged(int lastRowSelected)
{
    if (onSelectionChanged)
        onSelectionChanged(lastRowSelected);
}

void LibraryComponent::resized()
{
    auto area = getLocalBounds();
    
    auto topArea = area.removeFromTop(30);
    searchLabel.setBounds(topArea.removeFromLeft(60));
    searchBox.setBounds(topArea.removeFromLeft(200));
    
    area.removeFromTop(5);
    table.setBounds(area);
}

void LibraryComponent::changeListenerCallback(juce::ChangeBroadcaster*)
{
    updateContent();
}

void LibraryComponent::updateContent()
{
    auto searchTerm = searchBox.getText();
    currentRecordings = libraryManager.getFilteredRecordings(searchTerm);
    table.updateContent();
    repaint();
}

//==============================================================================
// Waveform Component Implementation  
//==============================================================================

WaveformComponent::WaveformComponent() : cache(5), thumbnail(512, formatManager, cache)
{
    formatManager.registerBasicFormats();
    thumbnail.addChangeListener(this);
}

void WaveformComponent::changeListenerCallback(juce::ChangeBroadcaster*)
{
    repaint();
}

WaveformComponent::~WaveformComponent()
{
    thumbnail.removeChangeListener(this);
}

void WaveformComponent::setAudioFile(const juce::File& file)
{
    if (currentFile != file)
    {
        currentFile = file;
        needsToLoadFile = true;
        startTimer(100);
    }
}

void WaveformComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));
    
    auto area = getLocalBounds().reduced(10);
    
    if (thumbnail.getTotalLength() > 0)
    {
        // Draw waveform
        g.setColour(juce::Colour(0xff1db954));
        thumbnail.drawChannels(g, area, 0.0, thumbnail.getTotalLength(), 1.0f);
        
        // Add border
        g.setColour(juce::Colour(0xff404040));
        g.drawRect(area, 1);
    }
    else
    {
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.setFont(juce::FontOptions(16.0f));
        g.drawText("No audio file selected", area, juce::Justification::centred);
        
        g.setColour(juce::Colour(0xff404040));
        g.drawRect(area, 1);
    }
}

void WaveformComponent::resized()
{
    // Nothing special needed for resize
}

void WaveformComponent::timerCallback()
{
    if (needsToLoadFile)
    {
        loadAudioFile();
        needsToLoadFile = false;
        stopTimer();
    }
}

void WaveformComponent::loadAudioFile()
{
    if (currentFile.existsAsFile())
    {
        thumbnail.setSource(new juce::FileInputSource(currentFile));
    }
    else
    {
        thumbnail.clear();
    }
    repaint();
}

//==============================================================================
// Main Component Implementation
//==============================================================================

MainComponent::MainComponent()
{
    setLookAndFeel(&darkLookAndFeel);
    
    // Initialize core components
    libraryManager = std::make_unique<LibraryManager>();
    audioRecorder = std::make_unique<AudioRecorder>();
    
    // Initialize UI components
    libraryComponent = std::make_unique<LibraryComponent>(*libraryManager);
    waveformComponent = std::make_unique<WaveformComponent>();
    
    setupUI();
    
    // Setup callbacks
    audioRecorder->onRecordingComplete = [this](const Recording& recording) 
    {
        onRecordingComplete(recording);
    };
    
    audioRecorder->onStatusChanged = [this](const juce::String& status)
    {
        juce::MessageManager::callAsync([this, status]()
        {
            statusLabel.setText(status, juce::dontSendNotification);
        });
    };
    
    audioRecorder->onError = [this](const juce::String& error)
    {
        juce::MessageManager::callAsync([this, error]()
        {
            statusLabel.setText("Error: " + error, juce::dontSendNotification);
        });
    };
    
    libraryComponent->onSelectionChanged = [this](int index)
    {
        onLibrarySelectionChanged(index);
    };
    
    libraryManager->addChangeListener(this);
    
    setSize(1200, 800);
    setAudioChannels(0, 2);
}

MainComponent::~MainComponent()
{
    libraryManager->removeChangeListener(this);
    audioRecorder->stopRecording();
    shutdownAudio();
    setLookAndFeel(nullptr);
}

void MainComponent::setupUI()
{
    addAndMakeVisible(titleLabel);
    addAndMakeVisible(recordButton);
    addAndMakeVisible(importFilesButton);
    addAndMakeVisible(importFolderButton);
    addAndMakeVisible(statusLabel);
    addAndMakeVisible(*libraryComponent);
    addAndMakeVisible(*waveformComponent);
    
    titleLabel.setText("CapSure - Internal Audio Recorder", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    
    recordButton.setButtonText("Record Internal Audio");
    recordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1db954));
    recordButton.onClick = [this]() 
    {
        if (audioRecorder->isRecording())
        {
            audioRecorder->stopRecording();
            recordButton.setButtonText("Record Internal Audio");
            recordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1db954));
        }
        else
        {
            if (audioRecorder->startLoopbackRecording())
            {
                recordButton.setButtonText("Stop Recording");
                recordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffe53e3e));
            }
        }
    };
    
    importFilesButton.setButtonText("Import Audio Files");
    importFilesButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff0ea5e9));
    importFilesButton.onClick = [this]() { importAudioFiles(); };
    
    importFolderButton.setButtonText("Import Folder");
    importFolderButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff8b5cf6));
    importFolderButton.onClick = [this]() { importAudioFolder(); };
    
    statusLabel.setText("Ready to record internal audio or import existing files", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    juce::ignoreUnused(samplesPerBlockExpected, sampleRate);
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();
}

void MainComponent::releaseResources()
{
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(10);
    
    // Title area
    titleLabel.setBounds(area.removeFromTop(40));
    area.removeFromTop(10);
    
    // Controls area
    auto controlsArea = area.removeFromTop(100);
    
    // Button row
    auto buttonRow = controlsArea.removeFromTop(40);
    recordButton.setBounds(buttonRow.removeFromLeft(200));
    buttonRow.removeFromLeft(10);
    importFilesButton.setBounds(buttonRow.removeFromLeft(150));
    buttonRow.removeFromLeft(10);
    importFolderButton.setBounds(buttonRow.removeFromLeft(130));
    
    controlsArea.removeFromTop(10);
    statusLabel.setBounds(controlsArea.removeFromTop(30));
    
    area.removeFromTop(10);
    
    // Main content area - split between library and waveform
    auto libraryArea = area.removeFromLeft(400);
    area.removeFromLeft(10); // spacing
    
    libraryComponent->setBounds(libraryArea);
    waveformComponent->setBounds(area);
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster*)
{
    // Library updated, refresh display
}

void MainComponent::updateRecordingStatus()
{
    if (audioRecorder->isRecording())
    {
        statusLabel.setText("Recording internal audio...", juce::dontSendNotification);
    }
    else
    {
        statusLabel.setText("Ready to record internal audio", juce::dontSendNotification);
    }
}

void MainComponent::onRecordingComplete(const Recording& recording)
{
    juce::MessageManager::callAsync([this, recording]()
    {
        libraryManager->addRecording(recording);
        statusLabel.setText("Recording saved: " + recording.name, juce::dontSendNotification);
        
        // Reset button
        recordButton.setButtonText("Record Internal Audio");
        recordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1db954));
    });
}

void MainComponent::onLibrarySelectionChanged(int selectedIndex)
{
    selectedRecordingIndex = selectedIndex;
    
    if (selectedIndex >= 0)
    {
        auto recordings = libraryManager->getFilteredRecordings(libraryComponent->searchBox.getText());
        if (juce::isPositiveAndBelow(selectedIndex, recordings.size()))
        {
            const auto& recording = recordings.getReference(selectedIndex);
            waveformComponent->setAudioFile(recording.file);
        }
    }
    else
    {
        waveformComponent->setAudioFile(juce::File());
    }
}

void MainComponent::importAudioFiles()
{
    auto chooserFlags = juce::FileBrowserComponent::openMode
                      | juce::FileBrowserComponent::canSelectFiles
                      | juce::FileBrowserComponent::canSelectMultipleItems;
                      
    juce::FileChooser chooser("Select audio files to import",
                             juce::File(),
                             "*.wav;*.mp3;*.flac;*.aiff;*.m4a;*.ogg");
    
    chooser.launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
    {
        auto files = fc.getResults();
        if (!files.isEmpty())
        {
            juce::MessageManager::callAsync([this, files]()
            {
                statusLabel.setText("Importing " + juce::String(files.size()) + " files...", 
                                   juce::dontSendNotification);
                
                libraryManager->importAudioFiles(files, false); // Don't copy to library by default
                
                statusLabel.setText("Imported " + juce::String(files.size()) + " audio files", 
                                   juce::dontSendNotification);
            });
        }
    });
}

void MainComponent::importAudioFolder()
{
    auto chooserFlags = juce::FileBrowserComponent::openMode
                      | juce::FileBrowserComponent::canSelectDirectories;
                      
    juce::FileChooser chooser("Select folder containing audio files",
                             juce::File());
    
    chooser.launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
    {
        auto folder = fc.getResult();
        if (folder.exists())
        {
            juce::MessageManager::callAsync([this, folder]()
            {
                statusLabel.setText("Importing files from folder...", juce::dontSendNotification);
                
                if (libraryManager->importFolder(folder, true, false)) // Recursive, don't copy
                {
                    statusLabel.setText("Successfully imported audio files from folder", 
                                       juce::dontSendNotification);
                }
                else
                {
                    statusLabel.setText("No audio files found in selected folder", 
                                       juce::dontSendNotification);
                }
            });
        }
    });
}