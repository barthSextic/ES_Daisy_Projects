#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM patch;


void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{

    for(size_t i = 0; i < size; i++)
    {
        out[0][i] = in[0][i]; /**< Copy the left input to the left output */
        out[1][i] = in[1][i]; /**< Copy the right input to the right output */
    }
}

int main(void)
{
    /** Initialize the hardware */
    patch.Init();

    /** Start Processing the audio */
    patch.StartAudio(AudioCallback);
    while(1) {}
}
