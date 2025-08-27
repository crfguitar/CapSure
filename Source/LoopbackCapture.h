#pragma once
#ifdef _WIN32
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <wrl/client.h>
#endif
#include <atomic>
#include <thread>
#include <functional>
#include <vector>

/**
 * Simple WASAPI loopback capturer (system output) for Windows.
 * Calls the provided callback with de‑interleaved float channel pointers.
 *
 * Usage:
 *  LoopbackCapture cap;
 *  cap.start([](const float** ch, int nch, int nframes, double sr){ ... });
 *  ...
 *  cap.stop();
 */
class LoopbackCapture
{
public:
    using Callback = std::function<void(const float** data, int numChannels, int numFrames, double sampleRate)>;

    bool start(Callback cb);
    void stop();
    bool isRunning() const { return running.load(); }

private:
#ifdef _WIN32
    void captureThread();
    bool initClient();

    Microsoft::WRL::ComPtr<IMMDevice> device;
    Microsoft::WRL::ComPtr<IAudioClient> audioClient;
    Microsoft::WRL::ComPtr<IAudioCaptureClient> captureClient;
    WAVEFORMATEX* mixFormat = nullptr;
#endif

    std::thread threadObj;
    std::atomic<bool> running { false };
    Callback callback;
    double sampleRate = 0.0;
};