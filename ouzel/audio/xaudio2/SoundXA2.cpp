// Copyright (C) 2017 Elviss Strazdins
// This file is part of the Ouzel engine.

#include "SoundXA2.h"
#include "AudioXA2.h"
#include "SoundDataXA2.h"
#include "core/Engine.h"
#include "utils/Log.h"

namespace ouzel
{
    namespace audio
    {
        SoundXA2::SoundXA2()
        {
        }

        SoundXA2::~SoundXA2()
        {
            if (sourceVoice) sourceVoice->DestroyVoice();
        }

        bool SoundXA2::init(const SoundDataPtr& newSoundData)
        {
            if (!Sound::init(newSoundData))
            {
                return false;
            }

            AudioXA2* audioXA2 = static_cast<AudioXA2*>(sharedEngine->getAudio());
            std::shared_ptr<SoundDataXA2> soundDataXA2 = std::static_pointer_cast<SoundDataXA2>(soundData);

            sourceVoice = audioXA2->createSourceVoice(soundDataXA2->getWaveFormat());

            if (!sourceVoice)
            {
                return false;
            }

            ready = true;

            return true;
        }

        bool SoundXA2::play(bool repeatSound)
        {
            if (!Sound::play(repeatSound))
            {
                return false;
            }

            XAUDIO2_BUFFER bufferData;
            bufferData.Flags = XAUDIO2_END_OF_STREAM;
            bufferData.AudioBytes = static_cast<UINT32>(soundData->getData().size());
            bufferData.pAudioData = soundData->getData().data();
            bufferData.PlayBegin = 0;
            bufferData.PlayLength = 0;
            bufferData.LoopBegin = 0;
            bufferData.LoopLength = 0;
            bufferData.LoopCount = repeatSound ? XAUDIO2_LOOP_INFINITE : 0;
            bufferData.pContext = nullptr;

            if (FAILED(sourceVoice->SubmitSourceBuffer(&bufferData)))
            {
                Log(Log::Level::ERR) << "Failed to upload sound data";
                return false;
            }

            if (FAILED(sourceVoice->Start()))
            {
                Log(Log::Level::ERR) << "Failed to start consuming sound data";
                return false;
            }

            return true;
        }

        bool SoundXA2::stop(bool resetSound)
        {
            if (!Sound::stop(resetSound))
            {
                return false;
            }

            if (FAILED(sourceVoice->Stop()))
            {
                Log(Log::Level::ERR) << "Failed to stop sound buffer";
                return false;
            }

            return true;
        }

        bool SoundXA2::reset()
        {
            if (!Sound::reset())
            {
                return false;
            }

            if (FAILED(sourceVoice->Stop()))
            {
                Log(Log::Level::ERR) << "Failed to stop sound buffer";
                return false;
            }

            if (FAILED(sourceVoice->FlushSourceBuffers()))
            {
                Log(Log::Level::ERR) << "Failed to flush sound buffer";
                return false;
            }

            return true;
        }
    } // namespace audio
} // namespace ouzel
