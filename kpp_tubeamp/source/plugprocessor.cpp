//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Examples
// Filename    : plugprocessor.cpp
// Created by  : Steinberg, 01/2018
// Description : HelloWorld Example for VST 3
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2019, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this
//     software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#include "../include/plugprocessor.h"
#include "../include/plugids.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

// Zita-convolver parameters
#define CONVPROC_SCHEDULER_PRIORITY 0
#define CONVPROC_SCHEDULER_CLASS SCHED_FIFO
#define THREAD_SYNC_MODE true

#define fragm 64

#define HAVE_STRUCT_TIMESPEC
#include "../thirdparty/zita-resampler/resampler.h"
#include "../thirdparty/zita-convolver/zita-convolver.h"

struct stProfile
{
  std::string path;
  st_profile_header header;
  Convproc preamp_convproc;
  Convproc convproc;
};


namespace Steinberg {
namespace Vst {

  PlugProcessor::PlugProcessor ()
  {
    setControllerClass (MyControllerUID);
  }

  tresult PLUGIN_API PlugProcessor::initialize (FUnknown* context)
  {
    tresult result = AudioEffect::initialize (context);
    if (result != kResultTrue)
      return kResultFalse;

    addAudioInput (STR16 ("AudioInput"), Vst::SpeakerArr::kStereo);
    addAudioOutput (STR16 ("AudioOutput"), Vst::SpeakerArr::kStereo);

    mDrive = 0.5;
    mBass = 0.5;
    mMiddle = 0.5;
    mTreble = 0.5;
    mVolume = 0.5;
    mLevel = 1.0;
    mCabinet = 1.0;
    mBypass = false;

    return kResultTrue;
  }

  tresult PLUGIN_API PlugProcessor::setBusArrangements (Vst::SpeakerArrangement* inputs,
                                                        int32 numIns,
                                                        Vst::SpeakerArrangement* outputs,
                                                        int32 numOuts)
  {
    if (numIns == 1 && numOuts == 1 && inputs[0] == outputs[0])
    {
      return AudioEffect::setBusArrangements (inputs, numIns, outputs, numOuts);
    }
    return kResultFalse;
  }

  tresult PLUGIN_API PlugProcessor::setupProcessing (Vst::ProcessSetup& setup)
  {
    sampleRate = setup.sampleRate;
    setBufsize(bufsize);
    return AudioEffect::setupProcessing (setup);
  }

  tresult PLUGIN_API PlugProcessor::setActive (TBool state)
  {
    if (state)
    {
      dsp = new TubeampDsp();
      dsp->init(sampleRate);

      dsp->ports.drive = mDrive * 100.0;
      dsp->ports.low = (mBass * 2.0 - 1.0) * 10.0;
      dsp->ports.middle = (mMiddle * 2.0 - 1.0) * 10.0;
      dsp->ports.high = (mTreble * 2.0 - 1.0) * 10.0;
      dsp->ports.mastergain = mVolume * 100.0;
      dsp->ports.volume = mLevel;
      dsp->ports.cabinet = mCabinet;

      if (profilePath != "")
      {
        if (check_profile_file(profilePath.c_str()))
        {
          profile = load_profile(profilePath.c_str());
          dsp->profile = &profile->header;
        }
      }
    }
    else
    {
      if (profile)
      {
        delete profile;
        profile = nullptr;
      }

      if (dsp)
      {
        delete dsp;
        dsp = nullptr;
      }
    }
    return AudioEffect::setActive (state);
  }

  tresult PLUGIN_API PlugProcessor::process (ProcessData& data)
  {
    if (data.inputParameterChanges)
    {
      int32 numParamsChanged = data.inputParameterChanges->getParameterCount ();
      for (int32 index = 0; index < numParamsChanged; index++)
      {
        IParamValueQueue* paramQueue =
        data.inputParameterChanges->getParameterData (index);
        if (paramQueue)
        {
          ParamValue value;
          int32 sampleOffset;
          int32 numPoints = paramQueue->getPointCount ();
          switch (paramQueue->getParameterId ())
          {
            case kDriveId:
              if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) ==
                kResultTrue)
              {
                mDrive = value;
                dsp->ports.drive = value * 100.0;
              }
              break;
            case kBassId:
              if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) ==
                kResultTrue)
              {
                mBass = value;
                dsp->ports.low = (value * 2.0 - 1.0) * 10.0;
              }
              break;
            case kMiddleId:
              if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) ==
                kResultTrue)
              {
                mMiddle = value;
                dsp->ports.middle = (value * 2.0 - 1.0) * 10.0;
              }
              break;
            case kTrebleId:
              if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) ==
                kResultTrue)
              {
                mTreble = value;
                dsp->ports.high = (value * 2.0 - 1.0) * 10.0;
              }
              break;
            case kVolumeId:
              if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) ==
                kResultTrue)
              {
                mVolume = value;
                dsp->ports.mastergain = value * 100.0;
              }
              break;
            case kLevelId:
              if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) ==
                kResultTrue)
              {
                mLevel = value;
                dsp->ports.volume = value;
              }
              break;
            case kCabinetId:
              if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) ==
                kResultTrue)
              {
                mCabinet = value;
                dsp->ports.cabinet = value;
              }
              break;
            case kBypassId:
              if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) ==
                kResultTrue)
                mBypass = (value > 0.5f);
              break;
          }
        }
      }
    }

    if (data.numInputs == 0 || data.numOutputs == 0)
    {
      return kResultOk;
    }

    SpeakerArrangement arr;
    getBusArrangement(kOutput, 0, arr);
    int32 numChannels = SpeakerArr::getChannelCount(arr);

    if ((data.numSamples > 0) && (profile))
    {
      float* inputs[2];
      float* outputs[2];

      if (numChannels >= 2)
      {
        inputs[0] = data.inputs[0].channelBuffers32[0];
        inputs[1] = data.inputs[0].channelBuffers32[1];

        outputs[0] = data.outputs[0].channelBuffers32[0];
        outputs[1] = data.outputs[0].channelBuffers32[1];
      }
      else
      {
        inputs[0] = data.inputs[0].channelBuffers32[0];
        inputs[1] = data.inputs[0].channelBuffers32[0];

        outputs[0] = data.outputs[0].channelBuffers32[0];
        outputs[1] = data.outputs[0].channelBuffers32[0];
      }

      if (!mBypass)
      {
        for (int i = 0; i < data.numSamples; i++)
        {
          preamp_inp_buf[i] = (inputs[0][i] + inputs[1][i]) / 2.0;
        }

        int bufp = 0;

        while (bufp < data.numSamples)
        {
          memcpy(profile->preamp_convproc.inpdata(0),
            preamp_inp_buf.data() + bufp,
            fragm * sizeof(float));
          profile->preamp_convproc.process(true);
          memcpy(preamp_outp_buf.data() + bufp,
            profile->preamp_convproc.outdata(0),
            fragm * sizeof(float));
          bufp += fragm;
        }

        for (int i = 0; i < data.numSamples; i++)
        {
          inputs[0][i] = preamp_outp_buf[i];
          inputs[1][i] = preamp_outp_buf[i];
        }

        dsp->compute(data.numSamples, inputs, outputs);
        bufp = 0;

        memcpy(drybuf_l.data(), outputs[0], data.numSamples * sizeof(float));
        memcpy(drybuf_r.data(), outputs[1], data.numSamples * sizeof(float));

        while (bufp < data.numSamples)
        {
          memcpy(profile->convproc.inpdata(0), outputs[0] + bufp, fragm * sizeof(float));
          memcpy(profile->convproc.inpdata(1), outputs[1] + bufp, fragm * sizeof(float));

          profile->convproc.process(true);
          memcpy(outputs[0] + bufp, profile->convproc.outdata(0), fragm * sizeof(float));
          memcpy(outputs[1] + bufp, profile->convproc.outdata(1), fragm * sizeof(float));

          bufp += fragm;
        }

        for (int i = 0; i < data.numSamples; i++)
        {
          outputs[0][i] = outputs[0][i] * (dsp->ports.cabinet) + drybuf_l[i] * (1.0 - dsp->ports.cabinet);
          outputs[1][i] = outputs[1][i] * (dsp->ports.cabinet) + drybuf_r[i] * (1.0 - dsp->ports.cabinet);
        }
      }
      else
      {
        for (int i = 0; i < data.numSamples; i++)
        {
          outputs[0][i] = inputs[0][i];
          outputs[1][i] = inputs[1][i];
        }
      }
    }
    else
    {
      for (int i = 0; i < numChannels; i++)
      {
        memset(data.outputs[0].channelBuffers32[i], 0, data.numSamples * sizeof(float));
      }
    }


    return kResultOk;
  }

  tresult PLUGIN_API PlugProcessor::setState (IBStream* state)
  {
    if (!state)
      return kResultFalse;

    IBStreamer streamer (state, kLittleEndian);

    float savedDrive = 0.f;
    if (streamer.readFloat (savedDrive) == false)
      return kResultFalse;

    float savedBass = 0.f;
    if (streamer.readFloat (savedBass) == false)
      return kResultFalse;

    float savedMiddle = 0.f;
    if (streamer.readFloat (savedMiddle) == false)
      return kResultFalse;

    float savedTreble = 0.f;
    if (streamer.readFloat (savedTreble) == false)
      return kResultFalse;

    float savedVolume = 0.f;
    if (streamer.readFloat (savedVolume) == false)
      return kResultFalse;

    float savedLevel = 0.f;
    if (streamer.readFloat (savedLevel) == false)
      return kResultFalse;

    float savedCabinet = 0.f;
    if (streamer.readFloat (savedCabinet) == false)
      return kResultFalse;

    int32 savedBypass = 0;
    if (streamer.readInt32(savedBypass) == false)
      return kResultFalse;

    profilePath = streamer.readStr8();

    mDrive = savedDrive;
    mBass = savedBass;
    mMiddle = savedMiddle;
    mTreble = savedTreble;
    mVolume = savedVolume;
    mLevel = savedLevel;
    mCabinet = savedCabinet;
    mBypass = savedBypass > 0;

    dsp->ports.drive = mDrive * 100.0;
    dsp->ports.low = (mBass * 2.0 - 1.0) * 10.0;
    dsp->ports.middle = (mMiddle * 2.0 - 1.0) * 10.0;
    dsp->ports.high = (mTreble * 2.0 - 1.0) * 10.0;
    dsp->ports.mastergain = mVolume * 100.0;
    dsp->ports.volume = mLevel;
    dsp->ports.cabinet = mCabinet;

    return kResultOk;
  }

  tresult PLUGIN_API PlugProcessor::getState (IBStream* state)
  {
    float toSaveDrive = mDrive;
    float toSaveBass = mBass;
    float toSaveMiddle = mMiddle;
    float toSaveTreble = mTreble;
    float toSaveVolume = mVolume;
    float toSaveLevel = mLevel;
    float toSaveCabinet = mCabinet;
    int32 toSaveBypass = mBypass ? 1 : 0;

    IBStreamer streamer (state, kLittleEndian);
    streamer.writeFloat (toSaveDrive);
    streamer.writeFloat (toSaveBass);
    streamer.writeFloat (toSaveMiddle);
    streamer.writeFloat (toSaveTreble);
    streamer.writeFloat (toSaveVolume);
    streamer.writeFloat (toSaveLevel);
    streamer.writeFloat (toSaveCabinet);
    streamer.writeInt32 (toSaveBypass);

    if (profile)
    {
      streamer.writeStr8(profile->path.c_str());
    }
    else
    {
      streamer.writeStr8("");
    }

    return kResultOk;
  }

  tresult PlugProcessor::receiveText (const char* text)
  {
    if (text)
    {
      if (check_profile_file(text))
      {
        stProfile *oldProfile = profile;
        profile = load_profile(text);
        profilePath = text;
        dsp->profile = &profile->header;
        if (oldProfile)
        {
          delete oldProfile;
        }
      }
    }
    return kResultOk;
  }

  bool PlugProcessor::check_profile_file(const char *path)
  {
    bool status = false;

    FILE * profile_file = fopen(path, "rb");

    if (profile_file != NULL)
    {
      st_profile_header check_profile;
      if (fread(&check_profile, sizeof(st_profile_header), 1, profile_file) == 1)
      {
        if (!strncmp(check_profile.signature, "TaPf", 4))
        {
          status = true;
        }
      }
      else status = false;

      fclose(profile_file);
    }

    return status;
  }

  // Function loads profile from file at 'path'
  // and creates new convolvers
  // with IR data from that *.tapf file
  stProfile* PlugProcessor::load_profile(const char *path)
  {

    FILE *profile_file = fopen(path, "rb");
    if (profile_file != NULL)
    {
      stProfile *p_profile = new stProfile();

      if (fread(&p_profile->header, sizeof(st_profile_header), 1, profile_file) == 1)
      {

        // IRs in *.tapf are 48000 Hz,
        // calculate ratio for resampling
        float ratio = (float)sampleRate / 48000.0;

        st_impulse_header preamp_impheader, impheader;

        // Load preamp IR data to temp buffer
        if (fread(&preamp_impheader, sizeof(st_impulse_header), 1, profile_file) != 1)
        {
          return NULL;
        }
        std::vector<float> preamp_impulse(preamp_impheader.sample_count);
        if (fread(preamp_impulse.data(), sizeof(float),
          preamp_impheader.sample_count,
          profile_file) != (size_t)preamp_impheader.sample_count)
        {
          return NULL;
        }

        std::vector<float> left_impulse;
        std::vector<float> right_impulse;
        // Load cabsym IR data to temp buffers
        for (int i=0;i<2;i++)
        {
          if (fread(&impheader, sizeof(st_impulse_header), 1, profile_file) != 1)
          {
            return NULL;
          }

          if (impheader.channel==0)
          {
            left_impulse.resize(impheader.sample_count);
            if (fread(left_impulse.data(), sizeof(float),
              impheader.sample_count, profile_file) != (size_t)impheader.sample_count)
            {
              return NULL;
            }
          }
          if (impheader.channel==1)
          {
            right_impulse.resize(impheader.sample_count);
            if (fread(right_impulse.data(), sizeof(float),
              impheader.sample_count, profile_file) != (size_t)impheader.sample_count)
            {
              return NULL;
            }
          }
        }

        // If current rate is not 48000 Hz do resampling
        // with Zita-resampler
        if (sampleRate!=48000)
        {
          {
            Resampler resampl;
            resampl.setup(48000,sampleRate,1,48);

            int k = resampl.inpsize();

            std::vector<float> preamp_in(preamp_impheader.sample_count + k/2 - 1 + k - 1);
            std::vector<float> preamp_out((unsigned int)((preamp_impheader.sample_count + k/2 - 1 + k - 1)*ratio));

            // Create paddig before and after signal, needed for zita-resampler
            for (int i = 0; i < preamp_impheader.sample_count + k/2 - 1 + k - 1; i++)
            {
              preamp_in[i] = 0.0;
            }

            for (int i = k/2 - 1; i < preamp_impheader.sample_count + k/2 - 1; i++)
            {
              preamp_in[i] = preamp_impulse[i - k/2 + 1];
            }

            resampl.inp_count = preamp_impheader.sample_count + k/2 - 1 + k - 1;
            resampl.out_count = (unsigned int)((preamp_impheader.sample_count + k/2 - 1 + k - 1)*ratio);
            resampl.inp_data = preamp_in.data();
            resampl.out_data = preamp_out.data();

            resampl.process();

            preamp_impulse.resize(preamp_impheader.sample_count * ratio);
            for (unsigned int i = 0; i < (unsigned int)(preamp_impheader.sample_count*ratio); i++)
            {
              preamp_impulse[i] = preamp_out[i] / ratio;
            }
          }

          {
            Resampler resampl;
            resampl.setup(48000,sampleRate,2,48);

            int k = resampl.inpsize();

            std::vector<float> inp_data((impheader.sample_count + k/2 - 1 + k - 1)*2);

            // Create paddig before and after signal, needed for zita-resampler
            for (int i = 0; i < impheader.sample_count + k/2 - 1 + k - 1; i++)
            {
              inp_data[i*2] = 0.0;
              inp_data[i*2+1] = 0.0;
            }

            for (int i = k/2 - 1; i < impheader.sample_count + k/2 - 1; i++)
            {
              inp_data[i*2] = left_impulse[i-k/2+1];
              inp_data[i*2+1] = right_impulse[i-k/2+1];
            }

            std::vector<float> out_data((unsigned int)((impheader.sample_count + k/2 - 1 + k - 1)*ratio*2));

            resampl.inp_count = impheader.sample_count + k/2 - 1 + k - 1;
            resampl.out_count = (unsigned int)((impheader.sample_count + k/2 - 1 + k - 1)*ratio);
            resampl.inp_data = inp_data.data();
            resampl.out_data = out_data.data();

            resampl.process();

            left_impulse.resize((unsigned int)(impheader.sample_count * ratio));
            right_impulse.resize((unsigned int)(impheader.sample_count * ratio));

            for (unsigned int i = 0; i < (unsigned int)(impheader.sample_count*ratio); i++)
            {
              left_impulse[i] = out_data[i*2] / ratio;
              right_impulse[i] = out_data[i*2+1] / ratio;
            }
          }

        }

        // Create preamp convolver
        Convproc *p_preamp_convproc = &p_profile->preamp_convproc;
        p_preamp_convproc->configure (1, 1, (unsigned int)(preamp_impheader.sample_count*ratio),
                                      fragm, fragm, Convproc::MAXPART, 0.0);
        p_preamp_convproc->impdata_create (0, 0, 1, preamp_impulse.data(),
                                           0, (unsigned int)(preamp_impheader.sample_count*ratio));

        p_preamp_convproc->start_process(CONVPROC_SCHEDULER_PRIORITY,
                                         CONVPROC_SCHEDULER_CLASS);

        // Create cabsym convolver
        Convproc *p_convproc = &p_profile->convproc;
        p_convproc->configure (2, 2, 48000/2, fragm, fragm, Convproc::MAXPART, 0.0);

        p_convproc->impdata_create (0, 0, 1, left_impulse.data(), 0, 48000/2);
        p_convproc->impdata_create (1, 1, 1, right_impulse.data(), 0, 48000/2);

        p_convproc->start_process (CONVPROC_SCHEDULER_PRIORITY, CONVPROC_SCHEDULER_CLASS);

        fclose(profile_file);

        p_profile->path = path;
        return p_profile;
      }
    }
    return nullptr;
  }

  void PlugProcessor::setBufsize(int size)
  {
    drybuf_l.resize(size);
    drybuf_r.resize(size);
    preamp_inp_buf.resize(size);
    preamp_outp_buf.resize(size);
  }

} // Vst
} // Steinberg
