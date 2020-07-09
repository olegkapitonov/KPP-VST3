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

#ifndef FAUST_SUPPORT_H
#define FAUST_SUPPORT_H

#include <map>
#include <string>

#include "kpp_tubeamp.h"

// Defines for compatability with
// FAUST generated code

#define VOLUME_CTRL ports.volume
#define DRIVE_CTRL ports.drive
#define MASTERGAIN_CTRL ports.mastergain

#define AMP_BIAS_CTRL profile->amp_bias
#define AMP_KREG_CTRL profile->amp_Kreg
#define AMP_UPOR_CTRL profile->amp_Upor

#define PREAMP_BIAS_CTRL profile->preamp_bias
#define PREAMP_KREG_CTRL profile->preamp_Kreg
#define PREAMP_UPOR_CTRL profile->preamp_Upor

#define LOW_CTRL ports.low
#define MIDDLE_CTRL ports.middle
#define HIGH_CTRL ports.high

#define LOW_FREQ_CTRL profile->tonestack_low_freq
#define MIDDLE_FREQ_CTRL profile->tonestack_middle_freq
#define HIGH_FREQ_CTRL profile->tonestack_high_freq

#define LOW_BAND_CTRL profile->tonestack_low_band
#define MIDDLE_BAND_CTRL profile->tonestack_middle_band
#define HIGH_BAND_CTRL profile->tonestack_high_band

#define PREAMP_LEVEL profile->preamp_level
#define AMP_LEVEL profile->amp_level

#define SAG_TIME profile->sag_time
#define SAG_COEFF profile->sag_coeff

#define OUTPUT_LEVEL profile->output_level

// Needed for compatability with FAUST generated code
struct Meta : std::map<const char*, const char*>
{
  void declare(const char *key, const char *value)
  {
    (*this)[key] = value;
  }
  const char* get(const char *key, const char *def)
  {
    if (this->find(key) != this->end())
      return (*this)[key];
    else
      return def;
  }
};


class UI {
public:
   UI(){};

  void openVerticalBox(const char * name) {};
  void closeBox() {};

};


class dsp {

    public:

        stPorts ports;
        st_profile_header *profile;

        dsp() {}
        virtual ~dsp() {}

        /* Return instance number of audio inputs */
        virtual int getNumInputs() = 0;

        /* Return instance number of audio outputs */
        virtual int getNumOutputs() = 0;

        /**
         * Trigger the ui_interface parameter with instance specific calls
         * to 'addBtton', 'addVerticalSlider'... in order to build the UI.
         *
         * @param ui_interface - the user interface builder
         */
        virtual void buildUserInterface(UI* ui_interface) = 0;

        /* Returns the sample rate currently used by the instance */
        virtual int getSampleRate() = 0;

        /**
         * Global init, calls the following methods:
         * - static class 'classInit': static tables initialization
         * - 'instanceInit': constants and instance state initialization
         *
         * @param samplingRate - the sampling rate in Hertz
         */
        virtual void init(int samplingRate) = 0;

        /**
         * Init instance state
         *
         * @param samplingRate - the sampling rate in Hertz
         */
        virtual void instanceInit(int samplingRate) = 0;

        /**
         * Init instance constant state
         *
         * @param samplingRate - the sampling rate in Hertz
         */
        virtual void instanceConstants(int samplingRate) = 0;

        /* Init default control parameters values */
        virtual void instanceResetUserInterface() = 0;

        /* Init instance state (delay lines...) */
        virtual void instanceClear() = 0;

        /**
         * Return a clone of the instance.
         *
         * @return a copy of the instance on success, otherwise a null pointer.
         */
        virtual dsp* clone() = 0;

        /**
         * Trigger the Meta* parameter with instance specific calls to 'declare' (key, value) metadata.
         *
         * @param m - the Meta* meta user
         */
        virtual void metadata(Meta* m) = 0;

        /**
         * DSP instance computation, to be called with successive in/out audio buffers.
         *
         * @param count - the number of frames to compute
         * @param inputs - the input audio buffers as an array of non-interleaved FAUSTFLOAT samples (eiher float, double or quad)
         * @param outputs - the output audio buffers as an array of non-interleaved FAUSTFLOAT samples (eiher float, double or quad)
         *
         */
        virtual void compute(int count, float** inputs, float** outputs) = 0;

        /**
         * DSP instance computation: alternative method to be used by subclasses.
         *
         * @param date_usec - the timestamp in microsec given by audio driver.
         * @param count - the number of frames to compute
         * @param inputs - the input audio buffers as an array of non-interleaved FAUSTFLOAT samples (eiher float, double or quad)
         * @param outputs - the output audio buffers as an array of non-interleaved FAUSTFLOAT samples (eiher float, double or quad)
         *
         */
        virtual void compute(double date_usec, int count, float** inputs, float** outputs) { compute(count, inputs, outputs); }

};

#endif
