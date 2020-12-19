#pragma once

const int max_meq_entries = 512;

struct meq_band
{
    float freq, gain, BW;
    bool active;
};

struct meq_snapshot
{
    meq_band bands[8];
    float gain;
    char name[32];
    int bank;
    int patch;
};

class morphEQ_loader
{
  public:
    morphEQ_loader();
    ~morphEQ_loader();
    void load(int bank, char *filename);
    meq_snapshot snapshot[max_meq_entries];
    float get_id(int bank, int patch);
    void set_id(int *bank, int *patch, float val);
    int n_loaded;
};
