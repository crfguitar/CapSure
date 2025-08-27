# CapSure

CapSure is a professional desktop application for Windows, macOS, and Linux, designed for high-quality internal audio recording and library management. It's an ideal tool for musicians, content creators, and machine learning engineers who need to capture and organize audio from their system.

The application features a modern, dark-themed interface inspired by professional audio software, ensuring a focused and pleasant user experience.

## Key Features

- **Internal Audio Recording:** Capture audio directly from your system's output (loopback recording) without the need for physical cables or complex routing.
- **Comprehensive Audio Library:** All your recordings and imported audio files are managed in a centralized library.
- **Flexible File Import:** Import individual audio files (`.wav`, `.mp3`, `.flac`, etc.) or entire folders with a single click.
- **Rich Metadata:** Each library item includes information like duration, timestamp, sample rate, and channels.
- **Waveform Visualization:** See a visual representation of your audio files. The waveform display helps you quickly identify and navigate through your recordings.
- **Powerful Search:** Instantly find any audio file in your library with the built-in search functionality.
- **Persistent Library:** Your library is automatically saved and loaded between sessions, so your work is always preserved.
- **Cross-Platform:** Built with the JUCE framework, CapSure is designed to run on Windows, macOS, and Linux.

## Getting Started

To build and run CapSure from the source code, you will need:

- A modern C++ compiler (e.g., MSVC on Windows, Clang on macOS, GCC on Linux).
- The JUCE framework.

The project is configured using the `CapSure_Fresh.jucer` file. You can open this file with the Projucer to generate project files for your preferred IDE:

1.  **Open `CapSure_Fresh.jucer` in the Projucer.**
2.  **Select your exporter** (e.g., "Visual Studio 2022", "Xcode", "Linux Makefile").
3.  **Save the project** to generate the IDE-specific project files.
4.  **Open the generated project** in your IDE (e.g., the `.sln` file for Visual Studio).
5.  **Build and run** the application.

## How to Use

### Recording Audio

1.  Click the **"Record Internal Audio"** button to start capturing your system's audio.
2.  The button will turn red and change to **"Stop Recording"**.
3.  Click the button again to stop. The recording will be automatically saved to your library and will appear in the library view.

### Managing the Library

- **Importing Files:**
    - Click **"Import Audio Files"** to select and add one or more audio files to your library.
    - Click **"Import Folder"** to add all the audio files from a specific folder.
- **Searching:** Use the search box at the top of the library to filter the list of recordings by name.
- **Selecting a File:** Click on any item in the library list to select it. This will display its waveform in the panel on the right.

## Future Development

This project is under active development. Future plans include:

- **UI Enhancements:** Further refinements to the user interface for a more polished look and feel.
- **Tagging System:** A dedicated system for adding, editing, and filtering by tags.
- **Metadata Editor:** An interface to edit the metadata of recordings and imported files.
- **Advanced Export Options:** The ability to export selected files or the entire library in various formats.
- **Performance Optimizations:** Continued work to ensure the application is fast and responsive, even with very large libraries.
