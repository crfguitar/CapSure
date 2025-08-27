#include "LoopbackCapture.h"
#include <cstring>

#ifdef _WIN32
#include <combaseapi.h>

bool LoopbackCapture::start(Callback cb)
{
    stop();
    callback = std::move(cb);
    if (!callback)
        return false;

    if (!initClient())
        return false;

    running = true;
    threadObj = std::thread([this]{ captureThread(); });
    return true;
}

void LoopbackCapture::stop()
{
    running = false;
    if (threadObj.joinable())
        threadObj.join();

#ifdef _WIN32
    if (audioClient) audioClient->Stop();
    if (mixFormat) { CoTaskMemFree(mixFormat); mixFormat = nullptr; }
    captureClient.Reset();
    audioClient.Reset();
    device.Reset();
#endif
}

bool LoopbackCapture::initClient()
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
        return false;

    Microsoft::WRL::ComPtr<IMMDeviceEnumerator> enumerator;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                IID_PPV_ARGS(&enumerator))))
        return false;

    if (FAILED(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device)))
        return false;

    if (FAILED(device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                                reinterpret_cast<void**>(audioClient.GetAddressOf()))))
        return false;

    if (FAILED(audioClient->GetMixFormat(&mixFormat)))
        return false;

    // Force float mix if possible (convert later if not float)
    // 100ms buffer
    const REFERENCE_TIME hnsBuffer = 1000000;
    DWORD streamFlags = AUDCLNT_STREAMFLAGS_LOOPBACK;

    hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                 streamFlags,
                                 hnsBuffer, 0, mixFormat, nullptr);
    if (FAILED(hr))
        return false;

    if (FAILED(audioClient->GetService(__uuidof(IAudioCaptureClient),
                                       reinterpret_cast<void**>(captureClient.GetAddressOf()))))
        return false;

    sampleRate = mixFormat->nSamplesPerSec;
    if (FAILED(audioClient->Start()))
        return false;
    return true;
}

static void convertToFloat(const BYTE* src, float* dst, int frames, int channels, const WAVEFORMATEX& fmt)
{
    if (fmt.wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
    {
        std::memcpy(dst, src, size_t(frames) * channels * sizeof(float));
        return;
    }
    // Assume PCM integer
    const int stride = channels;
    if (fmt.wBitsPerSample == 16)
    {
        auto* s16 = reinterpret_cast<const int16_t*>(src);
        for (int i = 0; i < frames * stride; ++i)
            dst[i] = s16[i] / 32768.0f;
    }
    else if (fmt.wBitsPerSample == 24)
    {
        for (int f = 0; f < frames; ++f)
            for (int c = 0; c < channels; ++c)
            {
                const BYTE* p = src + (f * stride + c) * 3;
                int32_t v = (int32_t(p[2]) << 24) | (int32_t(p[1]) << 16) | (int32_t(p[0]) << 8);
                v >>= 8;
                dst[f * stride + c] = v / 8388608.0f;
            }
    }
    else if (fmt.wBitsPerSample == 32)
    {
        auto* s32 = reinterpret_cast<const int32_t*>(src);
        for (int i = 0; i < frames * stride; ++i)
            dst[i] = s32[i] / 2147483648.0f;
    }
    else
    {
        // Unsupported depth – zero out
        std::fill(dst, dst + frames * stride, 0.0f);
    }
}

void LoopbackCapture::captureThread()
{
    const int maxChannels = (mixFormat ? mixFormat->nChannels : 2);
    std::vector<float> interleaved(4096 * maxChannels);
    std::vector<std::vector<float>> deinterleaved(maxChannels, std::vector<float>(4096));
    std::vector<const float*> chPtrs(maxChannels);

    while (running)
    {
        UINT32 packet = 0;
        if (FAILED(captureClient->GetNextPacketSize(&packet)))
        {
            running = false;
            break;
        }
        if (packet == 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }

        BYTE* data = nullptr;
        UINT32 numFrames = 0;
        DWORD flags = 0;
        if (FAILED(captureClient->GetBuffer(&data, &numFrames, &flags, nullptr, nullptr)))
        {
            running = false;
            break;
        }
        if (numFrames > 0)
        {
            const int channels = mixFormat->nChannels;
            if ((int)interleaved.size() < int(numFrames) * channels)
                interleaved.resize(size_t(numFrames) * channels);

            convertToFloat(data, interleaved.data(), (int)numFrames, channels, *mixFormat);

            for (int c = 0; c < channels; ++c)
            {
                auto& v = deinterleaved[c];
                if ((int)v.size() < (int)numFrames)
                    v.resize(numFrames);
                for (UINT32 f = 0; f < numFrames; ++f)
                    v[f] = interleaved[f * channels + c];
                chPtrs[c] = v.data();
            }

            if (callback)
                callback(chPtrs.data(), channels, (int)numFrames, sampleRate);
        }
        captureClient->ReleaseBuffer(numFrames);
    }

    callback = nullptr;
    CoUninitialize();
}

#else // !_WIN32

bool LoopbackCapture::start(Callback) { return false; }
void LoopbackCapture::stop() {}

#endif