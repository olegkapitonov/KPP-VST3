/*
 * Copyright (C) 2018-2020 Oleg Kapitonov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * --------------------------------------------------------------------------
 */

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

#include "../include/plugcontroller.h"
#include "../include/plugids.h"
#include "pluginterfaces/base/ustring.h"

#include "base/source/fstreamer.h"
#include "base/source/fstring.h"
#include "pluginterfaces/base/ibstream.h"

using namespace VSTGUI;

namespace Steinberg {
namespace Vst {
  class ToneParameter : public Parameter
  {
  public:
    ToneParameter (int32 flags, int32 id, const char *label);

    void toString (ParamValue normValue, String128 string) const SMTG_OVERRIDE;
    bool fromString (const TChar* string, ParamValue& normValue) const SMTG_OVERRIDE;
  };

  ToneParameter::ToneParameter (int32 flags, int32 id, const char *label)
  {
    UString (info.title, USTRINGSIZE (info.title)).assign (USTRING (label));
    UString (info.units, USTRINGSIZE (info.units)).assign (USTRING ("dB"));

    info.flags = flags;
    info.id = id;
    info.stepCount = 0;
    info.defaultNormalizedValue = 0.5f;
    info.unitId = kRootUnitId;

    setNormalized (1.f);
  }

  void ToneParameter::toString (ParamValue normValue, String128 string) const
  {
    char text[32];
    sprintf (text, "%.2f", (normValue * 2.0 - 1.0) * 15.0);

    Steinberg::UString (string, 128).fromAscii (text);
  }

  bool ToneParameter::fromString (const TChar* string, ParamValue& normValue) const
  {
    String wrapper ((TChar*)string); // don't know buffer size here!
    double tmp = 0.0;
    if (wrapper.scanFloat (tmp))
    {
      normValue = (tmp + 15.0) / 30.0;
      return true;
    }
    return false;
  }

  class GainParameter : public Parameter
  {
  public:
    GainParameter (int32 flags, int32 id);

    void toString (ParamValue normValue, String128 string) const SMTG_OVERRIDE;
    bool fromString (const TChar* string, ParamValue& normValue) const SMTG_OVERRIDE;
  };

  GainParameter::GainParameter (int32 flags, int32 id)
  {
    UString (info.title, USTRINGSIZE (info.title)).assign (USTRING ("Drive"));
    UString (info.units, USTRINGSIZE (info.units)).assign (USTRING ("dB"));

    info.flags = flags;
    info.id = id;
    info.stepCount = 0;
    info.defaultNormalizedValue = 0.5f;
    info.unitId = kRootUnitId;

    setNormalized (1.f);
  }

  void GainParameter::toString (ParamValue normValue, String128 string) const
  {
    char text[32];
    sprintf (text, "%.2f", normValue * 100.0);

    Steinberg::UString (string, 128).fromAscii (text);
  }

  bool GainParameter::fromString (const Vst::TChar* string, Vst::ParamValue& normValue) const
  {
    String wrapper ((Vst::TChar*)string); // don't know buffer size here!
    double tmp = 0.0;
    if (wrapper.scanFloat (tmp))
    {
      normValue = tmp / 100.0;
      return true;
    }
    return false;
  }

  tresult PLUGIN_API PlugController::initialize (FUnknown* context)
  {
    tresult result = EditControllerEx1::initialize (context);
    if (result == kResultTrue)
    {
      parameters.addParameter (STR16 ("Bypass"), nullptr, 1, 0,
                               ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass,
                               kBypassId);

      ToneParameter* bassParam = new ToneParameter (ParameterInfo::kCanAutomate, kBassId, "Bass");
      parameters.addParameter (bassParam);

      ToneParameter* middleParam = new ToneParameter (ParameterInfo::kCanAutomate, kMiddleId, "Middle");
      parameters.addParameter (middleParam);

      ToneParameter* trebleParam = new ToneParameter (ParameterInfo::kCanAutomate, kTrebleId, "Treble");
      parameters.addParameter (trebleParam);

      GainParameter* gainParam = new GainParameter (ParameterInfo::kCanAutomate, kGainId);
      parameters.addParameter (gainParam);

      parameters.addParameter (STR16 ("Volume"), NULL, 0, .5,
                               ParameterInfo::kCanAutomate, kVolumeId, 0,
                               STR16 ("Volume"));

      parameters.addParameter (STR16 ("Voice"), NULL, 0, .5,
                               ParameterInfo::kCanAutomate, kVoiceId, 0,
                               STR16 ("Voice"));
    }
    return kResultTrue;
  }

  IPlugView* PLUGIN_API PlugController::createView (const char* name)
  {
    if (name && strcmp (name, "editor") == 0)
    {
      auto* view = new VST3Editor (this, "view", "plug.uidesc");
      return view;
    }
    return nullptr;
  }

  tresult PLUGIN_API PlugController::setComponentState (IBStream* state)
  {
    if (!state)
      return kResultFalse;

    IBStreamer streamer (state, kLittleEndian);

    float savedBass = 0.f;
    if (streamer.readFloat (savedBass) == false)
      return kResultFalse;
    setParamNormalized (kBassId, savedBass);

    float savedMiddle = 0.f;
    if (streamer.readFloat (savedMiddle) == false)
      return kResultFalse;
    setParamNormalized (kMiddleId, savedMiddle);

    float savedTreble = 0.f;
    if (streamer.readFloat (savedTreble) == false)
      return kResultFalse;
    setParamNormalized (kTrebleId, savedTreble);

    float savedGain = 0.f;
    if (streamer.readFloat (savedGain) == false)
      return kResultFalse;
    setParamNormalized (kGainId, savedGain);

    float savedVolume = 0.f;
    if (streamer.readFloat (savedVolume) == false)
      return kResultFalse;
    setParamNormalized (kVolumeId, savedVolume);

    float savedVoice = 0.f;
    if (streamer.readFloat (savedVoice) == false)
      return kResultFalse;
    setParamNormalized (kVoiceId, savedVoice);

    int32 bypassState;
    if (streamer.readInt32 (bypassState) == false)
      return kResultFalse;
    setParamNormalized (kBypassId, bypassState ? 1 : 0);

    return kResultOk;
  }

  tresult PLUGIN_API PlugController::setParamNormalized (ParamID tag, ParamValue value)
  {
    tresult result = EditControllerEx1::setParamNormalized (tag, value);
    return result;
  }

  tresult PLUGIN_API PlugController::getParamStringByValue (ParamID tag, ParamValue valueNormalized,
                                                            String128 string)
  {
    return EditControllerEx1::getParamStringByValue (tag, valueNormalized, string);
  }

  tresult PLUGIN_API PlugController::getParamValueByString (ParamID tag, TChar* string,
                                                            ParamValue& valueNormalized)
  {
    return EditControllerEx1::getParamValueByString (tag, string, valueNormalized);
  }

} // Vst
} // Steinberg
