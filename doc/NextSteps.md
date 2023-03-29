# Next Steps (in gropued-by-function order) 

This was imported as issues on March 28 2023 and is here just in case those issues don't make sense on review

* Voice Management
  * Voice Lifetime Optimization ! Sample end, no processor, no loop, no repeat, kill voice ahead of AEG
  * Velocity Sensitive Voice Lookup ! Right now velocity range is mostly ignored I think
  * Zone from Key Cache ! Move from linear search to cached data structure on noteOn/noteOff
  * Poly mode works (this should be "done" but lets check it) 
  * Document Mono modes ! Mono Mode means several things as far as zone loopup on key change and retrigger.
  * Implement Mono modes ! Once we have them documented, do them.
  * MPE on a per-part basis
  
* Technical issues
  * Non-allocating memory pool ! We have the API but right now we just alloc in `scxt::engine::MemoryPool`
  * Add User Options from pluginInfra
  * JSON Versioning ! This matters for streaming and there's a start but we can be more intentional
  * JSON Throws ! Make sure all JSON parses are wraped in a recoverable try/catch
  * vembertech factoring beyond lipol_ps ! Move everything to templated basic-blocks
  * Unreferenced Sample Lifetime ! When do we free/GC samples from the sample manager?
  * Error Reporting API ! Serial, UI and Engine threads all need to generate errors to the UI
  * The 'is' questions ! isClientConnected; isAudioRunning, etc... and dealing with the queues if not
  * Code Review for Explicit Types ! make sure we are using int32_t rather than int etc... everywhere
  * Turn on SSE 4.1 ! Test this by using `mm_hadd_ps` in the generator sum
  * Review and rememdiate all the `// TODO` points I put in the new code either by making them issues or fixing
  * CMake BYOJUCE etc ! Allow people to BYOJUCE SIMDE etc... since I'm sure the linux folks will want that one day.
  * CMake ASIO Build ! Restore an ASIO build option for self-builders
  * CMake Skip VST3 Build
  * Improved CI Performance ! Split azure pipeline on mac for x86 and arm to speed up CI

* Multi-Output
  * Multi-Output Group Design ! What output paths does a group have
  * Design the Mixer Screen

* Voice
  * Velocity fade zones
  * Keyboard fade zones
  * Pre-Filter Gain
  
* DSP
  * Generate the Zone Procssor List ! What is the list of zone processors we want. Open an issue for each
  * Generate the Group Processor List ! What is the list of group processors. Issue for each
  * Part / Mixer Processor List ! What is the list of part/mixer processors. Issue for each.
  * Zone Processor routing ! This needs implementing
  * Finish the AHDSR | AHDSRShapedSC pow vs fastpow test and profile. Test and examine curve shapes.
  
* Modulation
  * Modulation Depth Display ! Un-screw the depth and index stuff (this is a technical baconpaul issue)
  * MIDI Sources in the Mod Matrix ! Modulation sources for all the MIDI and so on
  * Poly AT in the Mod Matrix ! It's just different enough to be its own issue
  * LFO Presets ! LFO Presets are kinda screwed. Revisit them.
  * MPE Modulators
  * Identify Sample Targets ! What parts of sample etc... are modulatable (list all the targets)
  
* Generator
  * Loop Fades
  * DSP Support for alternate bit depths beyond I16 and F32
    * 24 bit - currently upscales to F32. OK?
    * 8 bit - currently upscales to I16. OK?
    * 12 bit

* Temposync Support
  * Tempo and Time into the engine ! Get the juce info into the engine in a proper data structure

* Zone
  * Round Robin Support
  * Zone edit on Sample (like can we pick a different sample for a zone?)
  * Zone output section and routing
  * (An open issue for each section)
  * Delete/Rename
  * Solo
* Groups
  * Fully design all the group functions and document them
  * Add/Delete/Rename
  * Trigger Conditions
  * Group LFO/Modulator
  * Group Processors
  * Group output routing
* Parts
  * Design the Part screen view 
* File Browser
  * Previews and AutoLoads
* Pad View
* AutoMapping
* Macros
  * Mapping to MIDI CC
  * Mapping to plugin parameters
  * CLAP PolyMod

* Mixer Screen
  * Design

* Multi-Select
  * Design MultiSelect ! Rules and description of how this works
  
* I/O
  * Part Save and Load ! An individual part streams
  * Multi Save and Load ! An entire collection streams
  * Sample-by-value file format ! Compound (with samples in) file format for Part and Multi
  * Missing Sample Resolution
  
* Microtuning
  * Microtonal Zone Resolution ! Mode other than `re-zone and shift minimally` 
  * Native SCL/KBM support
  * MTS-ESP Snap on note-on ! vs continuous
  * Tuning Menu and Information

* Formats
  * riff_memfile vs libgig ! Do we need riff_memfile with libgig still?
  * WAV file Loop Markers and Root Note Markers
  * SF2 MultiInstrument Prompt and Expand ! An SF2 with multi-should prompt users as opposed to load all as zones.
  * SF2 Float 32 support ! Don't currently do it
  * GIG/DLS Support ! libgig gives it all to us right?
  * SFZ Support ! Whats the plan?
  * AKAI S6K support ! Tricky even with libakai it seems
  * AIFF File Import
  * FLAC File Import
  * OGG/Vorbis File Import
  
* MIDI
  * Improve the Controller Smoother ! That smoother is pretty limited
  * Sustain pedal and Hold Groups

* Accessibility
  * Implement Rudimentary Acecssibility

* Play Mode
  * Design Play Mode

* RegTests
  * Run in Pipeline ! Run them in the pipeline on pull request only 
  * Beef Up Regtests ! We should have tests which do stuff as opposed to just test streaming
  
* CLAP Support
  * Clap Sample Collection API/Extension

* User Interface
  * About Screen ! About Screen needs filling out quite a bit
  * NoteNames (C3/C4/C5) ! vs hardcoded C4 == MIDI 60
  * Style Sheet Setup ! Map phyical style sheet to logical style sheet
  * StyleSheet Editable 
  * Layout UI Issues ! Add an issue for each sub-section of the UI matching the wireframe basically  
  * VU Meter
  * Playing Zone Display
  * Playing Zone indicator and perhaps Playing POsition in Sample Indicator
  * Bring in the Scope ! Do we want to life in the Surge Scope for probing things?
  * JUCE_DECLARE_COPYABLE_ everywhere
  * Inactive knobs shouldn't draw
  * Waveform draw and edit
  * Attachment Pattern ! Promulgate the attachment pattern form ADSR ui to other bits ASAP
