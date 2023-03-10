# Baconpauls next sprints


Early March
- MultiSamples - at least SF2 making sound some
- Stereo and Mono Samples in the Zone and Processors
  - Output section 
- Zone Edits
   - Make it so adding a zone refreshes by factorig that better 
   - a2s_structure_refresh can just be the two lambda form
   - Sample waveform
   - Header
   - That kiudna thing
- Then: Mod Matrix Depth
  - Calculated but how to share with UI? 
- Then: LFO Editor Finished
  - Show the waveform
  - steps as discrete choice
  - the multiswitches
  - the arrows (which we can do locally)
  - Probably un-screw the streaming of the presets maybe
- Add some more modulation sources
  - the midi ones (keytrack, velocity, etc...)
- Then modulation for a voice should be "done"
- Comb Filter and Processor Stereo Revisit
- Then: Add a zone by sample revisit especially filters borked on add
- Then: Implement the memory pool more intelligently
- JSON .at() to findOr almost everywhere and add a stremaing version findIf
- Voice Design for real and zone level modulators
  - Lifecycle with GATED FINISHED CLEANUP etc
  - AEG/FEG
  - All the properties on the zone with the correct structures
  - What are the params and how do they get there
  - Pitch, pitch bend, etc...
  - Playmodes, Loops and start/stop points
  - Mod Targets
  - Stereo: So the thing I want is to have processors advertise if 
    they can accept mono in, if they can produce mono out from mono in, 
    and if stereo is different from dual mono and also if you consume 
    or ignore your input
- A VU Meter
- Runtime Startup Constraint Checks like streaming names are unique
- microgate hold units
- Hover Gestures in Named Panel
- Vembertech lipol_ps then its gone
- Generator keeps runnign after AEG is complete; don't
- AEG doesn't keep running after Generator is done. Do.
- Zone Variants and the zone-sample-wrapper data object
- Big Punt: Multi-Select
- Generator play modes (onRelease needs a change)
- Two big "is" questions: IsClientConnected; IsAudioRunning
- All that round robin discord chat
- Find samples when streamed by path
- Samples in the patch
- Port in the rest of the processors
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
- Samples load off thread
- Throw guards around json parses
- Restricts in the copy blocks in the mechanics helpers in basic blocks
- MicroGate gets mono and stereo implementation

Open Questions and ToDos and Reviews:
- Bring back ASIO
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

----------
Sprint Log I posted on Discord

## Day 29 (2023-03-10)

Start with integrating libgig so we can do SF2, Korg, Akai and GIG files
as load points. Get it building and use it to at least parse out some SF2
structure. This weekend lets see if I can make it play those samples!
(along the way added the mutate-from-audio-thread-safe-stop thing too).

## Day 28 (2023-03-08)

Stereo Processing!

## Day 27 (2023-03-07)

Ugh not feeling it. Got some more of the zone display on the screen
at least. Then called it. Then I uncalled it, wrote the zone visualizer
and fixed a problem with root keys. Then added boice plaback
as a mod target, fixed a problem with root keys, and generally thought
about whats next.

## Day 26 (2023-03-02)

A bit of unexpected time. So get enough of an LFO editor together
that we can at least screw with values and rates and know what the
presets are supposed to do. Then add a hook for the proper depth scaling
for the modulation matrix, and set it up for processors properly.

## Day 25 (2023-03-01)

A busy day with not much time, but did two things. First I got
clang building as a compiler option on windows in CI and turned
it on as default for now. Second I got all the LFO data and messages
flowing UI <-> Serializer <-> engine and was able to select an LFO
preset and change the rate and assign it in the modulator and hear
the effect. With a million caveats, like nothing is labeled, the units
are wrong, etc.... but probably a good starting point for the weekend
to kind of finish up all of modulation. 

## Day 24 (2023-02-27)

Get the mod matrix editable, albeit with depth units still screwed
and curves and the secondary source ignored, and the ui not finished.
But all the communication and hookup and streaming and engine mod
and zone selection works. Then start on making LFO do the same but
end day early since we have company visiting for a bit!

## Day 23 (2023-02-26)

Start cracking on the mod matrix in earnest. Get quite a bit started but nothing
done, with messages sending metadata and the routing matrix over, the start of
display names (and a lot of TODOs about an ugly structure) and a UI which at least
puts things in place and swaps around. Should be able to get some crude edits going
soon from here, but definitely a "progress on the path" not a "stopping point" day.

## Day 22 (2023-02-25)

Start with trying to port microgate which leads to some fun changes like
implementing a memory pool, writing a bunch of documentation, and changing
a few apis. Most importantly, all processors are stereo in stereo out by default
and the complexity of maintaining an infrequently used mono side is gone. But
by the afternoon have the microgate ready for merge 1. Fix up some straggler stuff
in the evening, including knbos have their 0 point be 0 in bipolar mode

Then demonstrate the custom panel design. Neat! Works well for SuperSVF.

Finally the MemoryPool makes it clear we can do inplace new for the raw processor
in all cases, so remove the support for the allocating version, make a static assert
if your object becomes too big, and just set the course for inplace new only.

## Day 21 (2023-02-24)

Processors! I got the messages working and the float editors up in the
UI and they snap at voice on at least. Woof. Then go change the mod matrix
so the destinations are indexed and that lets me intermediate the processor
through it, meaning knob drags while playing change processors. So to finish off
the day, which in retrospect was pretty productive, I also made int params express
themselves as control descriptions, blasted them to the ui, and painted them
as a multi-switch.


## Day 20 (2023-02-22 for a smidge then a smidge more on the 23)

Start with some evening cleanup removing the old unused ControlMode enum as
part of my approach to cleaning up ControlDescriptions and making Processors work.
Then look at those message types and realize its enough of a pattern to introduce a
few helper macros; so rewrite the messaging to use those and reduce the code enormously.

With these newfound bits and bobs, start cracking on processors. Begin by
creating the list of implemetned processors and sending it to the UI on 
registration. This let me make the hamburger menu on processors which is
the start of the processor round-trip (which is kinda like a dynamic version
of the ADSR round trip). But then I ran out of steam, so processors tomorrow.


## Day 19 (2023-02-21 for a smidge then really the 22)

Push ControlDescriptions down to the connector. This allows formatting and
is path to get tooltips and so on on the ADSR sliders. Add a crude tooltip.
Then push next up to the main repo and turn on azure again.

Then turn to zone mapping. Make a zone mapping metadata object, stream it, 
run it through the UI and so on, and make it all just work so you can edit
zone mapping. While in there, make the juce streaming work (but this is of 
course not stable) so we can save sessions and turn off the default patch.

Solid day.

## Day 18 (2023-02-17)

Last day before a 4 day hard break (no laptop travel). Try to get the ui components
in place so I can start writing some of the zone work, so add tabbed and hamburgered
named panel and use in some spots and use those to add some features to the UI.

## Day 17 (2023-02-16)

Lighter dev day again. Make Comic Sans the error font everywhere. Phew.
Then onto the add zone gesture. So actually plumb up the drop -> path 
-> serial -> load -> audio -> configure -> serial -> notify pipeline and
voila you can drop a sample on and it maps into part 1 group 1 from 
48..72 with root note 60 reliably! Huge! And thats it for the day today.

## Day 16 (2023-02-15)

Slower day with some busyness keeping me away from dev but did some
UI focus. Moved to using more of hte jucegui components properly, including
a clever factoring of how the model hooks up to the UI which is really super
easy, and doing a closer but not perfect layout of the ADSR, plus some style
sheet work to accomadate and start thinking about chasing the figma. Probably 
processors or maybe add zone next.

## Day 15 (2023-02-14)

More than 2 weeks at it. But going strong. Started by doing 
some cleanup from the overnight reports on other systems 
like show the voice count in the header and so forth. I also
changed something in the UI threading which greatly speeds up
response to inbound messages and may mean SCXT doesn't need an
idle loop. Right now idle does nothing.

But the real meat today was part group zone selection. Start bu
making a crap juce view of the structure which I can select into.
And that lets me write the single selection message which switches
the active zone server side and updates the view client side.

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
