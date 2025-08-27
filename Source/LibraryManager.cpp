#include "LibraryManager.h"

LibraryManager::LibraryManager()
{
    formatManager.registerBasicFormats();
    ensureDirectoriesExist();
    loadLibrary();
}

LibraryManager::~LibraryManager()
{
    saveLibrary();
}

void LibraryManager::ensureDirectoriesExist()
{
    auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                          .getChildFile("CapSure");
    
    recordingsDirectory = appDataDir.getChildFile("Recordings");
    libraryFile = appDataDir.getChildFile("library.xml");
    
    appDataDir.createDirectory();
    recordingsDirectory.createDirectory();
}

void LibraryManager::addRecording(const Recording& recording)
{
    recordings.add(recording);
    saveLibrary();
    sendChangeMessage();
}

void LibraryManager::removeRecording(int index)
{
    if (juce::isPositiveAndBelow(index, recordings.size()))
    {
        auto recording = recordings[index];
        recordings.remove(index);
        
        // Optionally delete the file
        if (recording.file.existsAsFile())
        {
            recording.file.deleteFile();
        }
        
        saveLibrary();
        sendChangeMessage();
    }
}

void LibraryManager::updateRecording(int index, const Recording& recording)
{
    if (juce::isPositiveAndBelow(index, recordings.size()))
    {
        recordings.set(index, recording);
        saveLibrary();
        sendChangeMessage();
    }
}

const Recording* LibraryManager::getRecording(int index) const
{
    if (juce::isPositiveAndBelow(index, recordings.size()))
        return &recordings.getReference(index);
    return nullptr;
}

juce::Array<Recording> LibraryManager::getFilteredRecordings(const juce::String& filter) const
{
    if (filter.isEmpty())
        return recordings;
    
    juce::Array<Recording> filtered;
    auto filterLower = filter.toLowerCase();
    
    for (const auto& recording : recordings)
    {
        if (recording.name.toLowerCase().contains(filterLower) ||
            recording.uid.toLowerCase().contains(filterLower))
        {
            filtered.add(recording);
            continue;
        }
        
        // Check tags
        for (const auto& tag : recording.tags)
        {
            if (tag.toLowerCase().contains(filterLower))
            {
                filtered.add(recording);
                break;
            }
        }
    }
    
    return filtered;
}

juce::Array<int> LibraryManager::findRecordingsByTag(const juce::String& tag) const
{
    juce::Array<int> indices;
    auto tagLower = tag.toLowerCase();
    
    for (int i = 0; i < recordings.size(); ++i)
    {
        for (const auto& recordingTag : recordings.getReference(i).tags)
        {
            if (recordingTag.toLowerCase() == tagLower)
            {
                indices.add(i);
                break;
            }
        }
    }
    
    return indices;
}

juce::Array<int> LibraryManager::searchRecordings(const juce::String& searchTerm) const
{
    juce::Array<int> indices;
    auto searchLower = searchTerm.toLowerCase();
    
    for (int i = 0; i < recordings.size(); ++i)
    {
        const auto& recording = recordings.getReference(i);
        
        if (recording.name.toLowerCase().contains(searchLower) ||
            recording.uid.toLowerCase().contains(searchLower))
        {
            indices.add(i);
            continue;
        }
        
        // Check tags
        for (const auto& tag : recording.tags)
        {
            if (tag.toLowerCase().contains(searchLower))
            {
                indices.add(i);
                break;
            }
        }
    }
    
    return indices;
}

void LibraryManager::saveLibrary()
{
    juce::ValueTree libraryTree("LIBRARY");
    libraryTree.setProperty("version", "1.0", nullptr);
    libraryTree.setProperty("created", juce::Time::getCurrentTime().toISO8601(false), nullptr);
    
    for (const auto& recording : recordings)
    {
        libraryTree.addChild(recording.toValueTree(), -1, nullptr);
    }
    
    auto xml = libraryTree.createXml();
    if (xml)
    {
        xml->writeTo(libraryFile);
    }
}

void LibraryManager::loadLibrary()
{
    if (!libraryFile.existsAsFile())
        return;
    
    auto xml = juce::XmlDocument::parse(libraryFile);
    if (!xml)
        return;
    
    auto libraryTree = juce::ValueTree::fromXml(*xml);
    if (!libraryTree.hasType("LIBRARY"))
        return;
    
    recordings.clear();
    
    for (auto recordingTree : libraryTree)
    {
        if (recordingTree.hasType("RECORDING"))
        {
            auto recording = Recording::fromValueTree(recordingTree);
            // Only add if the file still exists
            if (recording.file.existsAsFile())
            {
                recordings.add(recording);
            }
        }
    }
    
    sendChangeMessage();
}

// Import functionality implementations
bool LibraryManager::importAudioFile(const juce::File& file, bool copyToLibrary)
{
    if (!file.existsAsFile() || !isAudioFile(file))
        return false;
    
    // Check if already imported
    for (const auto& recording : recordings)
    {
        if (recording.file == file || recording.file.getFileName() == file.getFileName())
            return false; // Already exists
    }
    
    juce::File targetFile = file;
    
    if (copyToLibrary)
    {
        // Copy file to recordings directory
        targetFile = recordingsDirectory.getChildFile(file.getFileName());
        if (!file.copyFileTo(targetFile))
            return false;
    }
    
    auto recording = createRecordingFromFile(targetFile);
    if (recording.durationInSeconds > 0)
    {
        addRecording(recording);
        return true;
    }
    
    return false;
}

void LibraryManager::importAudioFiles(const juce::Array<juce::File>& files, bool copyToLibrary)
{
    int successCount = 0;
    for (const auto& file : files)
    {
        if (importAudioFile(file, copyToLibrary))
            successCount++;
    }
    
    if (successCount > 0)
    {
        saveLibrary();
        sendChangeMessage();
    }
}

bool LibraryManager::importFolder(const juce::File& folder, bool recursive, bool copyToLibrary)
{
    if (!folder.isDirectory())
        return false;
    
    juce::Array<juce::File> audioFiles;
    
    auto collectAudioFiles = [this, &audioFiles](const juce::File& dir, bool recurse)
    {
        juce::DirectoryIterator iter(dir, recurse, "*", juce::File::findFiles);
        while (iter.next())
        {
            auto file = iter.getFile();
            if (isAudioFile(file))
                audioFiles.add(file);
        }
    };
    
    collectAudioFiles(folder, recursive);
    
    if (audioFiles.size() > 0)
    {
        importAudioFiles(audioFiles, copyToLibrary);
        return true;
    }
    
    return false;
}

Recording LibraryManager::createRecordingFromFile(const juce::File& file)
{
    Recording recording;
    recording.uid = Recording::generateUID();
    recording.file = file;
    recording.timestamp = juce::Time(file.getLastModificationTime());
    
    // Extract name from filename (remove extension)
    recording.name = file.getFileNameWithoutExtension();
    
    // Try to get audio file info
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader)
    {
        recording.durationInSeconds = (double)reader->lengthInSamples / reader->sampleRate;
        recording.sampleRate = reader->sampleRate;
        recording.numChannels = (int)reader->numChannels;
    }
    
    // Extract tags from filename/path
    recording.tags = extractTagsFromFilename(file.getFileNameWithoutExtension());
    recording.tags.add("Imported");
    
    // Add genre/type tags based on file location or name
    auto pathString = file.getFullPathName().toLowerCase();
    if (pathString.contains("music") || pathString.contains("songs"))
        recording.tags.add("Music");
    if (pathString.contains("podcast"))
        recording.tags.add("Podcast");
    if (pathString.contains("meeting") || pathString.contains("call"))
        recording.tags.add("Meeting");
    
    return recording;
}

bool LibraryManager::isAudioFile(const juce::File& file)
{
    auto extension = file.getFileExtension().toLowerCase();
    return extension == ".wav" || 
           extension == ".mp3" || 
           extension == ".flac" || 
           extension == ".aiff" || 
           extension == ".m4a" ||
           extension == ".ogg";
}

juce::StringArray LibraryManager::extractTagsFromFilename(const juce::String& filename)
{
    juce::StringArray tags;
    auto nameLower = filename.toLowerCase();
    
    // Common patterns in filenames
    if (nameLower.contains("mix") || nameLower.contains("remix"))
        tags.add("Mix");
    if (nameLower.contains("live") || nameLower.contains("concert"))
        tags.add("Live");
    if (nameLower.contains("demo") || nameLower.contains("rough"))
        tags.add("Demo");
    if (nameLower.contains("master") || nameLower.contains("final"))
        tags.add("Master");
    if (nameLower.contains("loop"))
        tags.add("Loop");
    if (nameLower.contains("sample"))
        tags.add("Sample");
    
    if (tags.isEmpty())
        tags.add("Audio");
    
    return tags;
}