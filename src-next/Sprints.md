# Baconpauls next sprints

- Next THreading Sprint
   - audio <-> serialization
   - Throw guards in the right spots
   - non-fiction novel
- Cleanup header guards
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
- Port in the rest of the processors
- Bring back the SST Libs and CMake fix up
  - including git version 
- What to do with the tests? Maybe just start new ones
- Zone Metadata
- Other Sample Formats other than .wav
- Content Browser
- The UI can work if theres no audio thread
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
- fs:: to std::filesystem

Ideas for the future
- Parallel or Serial Processors on zone for instance


----------
Sprint Log I posted on Discord

## Day 7 (2023-02-05)
All the message threads are hooked up and talking to each other 
and the JUCE app shows this. I can use object patterns to send 
messages between the UI and the Serialization thread. Upon a 
request the UI gets a readonly copy of the Patch object in its 
memory space. Next up is the proper object pattern for 
Serialization <> Audio Thread

## Day 6 (2023-02-04)

Mostly thinking about the messaging patterns. Wrote the non-fiction
novel about how the message queues interact. Started prototyping
some classes. I also revived the JUCE linkage and now have a little
standalone ui-compoennt-only JUCE app which will let us start
sending messages based on the CLI client. I got enough threads and
queues and locks and lock free stuff running that I can go from 
UI -> serialization thread in the engine. Tomorrow do
serialization thread -> audio thread and the other trips
and get startup json.

Also renamed Filter to Processor everywhere

## Day 5 (2023-02-03)

Here comes the JSON: So today I bound to the (amazingly awesome) 
taocpp/json library which basiczlly puts onto-offof json entirely 
into type safe template magic. But as a result by the end of the 
day and the push I just did I can take my engine in my cli player 
configured, stream it to json, start another instance and rather 
than doing the in-memory config, load that json, and voila, same 
sound same behavior. Also bought back the regtests and wrote some.

(no dev the second due to personal commitments)

## Day 4 (2023-02-01)

Modulation is basically working to voices, although very incomplete. 
And it is way cleaner than in the SCXT codebase (although still has 
the same fixed table with enumerated slots style, and I haven't done 
the edge functions).

But this code makes a warbling saw tooth amplutde with a sample attack

so by end of day 4 I have engine, patch, part, group, zone , 
voice hierarchy working; i can load and play samples; i can 
start and stop filters without allocating at audio time, 
and i can modulate. These are all 'minimal' codepaths 
(that is, I don't have all the data modulatable, 
I only have 2 filters) but the idea that a "fresh scaffolding" 
would go fast is proving true. I also see how to do things like 
microtuning and mpe which would have been really hard in the old 
code base


## Day 3 (2023-01-31)

Day 3 (probabky) end of dev blog (might do one more sprint later). 
Have “filters” in place infra wise including using placement new 
for most; I’m still at zero allocs on the audio thread. Have parameters 
starting to move over but don’t have any filters working. Tomorrow 
low pass and osc pulse sync into a test patch and then review the rest.

Day 3.1 update: I can run a zone with a OscPulseSync and have it 
generate sync waves instead of samples. Still don't have mixing yet 
but that's easy. Will add lowpass tomorrow and implement mix and that
will finish my "first filter sprint"

## Day 2 (2023-01-29)

Day 2 end of dev report: I can play polyphonic samples across the 
keyboard which I set up in my CLI and control with a keyboard. 
I can layer multiple zones on the same midi note. The re-tuning 
is correct (12-TET but I see where to add micro). The code is fairly 
reasonable but I still haven't checked it win/lin. I'm going to 
stay mac only for the week.

I also made a few CC0 samples with surge and bazille to 
configure the default patch. Here's me playing it on my LPK25 
at my workspace.

## Day 1 (2023-01-28)

No update posted but basically I started getting the data structures
together on the night of 28th and 29th and that became "day 1"