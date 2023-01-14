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

const float CUTOFF_MIN       = 20.0;
const float CUTOFF_MAX       = 20000.0;
const float RESONANCE_MAX    = 1.0;

DaisyPatchSM patch;
Svf svfL1, svfL2, svfR1, svfR2;

float   cutoff, fmKnob, fmCV1, fmCV2, 
        resonance, rmKnob, rmCV, 
        spread, spreadKnob, spreadCV,
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
    cutoff          = fmKnob + fmCV1 + fmCV2;
    cutoff          = RestrictRange(cutoff, 0.0, 1.0);
    cutoff          = cutoff * cutoff * CUTOFF_MAX;

    resonance       = rmKnob + rmCV;
    resonance       = RestrictRange(resonance, 0.0, 1.0);

    spread          = RestrictRange(spreadKnob + spreadCV, 0.0, 1.0);

    // CALCULATE SPREAD MAP

    // SET FILTER VALUES
    svfL1.SetFreq(cutoff);
    svfR1.SetFreq(cutoff);
    
    svfL1.SetRes(resonance);
    svfR1.SetRes(resonance);

    // PROCESS SAMPLES
    for(size_t i = 0; i < size; i++)
    {
        
        svfL1.Process(in[0][i]);
        svfR1.Process(in[1][i]);

        out[0][i] = svfL1.Low(); /**< Copy the left input to the left output */
        out[1][i] = svfR1.Low(); /**< Copy the right input to the right output */
    }
} // END AUDIO CALLBACK METHOD


// MAIN METHOD
int main(void)
{
    /** Initialize the hardware */
    patch.Init();
    samplerate = patch.AudioSampleRate();

    svfL1.Init(samplerate);
    svfR1.Init(samplerate);

    /** Start Processing the audio */
    patch.StartAudio(AudioCallback);
    while(1) {}
}
