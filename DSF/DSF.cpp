#include "daisy_patch_sm.h"
#include "daisysp.h"
#include <math.h>

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

/**
    CV_1 < Spread
    CV_2 < Resonance
    CV_3 < Cutoff
    CV_4 < High Pass

    CV_5 < Frequency Mod 1
    CV_6 < Frequency Mod 2
    CV_7 < Resonance
    CV_8 < Spread
*/

const float CUTOFF_MIN       = 20.0f;
const float CUTOFF_MAX       = 20000.0f;
const float CUTOFF_RANGE     = 20000.0f;
const float RESONANCE_MAX    = 1.0f;
const float BASE_1           = 4.0f;
const float BASE_2           = 1.7f;
const float VOLUME_NORMAL    = 0.5f;
const int   FILTER_STATES    = 5;

DaisyPatchSM        patch;
Switch              button;
Led                 led;
Svf                 svfL1, svfL2, svfR1, svfR2, highPassL, highPassR;

float   cutoff, fmKnob, fmCV1, fmCV2, cutoffSquared, 
        resonance, rmKnob, rmCV, 
        spread, spreadKnob, spreadCV,
        imageL1, imageL2, imageR1, imageR2, exponent, 
        hpCutoff, samplerate;
int     buttonValue;
bool    buttonState;

float RestrictRange(float input, float min, float max) {
    if (input > max) {
        return max;
    } else if (input < min) {
        return min;
    } else {
        return input;
    }
} // END RESTRICT RANGE METHOD


// MAIN AUDIO PROCESSING METHOD
void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size) {
    
    // UPDATE CV INPUTS
    patch.ProcessAllControls();

    // Debounce button
    button.Debounce();

    // Process Button
    // Need to get rising edge for a single input
    if (button.RisingEdge()) {
        buttonValue++;
    }

    buttonValue = buttonValue % FILTER_STATES;

    // GET ADC VALUES
    fmKnob          = patch.GetAdcValue(CV_3);
    fmCV1           = patch.GetAdcValue(CV_5);
    fmCV2           = patch.GetAdcValue(CV_6);

    rmKnob          = patch.GetAdcValue(CV_2);
    rmCV            = patch.GetAdcValue(CV_7);

    spreadKnob      = patch.GetAdcValue(CV_1);
    spreadCV        = patch.GetAdcValue(CV_8);

    hpCutoff        = patch.GetAdcValue(CV_4);

    // COMBINE PARAMETERS
    cutoff          = RestrictRange(fmKnob + fmCV1 + fmCV2, 0.0f, 1.0f);
    resonance       = RestrictRange(rmKnob + rmCV, 0.0f, 1.0f);
    spread          = RestrictRange(spreadKnob + spreadCV, 0.0f, 1.0f);

    // CALCULATE SPREAD MAP
    // NORMALIZE SPREAD TO [-1, 1]
    spread = (2.0f * spread) - 1.0f;
    cutoffSquared = cutoff * cutoff;

    // WRITE CUTOFF VALUE TO LED
    led.Set(cutoff);

    // COMPUTE IMAGES
    imageL1 = RestrictRange(pow(BASE_1, -spread) * cutoffSquared, 0.0f, 1.0f) * CUTOFF_RANGE;
    imageL2 = RestrictRange(pow(BASE_2, spread) * cutoffSquared, 0.0f, 1.0f) * CUTOFF_RANGE;
    imageR1 = RestrictRange(pow(BASE_1, spread) * cutoffSquared, 0.0f, 1.0f) * CUTOFF_RANGE;
    imageR2 = RestrictRange(pow(BASE_2, -spread) * cutoffSquared, 0.0f, 1.0f) * CUTOFF_RANGE;

    hpCutoff = hpCutoff * hpCutoff * CUTOFF_RANGE;

    // SET FILTER VALUES
    svfL1.SetFreq(imageL1);
    svfL2.SetFreq(imageL2);
    svfR1.SetFreq(imageR1);
    svfR2.SetFreq(imageR2);
    
    svfL1.SetRes(resonance);
    svfL2.SetRes(resonance);
    svfR1.SetRes(resonance);
    svfR2.SetRes(resonance);

    highPassL.SetFreq(hpCutoff);
    highPassL.SetRes(0.0f);
    highPassR.SetFreq(hpCutoff);
    highPassR.SetRes(0.0f);

    // PROCESS SAMPLES
    for(size_t i = 0; i < size; i++)
    {
        led.Update();

        highPassL.Process(in[0][i]);
        highPassR.Process(in[1][i]);

        svfL1.Process(highPassL.High());
        svfL2.Process(highPassL.High());
        svfR1.Process(highPassR.High());
        svfR2.Process(highPassR.High());

        // COMBINE FILTERS
        if (buttonValue == 1) {
            out[0][i] = (svfL1.Band() + svfL2.Band()) * VOLUME_NORMAL; 
            out[1][i] = (svfR1.Band() + svfR2.Band()) * VOLUME_NORMAL; 
        } else if (buttonValue == 2) {
            out[0][i] = (svfL1.High() + svfL2.High()) * VOLUME_NORMAL; 
            out[1][i] = (svfR1.High() + svfR2.High()) * VOLUME_NORMAL; 
        } else if (buttonValue == 3) {
            out[0][i] = (svfL1.Notch() + svfL2.Notch()) * VOLUME_NORMAL; 
            out[1][i] = (svfR1.Notch() + svfR2.Notch()) * VOLUME_NORMAL; 
        } else if (buttonValue == 4) {
            out[0][i] = (svfL1.Peak() + svfL2.Peak()) * VOLUME_NORMAL; 
            out[1][i] = (svfR1.Peak() + svfR2.Peak()) * VOLUME_NORMAL; 
        } else {
            out[0][i] = (svfL1.Low() + svfL2.Low()) * VOLUME_NORMAL; 
            out[1][i] = (svfR1.Low() + svfR2.Low()) * VOLUME_NORMAL;
        }

    }
} // END AUDIO CALLBACK METHOD


// MAIN METHOD
int main(void)
{
    /** Initialize the hardware */
    patch.Init();
    button.Init(patch.B7);
    
    samplerate = patch.AudioSampleRate();

    // GPIO pin, bool invert, samplerate
    // B8 sends PWM signal from 0V - 3.3V at ~500 mA
    led.Init(patch.B8, false, samplerate);

    svfL1.Init(samplerate);
    svfL2.Init(samplerate);
    svfR1.Init(samplerate);
    svfR2.Init(samplerate);

    highPassL.Init(samplerate);
    highPassR.Init(samplerate);

    /** Start Processing the audio */
    patch.StartAudio(AudioCallback);
    while(1) {}
}
