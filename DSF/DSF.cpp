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

DaisyPatchSM patch;
Svf svfL1, svfL2, svfR1, svfR2;

float   cutoff, fmKnob, fmCV1, fmCV2, cutoffSquared, 
        resonance, rmKnob, rmCV, 
        spread, spreadKnob, spreadCV,
        imageL1, imageL2, imageR1, imageR2, exponent, 
        samplerate;


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

    // GET ADC VALUES
    fmKnob          = patch.GetAdcValue(CV_3);
    fmCV1           = patch.GetAdcValue(CV_5);
    fmCV2           = patch.GetAdcValue(CV_6);

    rmKnob          = patch.GetAdcValue(CV_2);
    rmCV            = patch.GetAdcValue(CV_7);

    spreadKnob      = patch.GetAdcValue(CV_1);
    spreadCV        = patch.GetAdcValue(CV_8);

    // COMBINE PARAMETERS
    cutoff          = RestrictRange(fmKnob + fmCV1 + fmCV2, 0.0f, 1.0f);
    resonance       = RestrictRange(rmKnob + rmCV, 0.0f, 1.0f);
    spread          = RestrictRange(spreadKnob + spreadCV, 0.0f, 1.0f);

    // CALCULATE SPREAD MAP
    // NORMALIZE SPREAD TO [-1, 1]
    spread = (2.0f * spread) - 1.0f;
    cutoffSquared = cutoff * cutoff;

    // WRITE CUTOFF VALUE TO LED
    patch.WriteCvOut(CV_OUT_2, cutoff * 3.5);
    // COMPUTE IMAGES
    imageL1 = RestrictRange(pow(BASE_1, -spread) * cutoffSquared, 0.0f, 1.0f) * CUTOFF_RANGE;
    imageL2 = RestrictRange(pow(BASE_2, spread) * cutoffSquared, 0.0f, 1.0f) * CUTOFF_RANGE;
    imageR1 = RestrictRange(pow(BASE_1, spread) * cutoffSquared, 0.0f, 1.0f) * CUTOFF_RANGE;
    imageR2 = RestrictRange(pow(BASE_2, -spread) * cutoffSquared, 0.0f, 1.0f) * CUTOFF_RANGE;

    // SET FILTER VALUES
    svfL1.SetFreq(imageL1);
    svfL2.SetFreq(imageL2);
    svfR1.SetFreq(imageR1);
    svfR2.SetFreq(imageR2);
    
    svfL1.SetRes(resonance);
    svfL2.SetRes(resonance);
    svfR1.SetRes(resonance);
    svfR2.SetRes(resonance);

    // PROCESS SAMPLES
    for(size_t i = 0; i < size; i++)
    {
        
        svfL1.Process(in[0][i]);
        svfL2.Process(in[0][i]);
        svfR1.Process(in[1][i]);
        svfR2.Process(in[1][i]);

        // COMBINE FILTERS

        out[0][i] = (svfL1.Low() + svfL2.Low()) * 0.5f; 
        out[1][i] = (svfR1.Low() + svfR2.Low()) * 0.5f; 
    }
} // END AUDIO CALLBACK METHOD


// MAIN METHOD
int main(void)
{
    /** Initialize the hardware */
    patch.Init();
    samplerate = patch.AudioSampleRate();

    svfL1.Init(samplerate);
    svfL2.Init(samplerate);
    svfR1.Init(samplerate);
    svfR2.Init(samplerate);

    /** Start Processing the audio */
    patch.StartAudio(AudioCallback);
    while(1) {}
}
