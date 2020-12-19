#pragma once

//-------------------------------------------------------------------------------------------------

enum envelope_state
{
    sIdle = 0,
    sAttack,
    sHold,
    sDecay,
    sSustain,
    sRelease,
    sUberRelease
};

//-------------------------------------------------------------------------------------------------

struct envelope_AHDSR;

//-------------------------------------------------------------------------------------------------

class Envelope
{
  public:
    Envelope();
    ~Envelope();
    void Assign(float *envA, float *envH, float *envD, float *envS, float *envR, float *shape);
    void Attack(bool no_sustain = false);
    void Release();
    void UberRelease();
    bool Process(unsigned int samples);
    float output;
    float level, droplevel;

  protected:
    void SetState(long);
    void SetCurve(float);
    void SetRate(float Rate);
    float GetValue();
    float CalcCurve();

    float *A, *H, *D, *S, *R, *shape;
    envelope_AHDSR *edata;
    // float phase;
    // float rate;
    unsigned int phase, rate;
    unsigned int curve;
    bool no_sustain;
    long state;
    bool is_AEG;
    unsigned int block;
};

//-------------------------------------------------------------------------------------------------