# Baconpauls next sprints

Week of Feb 13:
- The Selection and State Management Object
    - Message to get current part/group/zone hierarchy for a part 
      and current list of parts and to select part 0 as startup
    - Sidebar ui does bad display of said hierarchy with a juce::list or so
    - builtin patch has 3 zones again in one group
    - selection ui changes selection state (single select for now)
    - including no select
    - selection changes envelopes
    - If you can get that far, commit
- Remove the old clients and their subordinate libraries
- Revisit all the interaction code c
   - Classes and patterns 
   - better file class and enum names (like client messages gets split up)
   - non-fiction novel
   - Throw guards around json parses
- Then: Processor views
- Then: Add a zone by sample
- Then: Two Zones - select
- Then: A bit of UI work to make it look pixel-exact to wireframe
- A VU Meter
- Set up a 'next' nightly

once that's done
- Vembertech lipol_ps then its gone
- Generator keeps runnign after AEG is complete; don't
- Zone Variants and the zone-sample-wrapper data object
- Voice Design for real and zone level modulators
    - Lifecycle with GATED FINISHED CLEANUP etc
    - AEG/FEG
    - All the properties on the zone with the correct structures
    - What are the params and how do they get there
    - Pitch, pitch bend, etc...
    - Playmodes, Loops and start/stop points
    - Mod Targets
- Generator play modes (onRelease needs a change)
- Two big "is" questions: IsClientConnected; IsAudioRunning
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
- The curve from SC Envelope into the ADSR

Open Questions and ToDos and Reviews:
- int vs sized type (int32_t etc)
- riff_memfile and riff_wave review
- what's all the stuff in samplevoice which didnt make it
   - oversampling
   - fades
- Sample GC
- Sample Reload All / Refresh
- fs:: to std::filesystem
- I think we don't need this but: True/False readonly traits percolate by having template<bool T> scxt_traits (maybe)
- Loop cleanup wave alignment stuff


Notes
- Font is Intermedium

----------
Sprint Log I posted on Discord

## Day 14 (2023-02-13)

I finally start on the selection mechanics, but in doing so see
the pattern to fix up the Request/Response verbosity so implement
that quickly, as well as clean up the pre-juce clients and unneeded
submodules. Then turned to the registration time. With my refactored
messages I added a part/group/zone view and onregister message. The
onregister even sends me the full group/zone list at startup with names.

Also got it building on lin/win again.

## Day 13 (2023-02-12)

Unexpected hour of dev so I did some **Day 13** code shuffling. 
Basically made src-next into src and stuff; but also rewrote the 
client to be a proper juce plugin. And its great. The editor can't 
even see the engine - the engine is private on the processor. 
And so now I can run the standalone and play it wiht my midi 
keyboard. Selections and cleanup tomorrow.

## Day 12 (2023-02-10 - 2023-02-11)

But of code burnout so sort of slow down across the weekend for a two day "day 12".
Not much on Friday but on Saturday I got the pipeline from UI->serial->engine and back
all working with udpates from the engine to the ui or the ui to the engine bound
properly into the streaming and also into the sst-jucegui pattern. Cleanup to do but
the design is "completely in principle" even though the code is still overly verbose.

## Day 11 (2023-02-09)

Not a big dev day. Finished the rack refactoring to basic blocks and
made sc depend on it. Also added a new airwindows rack feature while back
in rack land (distractions distractions). Moved a bunch of vembertech
stuff into basic-blocks and reworked some of it. Also scripted a
consistent file comment / consistent header guard applier and applied it.


## Day 10 (2023-02-08)

A design day. 2 Hours on the phone with evil and we hashed out a lot
and thought about "group lock" mode; a tiny bit of code cleanup; start
refactoring some of the modulator code since we relaized a good first
target is the zone AEG working. Went back to rack to clean up that code
into sst-basic-blocks. Updated notes in sprints doc too!

## Day 9 (2023-02-07)

Some renames and cleanups at the open while I get ready for the next big
structure-of-the-ui change. Got the UI laying out regions and have the
classes starting to get named and stuff. Most importantly have a request
response pattern emerging for client data queries which will let me go
very quickly on building the UI once I've done my second "thing". Tomorrow
is some UI work for a change of pace. Tired of looking at templates.

## Day 8 (2023-02-06)

Audio->Serialization->Client works, with voice count messages going from the 
engine up to the UI. Client->Serialization->Audio works with mutatino from a UI
slider changing the value of the engine parameter. Also some internal refactoring 
to simplify the streaming requirements of messages by using the 'payload' pattern
and a variety of other code cleanup things.

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


-------------------
Convo with Evil

- saving
  multi as a directory
  multi as a zip
  part with patch references
  part with patches copied to folder
  part with patches in a zip

part box
- omni / mpe / channel
- s m are solo mute
- output is default output which is the 'part' output in group
- part tuning control: off is pitch bend wheen; modify incoming midi note
- poly will have mono and legato modes. legato (no retrigger) mono (retrigger) or 2+
- start with one part and have + button; part has a concept of 'active'. How to remove?
- rmb for emtpy / clear / duplicate

envelope and lfo boxes
- preset menu

mdoulation section
- obvious

output section
- mute is post fader probably

Processor block
- dropdown selects the effect
- dry wet mix is on the panel
- should be able to fit 6-8 knobs
- want a layout per effect basically. so crib from the rack code

Browser section
- for every sample aptch or multi get a "*" for favoriting
- colored hearts for favorite categories which you name
- draggable separator for the split. probably remembered as global setting.
- auto preview is speaker at bottom - click a sample to play it back at volume. press play
- auto load means as soon as you click you replace the zone. don't do that until i do undo :)
- search button replaces devices with search over file list by file name
- probably borrow the surge sql implementation
- need to add a back button
- keyboard navigation is a must

Group List
- selection sync with zone display
- alternates show under a zone as expansion
- Do we want group level filters? no; we just need group lock

Mapping
- drop a sample switch to group if you are on part automatically
- selection links to group/part etc...
- Exclusive groups and choke clusters
- File: Just samples inside the current folder
- Sample page - plus to add variants.
- Macros at part level (uni, bi, toggle)
