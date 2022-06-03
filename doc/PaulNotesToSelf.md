# Pauls notes to himself as he goes

## Playback and Voice Sounding

- sampler_voice
- most importantly, ::play takes a sample, a sample_zome, a sampel_part, as well
  as key velocity etc
- process_block
- most importantly, it plays for a zone z's sample in `sampler::PlayNote`

## Zones

Each zone holds a single sample ID and a collection of playback features

### What is it

- sample_zone in sampler_state.h
- contains all the parts you would expect (root, vel range, loop markers) as values
- contains a filterstruct, two envelopes, and 3 step LFOs
- contains a sampleid and group information
- contains some voice stuff

### How is it created 

- sampler has a `sample_zone zone[MAX_ZONES]` member

### What is the lifecycle

### How does it interact with playback / voice



## To be explored

- What is the 'filterstruct' class here and how does it relate to surge
