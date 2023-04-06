## Day 46 (2023-04-07 with a squinch on the 6th)

Start thinking about multi-select and voice state.

## Day 45 (2023-04-06)

Really a micro-day; fix the generator loop if it abuts the end of the sample.

## Day 44 (2023-04-04)

Start with cleaning up the files for the processors some so you get
more like a 'one pair per processor' pattern which is way easier to
navigate. Add the sin oscillator and a quadrature osc to basic blocks.
Fix two tiny bugs with AEG/EG2 in mod matrix and filter display on startup.

## Day 43 (2023-04-03)

Superquickly, select the first new zone when importing an SF2. Then turn to the
"is" questions state (isAudioRunning, isClientConnected; #430) which lets me also
do things like engine status up to the UI properly (although I dont paint the
audio state now jsut the rest in the about screen). Fix some plugininfra lin (#487).
Add a few more SFZ opcodes so oddy's clarinet loads.

And a little extra. lipol -> basic blocks and the phase mod ported.

## Day 42 (2023-04-02)

Again a slow dev day. Some more fixes to my SF2 fixes which I think will solve
oddy's linux problem. Come to grips with the vagueness which is SFZ. Deep breaths
and rewrite my parser since i don't think you can parse SFZ with a grammar so just
do a parser like they did in the 17th century. And that makes basic SFZ import on
very simple SFZs work. Tomorrow is either pixels or processors or playing state. Not sure yet.

## Day 41 (2023-04-01)

Welcome to April! Anyway start by fixing stereo SF2 (#492). Make extension
checks on files case insensitive.

I think my next big swaths of work are: port more of the
zone processors, make the ui match the wireframe for a zone pixel perfectly, and
playing state indicators. I may also sneak in rudimentary sfz

## Day 40 (2023-03-31)

Again a slow dev day. Added FLAC support; added a single function to push an error to the client.
Make it show an error albeit a so so one when a sample doesnt load.

## Day 39 (2023-03-30)

Not much dev today. Screw around with SFZ some. Write a complete SFZ parser using pegtl and test it.
Also make it so missing samples show a clear error message and no lnnger crash if you note on (#488).
So maybe some dev after all.

## Day 38 (2023-03-29)

OK so we are on issues! So we can close some. Like cmake configuration of build type
(#388) which along the way fixed a problem with auval and the new selection I added yesterday.
Also turn on SSE 4.2 (#432). And make it so a dropped sample is auto-selected,
Make velocity-sensitive zone lookup work (#380). Fix up the pipelines (#389) and have them
run the super-tiny regtest (#412). Support Stereo SF2 (#475)

## Day 37 (the plane ride back)

- Selection Manager streamed
- Attachment pattern gets a bit tigher
- Mapping Region highlights selected zone in sync with sidebar
- Keyboard layout which actually plays notes (at fixed velocity)
- Click Zone to Select works
- Sidebar shows selection in sync with zones and startup path
- Drag to edit zone mappings

## The 10 day break (2023-03-18 - )

Smaller stuff in this period.

- Import a looped sample reads loop points *and* turns loop on.
- Add an about screen
- Implement AHDSR with Shape (some optimizaiton to do)

## Day 36 (2023-03-17)

Getting ready for a bit of a vaca and slowdown so make sure the NextSteps is OK. Also
fix a bug in the reverse-playback-loop-mode and re-add the oversampler for high ratio
sample playback. Then screw it. Add tuning support.

## Day 35 (2023-03-16)

Busier day. Start with making pitch bend and midi CC work with a crude smoother.

## Day 34 (2023-03-15)

It's still all playmodes. Restructure to the per-variation switches-based version in the UI
and the data structure and merge it. Then basically rewrite the generator object for every
part except the sinc window resampler, cleaning up and improving the looping and so on
support.

## Day 33 (2023-03-14)

It's all playmodes, loops, and zones today. Basically get some of this in the UI
and get some of the play modes working. For now I used the legacy non-parameterized
play modes from the old generator which map to a set of new cool switches which
is way better. Fix that, but it's good enough to merge. You can set up loops and
play them and activate and deactivate them on a sample and it more or less works.

## Day 32 (3032-03-13)

Move next to main, admitting that this is the course we are on. Then try
out a PR by a small tweak to json helpers and adding the next steps document
for consideration, turning on the plugins, etc. Then I fix a small tuning bug
Witker reported which also lets me segregate the playback ratio from the pitch
shift properly. Update the JSON parsing with a technical issue so Evil has one
less issue to make I guess. And merge that and start thinking about zone playback modes.
So add at least the data members for playback mode, start and stop sample point and
start and stop loop point. Then call it a day.

## Day 31 (2023-03-12)

Start work on setting up sample parameters (loops, play direction etc) but begin
that by making the zone have up to 16 samples for future round robin support and so on.

## Day 30 (2023-03-11)

A series of commits to see how much SF2 I can get working. Start with
at least loading and having samples sound, albeit out of tune, and
not restoring from a save. Make it load from a save next. Finally parse out all the envelope data and root
key and stuff; It's basically starting to work!

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
