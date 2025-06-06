#include "daisy_pod.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPod hw;

struct BenjolinRungler
{
    Oscillator osc1;
    Oscillator osc2;
    
    uint8_t shiftRegister;
    
    float sampleHoldValue;
    float lastClockState;
    int clockNumber;
    int clockCount;

    // Filter?
    // Svf filter;
    // Reverb?
    // ReverbSc reverb;

    // Param
    float osc1Freq;
    float osc2Freq;
    float feedback1;
    float feedback2;
    
    bool initialized;

    void Init(float samplerate)
    {
        osc1.Init(samplerate);
        osc1.SetWaveform(Oscillator::WAVE_TRI);
        osc1.SetFreq(100.0f);
        osc1.SetAmp(1.0f);
        
        osc2.Init(samplerate);
        osc2.SetWaveform(Oscillator::WAVE_TRI);
        osc2.SetFreq(50.0f);
        osc2.SetAmp(1.0f);
        
        shiftRegister = 0b00110010;
        sampleHoldValue = 0.0f;
        lastClockState = 0.0f;
        clockNumber = 4;
        clockCount = 0;
        
        // filter.Init(samplerate);
        // filter.SetFreq(4000.0f);
        // filter.SetRes(0.5f);

        // reverb.Init(samplerate);
        // reverb.SetFeedback(0.5f);
        // reverb.SetLpFreq(4000.0f);

        osc1Freq = 100.0f;
        osc2Freq = 10.0f;
        feedback1 = 0.5f;
        feedback2 = 0.5f;
        // filterFreq = 0.0f;
        // filterRes = 0.0f;
        
        initialized = true;
    }

    float Process()
    {
        if (!initialized) 
            return 0.0f;
        
        float osc1Out = osc1.Process();
        float osc2Out = osc2.Process();
        
        // clock
        float clockSignal = osc2Out > 0.0f ? 1.0f : 0.0f;
        
        if (clockSignal > lastClockState) 
        {
            clockCount++;

            if (clockCount == clockNumber) 
            {
                clockCount = 0;
                bool osc1Bit = osc1Out > 0.0f;
                bool osc2Bit = osc2Out > 0.0f;
                
                bool newBit = osc1Bit ^ osc2Bit;
                shiftRegister = (shiftRegister >> 1) | (newBit ? 0x80 : 0x00);

                // Ways to XOR
                // bool prevBit = (shiftRegister & 0x01) != 0;
                // bool newBit = (osc1Bit ^ osc2Bit) ^ prevBit;
                
                // s1 = osc1Out;
                // s2 = osc2Out;            
                }
        }
        
        // Ways to manipulate rungler output

        // float runglerOut1 = (shiftRegister & 0x0F) / 15.0f;
        // float runglerOut2 = ((shiftRegister & 0xF0) >> 4) / 15.0f;
        
        // osc 1
        float runglerOut1 = shiftRegister / 255.0f;
        // osc 2
        uint8_t lastThree = shiftRegister & 0x07;
        float runglerOut2 = lastThree / 7.0f;
        
        // TODO: Ways to use rungler output to control frequencies
        // linear
        // float modOsc1Freq = osc1Freq * (1.0f + (runglerOut1 - 0.5f) * feedback1);
        // float modOsc2Freq = osc2Freq * (1.0f + (runglerOut2 - 0.5f) * feedback2);

        // exp
        float feedbackScale1 = 4.0f;
        float feedbackScale2 = 3.0f;
        float modOsc1Freq = osc1Freq * powf(2.0f, feedback1 * feedbackScale1 * (runglerOut1 - 0.5f));
        float modOsc2Freq = osc2Freq * powf(2.0f, feedback2 * feedbackScale2 * (runglerOut2 - 0.5f));
        
        // safety?
        // modOsc1Freq = fclamp(modOsc1Freq, 20.0f, 20000.0f);
        // modOsc2Freq = fclamp(modOsc2Freq, 20.0f, 20000.0f);
        // modOsc1Freq = fmaxf(modOsc1Freq, 0.1f);
        // modOsc2Freq = fmaxf(modOsc2Freq, 0.1f);
        
        osc1.SetFreq(modOsc1Freq);
        osc2.SetFreq(modOsc2Freq);
                
        float mixSig = (osc1Out * (1.0f - 0.5f)) + (osc2Out * 0.5f);
        
        // filter.Process(mixSig);
        // float filteredSig = filter.Low();
        // float reverbL, reverbR;
        // reverb.Process(drySignal, drySignal, &reverbL, &reverbR);
        
        // rhythm?
        float runglerSig = (runglerOut1 - 0.5f) * 0.3f;
        
        return mixSig + runglerSig;
    }
    
    void SetOsc1Freq(float freq) { osc1Freq = freq;} // fmaxf(freq, 0.1f);}
    void SetOsc2Freq(float freq) { osc2Freq = freq;} // fmaxf(freq, 0.1f); }
    void SetFeedback1(float fb1) { feedback1 = fclamp(fb1, 0.0f, 2.0f); }
    void SetFeedback2(float fb2) { feedback2 = fclamp(fb2, 0.0f, 2.0f); }
    void SetClockNumber(int cnum) { clockNumber = fclamp(cnum, 1, 16); }
    // void SetFilterFreq(float freq) {}
    // void SetFilterRes(float res) {}
};

BenjolinRungler benjolin;
Parameter params[4];

bool button1State = false;
bool button2State = false;

void ProcessControls();

static void AudioCallback(AudioHandle::InputBuffer  in,
                         AudioHandle::OutputBuffer out,
                         size_t                    size)
{
    ProcessControls();
    
    for(size_t i = 0; i < size; i++)
    {
        float benjolinOut = benjolin.Process();
        
        // safety?
        benjolinOut = benjolinOut * 0.7f;
        
        out[0][i] = benjolinOut;
        out[1][i] = benjolinOut;
    }
}

void InitBenjolin(float samplerate)
{
    benjolin.Init(samplerate);
    
    // Control params
    params[0].Init(hw.knob1, 50.0f, 500.0f, Parameter::LOGARITHMIC);
    params[1].Init(hw.knob2, 5.0f, 500.0f, Parameter::LOGARITHMIC);
    params[2].Init(hw.knob1, 0.0f, 2.0f, Parameter::LINEAR);
    params[3].Init(hw.knob2, 0.0f, 2.0f, Parameter::LINEAR);
}

int main(void)
{
    float samplerate;
    hw.Init();
    hw.SetAudioBlockSize(4);
    samplerate = hw.AudioSampleRate();
    
    InitBenjolin(samplerate);
    
    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    
    while(1) {
        
        if (button1State) {
            hw.led1.SetRed(1.0f);    // Red = Feedback1 control
            hw.led1.SetGreen(0.0f);
            hw.led1.SetBlue(0.0f);
        } 
        else {
            hw.led1.SetRed(0.0f);    // Off = Osc1 Freq control
            hw.led1.SetGreen(0.0f);
            hw.led1.SetBlue(0.0f);
        }
        
        if (button2State) {
            hw.led2.SetRed(1.0f);    // Red = Feedback2 control
            hw.led2.SetGreen(0.0f);
            hw.led2.SetBlue(0.0f);
        } 
        else {         
            hw.led2.SetRed(0.0f);   // Off = Osc2 Freq control
            hw.led2.SetGreen(0.0f);
            hw.led2.SetBlue(0.0f);
        }
        
        hw.led1.Update();
        hw.led2.Update();
    }
}

void ProcessControls()
{
    hw.ProcessAllControls();
    
    if (hw.button1.RisingEdge()) {
        button1State = !button1State;
    }
    
    if (hw.button2.RisingEdge()) {
        button2State = !button2State;
    }
    
    if (!button1State) {
        float osc1Freq = params[0].Process();
        benjolin.SetOsc1Freq(osc1Freq);
    } 
    else {
        float feedback1 = params[2].Process();
        benjolin.SetFeedback1(feedback1);
    }
    
    if (!button2State) {
        float osc2Freq = params[1].Process();
        benjolin.SetOsc2Freq(osc2Freq);
    } 
    else {
        float feedback2 = params[3].Process();
        benjolin.SetFeedback2(feedback2);
    }
    
    // Potential effects
    benjolin.SetClockNumber(4);
    // benjolin.SetFilterRes(0.5f);
    // benjolin.SetFilterFreq(4000.0f);
}