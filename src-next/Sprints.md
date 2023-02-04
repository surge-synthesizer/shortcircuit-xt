# Baconpauls next sprints


- JSON Message Infrastructure
   - Do this as a sparate sprintlet 
   - A crude two process UI? What to do about the 'other side' structure?
- NEXT branch readme and CLI from clap-info
- Voice Design for real and zone level modulators
    - Lifecycle with GATED FINISHED CLEANUP etc
    - AEG/FEG
    - All the properties on the zone with the correct structures
    - What are the params and how do they get there
    - Pitch, pitch bend, etc...
    - Playmodes, Loops and start/stop points
    - Mod Targets
- vembertech factoring is probably sooner than later; copyblock etc
- All that round robin discord chat
- Find samples when streamed by path
- Samples in the patch
- Port in the rest of the filters
- Bring back the SST Libs and CMake fix up
  - including git version 
- What to do with the tests? Maybe just start new ones
- Zone Metadata
- Other Sample Formats other than .wav
- Content Browser
- Error reporting
   - those warnigns in riff_memfile
   - what sort of object?
- A plugin client which can at least configure a zone
- State streaming
- Engine Path to Keyboard Lookup way way faster (probably precache)

Open Questions and ToDos and Reviews:
- int vs sized type (int32_t etc)
- riff_memfile and riff_wave review
- what's all the stuff in samplevoice which didnt make it
   - oversampling
   - fades
- Sample GC
- Sample Reload All / Refresh

Ideas for the future
- Parallel or Serial Filters on zone for instance
