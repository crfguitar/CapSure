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

void LibraryComponent::paintRowBackground(juce::Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected)
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
    // Ensure the table repaints to show selection
    table.repaint();
    
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

void LibraryComponent::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown())
    {
        // Find which row was clicked
        auto rowClicked = table.getRowContainingPosition(e.getMouseDownX(), e.getMouseDownY() - table.getY());
        if (juce::isPositiveAndBelow(rowClicked, currentRecordings.size()))
        {
            // Select the row first
            table.selectRow(rowClicked);
            showContextMenu(rowClicked, e);
        }
    }
    else
    {
        Component::mouseDown(e);
    }
}

void LibraryComponent::showContextMenu(int rowNumber, const juce::MouseEvent& /*e*/)
{
    if (!juce::isPositiveAndBelow(rowNumber, currentRecordings.size()))
        return;
        
    const auto& recording = currentRecordings.getReference(rowNumber);
    
    juce::PopupMenu menu;
    menu.addItem(1, "Edit Info...");
    menu.addItem(2, "Delete from Library");
    menu.addItem(3, "Export Audio...");
    menu.addSeparator();
    menu.addItem(4, "Copy Path");
    menu.addItem(5, "Show in Explorer");
    
    menu.showMenuAsync(juce::PopupMenu::Options{}, [this, rowNumber, recording](int result)
    {
        switch (result)
        {
            case 1: // Edit Info
                if (onRecordingEdit)
                    onRecordingEdit(recording);
                break;
                
            case 2: // Delete from Library
                if (onRecordingRemove)
                    onRecordingRemove(rowNumber);
                break;
                
            case 3: // Export Audio
                if (onRecordingExport)
                    onRecordingExport(recording);
                break;
                
            case 4: // Copy Path
                juce::SystemClipboard::copyTextToClipboard(recording.file.getFullPathName());
                break;
                
            case 5: // Show in Explorer
                recording.file.revealToUser();
                break;
        }
    });
}

//==============================================================================
// Metadata Editor Component Implementation
//==============================================================================

MetadataEditorComponent::MetadataEditorComponent(Recording recording) : originalRecording(recording)
{
    addAndMakeVisible(nameLabel);
    addAndMakeVisible(nameEditor);
    addAndMakeVisible(artistLabel);
    addAndMakeVisible(artistEditor);
    addAndMakeVisible(tagsLabel);
    addAndMakeVisible(tagsEditor);
    addAndMakeVisible(genreLabel);
    addAndMakeVisible(genreEditor);
    addAndMakeVisible(trackLabel);
    addAndMakeVisible(trackEditor);
    addAndMakeVisible(saveButton);
    addAndMakeVisible(cancelButton);
    
    // Setup labels
    nameLabel.setText("Name:", juce::dontSendNotification);
    artistLabel.setText("Artist:", juce::dontSendNotification);
    tagsLabel.setText("Tags:", juce::dontSendNotification);
    genreLabel.setText("Genre:", juce::dontSendNotification);
    trackLabel.setText("Track #:", juce::dontSendNotification);
    
    // Setup editors with current values
    nameEditor.setText(recording.name);
    artistEditor.setText(recording.artist.isEmpty() ? "" : recording.artist);
    tagsEditor.setText(recording.tags.joinIntoString(", "));
    genreEditor.setText(recording.genre.isEmpty() ? "" : recording.genre);
    trackEditor.setText(recording.trackNumber > 0 ? juce::String(recording.trackNumber) : "");
    
    // Setup buttons
    saveButton.setButtonText("Save");
    saveButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1db954));
    saveButton.onClick = [this]() { if (onSave) onSave(); };
    
    cancelButton.setButtonText("Cancel");
    cancelButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff666666));
    cancelButton.onClick = [this]() { if (onCancel) onCancel(); };
    
    setSize(400, 300);
}

void MetadataEditorComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));
    g.setColour(juce::Colour(0xff404040));
    g.drawRect(getLocalBounds(), 2);
}

void MetadataEditorComponent::resized()
{
    auto area = getLocalBounds().reduced(15);
    
    auto setupRow = [&area](juce::Label& label, juce::TextEditor& editor)
    {
        auto row = area.removeFromTop(35);
        label.setBounds(row.removeFromLeft(80));
        row.removeFromLeft(5);
        editor.setBounds(row);
        area.removeFromTop(5);
    };
    
    setupRow(nameLabel, nameEditor);
    setupRow(artistLabel, artistEditor);
    setupRow(genreLabel, genreEditor);
    setupRow(trackLabel, trackEditor);
    setupRow(tagsLabel, tagsEditor);
    
    area.removeFromTop(15);
    auto buttonArea = area.removeFromTop(30);
    
    cancelButton.setBounds(buttonArea.removeFromRight(80));
    buttonArea.removeFromRight(10);
    saveButton.setBounds(buttonArea.removeFromRight(80));
}

Recording MetadataEditorComponent::getEditedRecording() const
{
    Recording edited = originalRecording;
    edited.name = nameEditor.getText();
    edited.artist = artistEditor.getText();
    edited.genre = genreEditor.getText();
    edited.trackNumber = trackEditor.getText().getIntValue();
    
    // Parse tags
    edited.tags.clear();
    auto tagsText = tagsEditor.getText();
    if (!tagsText.isEmpty())
    {
        auto tagsList = juce::StringArray::fromTokens(tagsText, ",", "");
        for (auto& tag : tagsList)
            edited.tags.add(tag.trim());
    }
    
    return edited;
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
        
        if (file == juce::File() || !file.existsAsFile())
        {
            thumbnail.clear();
            repaint();
        }
        else
        {
            startTimer(100);
        }
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
        
        // Show file info at the bottom
        if (currentFile != juce::File())
        {
            g.setColour(juce::Colours::white.withAlpha(0.8f));
            g.setFont(juce::FontOptions(12.0f));
            auto infoText = currentFile.getFileNameWithoutExtension() + " (" + 
                           juce::String(thumbnail.getTotalLength(), 1) + "s)";
            g.drawText(infoText, area.removeFromBottom(20), juce::Justification::centredLeft);
        }
    }
    else if (needsToLoadFile)
    {
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.setFont(juce::FontOptions(16.0f));
        g.drawText("Loading audio file...", area, juce::Justification::centred);
        
        g.setColour(juce::Colour(0xff404040));
        g.drawRect(area, 1);
    }
    else if (currentFile != juce::File() && !currentFile.existsAsFile())
    {
        g.setColour(juce::Colours::red.withAlpha(0.6f));
        g.setFont(juce::FontOptions(16.0f));
        g.drawText("Audio file not found", area, juce::Justification::centred);
        
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
        auto* inputSource = new juce::FileInputSource(currentFile);
        thumbnail.setSource(inputSource);
        
        // The thumbnail will update automatically via the change listener
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
    
    libraryComponent->onRecordingRemove = [this](int index)
    {
        onRecordingRemove(index);
    };
    
    libraryComponent->onRecordingEdit = [this](const Recording& recording)
    {
        showMetadataEditor(recording);
    };
    
    libraryComponent->onRecordingExport = [this](const Recording& recording)
    {
        onRecordingExport(recording);
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
            // Ensure the file exists before attempting to load
            if (recording.file.existsAsFile())
            {
                waveformComponent->setAudioFile(recording.file);
                statusLabel.setText("Loaded: " + recording.name, juce::dontSendNotification);
            }
            else
            {
                waveformComponent->setAudioFile(juce::File());
                statusLabel.setText("File not found: " + recording.file.getFullPathName(), juce::dontSendNotification);
            }
        }
    }
    else
    {
        waveformComponent->setAudioFile(juce::File());
        statusLabel.setText("Ready to record internal audio or import existing files", juce::dontSendNotification);
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
                
                int beforeCount = libraryManager->getNumRecordings();
                libraryManager->importAudioFiles(files, false); // Don't copy to library by default
                int afterCount = libraryManager->getNumRecordings();
                int imported = afterCount - beforeCount;
                
                if (imported > 0)
                {
                    statusLabel.setText("Successfully imported " + juce::String(imported) + " audio files", 
                                       juce::dontSendNotification);
                }
                else
                {
                    statusLabel.setText("No new audio files imported (files may already exist or be invalid)", 
                                       juce::dontSendNotification);
                }
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
                statusLabel.setText("Scanning folder for audio files...", juce::dontSendNotification);
                
                int beforeCount = libraryManager->getNumRecordings();
                bool foundFiles = libraryManager->importFolder(folder, true, false); // Recursive, don't copy
                int afterCount = libraryManager->getNumRecordings();
                int imported = afterCount - beforeCount;
                
                if (imported > 0)
                {
                    statusLabel.setText("Successfully imported " + juce::String(imported) + " audio files from folder", 
                                       juce::dontSendNotification);
                }
                else if (foundFiles)
                {
                    statusLabel.setText("Found audio files but none were new (may already exist in library)", 
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

void MainComponent::onRecordingRemove(int index)
{
    auto recordings = libraryManager->getFilteredRecordings(libraryComponent->searchBox.getText());
    if (juce::isPositiveAndBelow(index, recordings.size()))
    {
        const auto& recording = recordings.getReference(index);
        
        // Show confirmation dialog
        juce::AlertWindow::showYesNoCancelBox(juce::AlertWindow::QuestionIcon,
                                             "Delete Recording",
                                             "Are you sure you want to remove \\\"" + recording.name + "\\\" from the library?\\n\\n" +
                                             "The audio file will remain on disk.",
                                             "Delete", "Cancel", {},
                                             this,
                                             juce::ModalCallbackFunction::create([this, recording](int result)
                                             {
                                                 if (result == 1) // Yes
                                                 {
                                                     // Find the actual index in the main recordings array and remove
                                                     auto allRecordings = libraryManager->getAllRecordings();
                                                     for (int i = 0; i < allRecordings.size(); ++i)
                                                     {
                                                         if (allRecordings[i].uid == recording.uid)
                                                         {
                                                             libraryManager->removeRecording(i);
                                                             statusLabel.setText("Removed \\\"" + recording.name + "\\\" from library", juce::dontSendNotification);
                                                             
                                                             // Clear waveform if this was the selected recording
                                                             if (selectedRecordingIndex == i)
                                                             {
                                                                 waveformComponent->setAudioFile(juce::File());
                                                                 selectedRecordingIndex = -1;
                                                             }
                                                             break;
                                                         }
                                                     }
                                                 }
                                             }));
    }
}

void MainComponent::showMetadataEditor(const Recording& recording)
{
    auto editorWindow = std::make_unique<juce::DocumentWindow>("Edit Recording Info", 
                                                              juce::Colour(0xff1e1e1e), 
                                                              juce::DocumentWindow::closeButton);
    
    auto editor = std::make_unique<MetadataEditorComponent>(recording);
    auto* editorPtr = editor.get();
    
    editor->onSave = [this, editorPtr, recording]() 
    {
        auto editedRecording = editorPtr->getEditedRecording();
        
        // Find and update the recording in the library
        auto allRecordings = libraryManager->getAllRecordings();
        for (int i = 0; i < allRecordings.size(); ++i)
        {
            if (allRecordings[i].uid == recording.uid)
            {
                libraryManager->updateRecording(i, editedRecording);
                statusLabel.setText("Updated info for \"" + editedRecording.name + "\"", juce::dontSendNotification);
                break;
            }
        }
        
        if (auto* window = editorPtr->findParentComponentOfClass<juce::DocumentWindow>())
            window->closeButtonPressed();
    };
    
    editor->onCancel = [editorPtr]()
    {
        if (auto* window = editorPtr->findParentComponentOfClass<juce::DocumentWindow>())
            window->closeButtonPressed();
    };
    
    editorWindow->setContentOwned(editor.release(), true);
    editorWindow->centreWithSize(420, 320);
    editorWindow->setVisible(true);
    editorWindow->toFront(true);
    
    // Keep the window alive (will be deleted when closed)
    editorWindow.release();
}

void MainComponent::onRecordingExport(const Recording& recording)
{
    auto chooserFlags = juce::FileBrowserComponent::saveMode;
    
    juce::FileChooser chooser("Export audio file",
                             juce::File::getSpecialLocation(juce::File::userDesktopDirectory),
                             "*" + recording.file.getFileExtension());
    
    chooser.launchAsync(chooserFlags, [this, recording](const juce::FileChooser& fc)
    {
        auto targetFile = fc.getResult();
        if (targetFile != juce::File{})
        {
            juce::MessageManager::callAsync([this, recording, targetFile]()
            {
                if (recording.file.copyFileTo(targetFile))
                {
                    statusLabel.setText("Exported \"" + recording.name + "\" to " + targetFile.getFullPathName(), 
                                       juce::dontSendNotification);
                }
                else
                {
                    statusLabel.setText("Failed to export \"" + recording.name + "\"", 
                                       juce::dontSendNotification);
                }
            });
        }
    });
}

void MainComponent::exportAudioFile(const Recording& recording)
{
    onRecordingExport(recording);
}