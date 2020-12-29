# ShortCircuit 3

What? ShortCircuit is open source? Well yes, but there are big big caveats. **Right now there are no downloadable
assets you can to play music, and there is no runnable opensource ShortCircuit plugin**. We estimate this 
will be the case for much of 2021, and maybe indefinitely. **You may never be able to run an opensource
short circuit**. Might just be too much work.

## How we got here

VemberAudio, founded by Claes Johnason, released Surge and ShortCircuit in the 2000s. In 2018, Claes, realizing
he would not complete Surge 1.6, open sourced it. A community arose around it, leading to the surge we have today.

In late 2020, @baconpaul (one of the maintainers of Surge) sent a thank you note to Claes for open sourcing Surge
and they got to chatting about ShortCircuit. That led to Claes sending Paul the SC code, and that led to this repo 
and plan.

# Where we are

This repo contains a snapshot of the ShortCircuit 2 code which Paul has been chipping away on a bit, but it 
requires a lot of work to get into a running state.  Right now it is only of interest to developers and there are 
no downloadable assets or available programs that make music. 

The state of the code is much more primitive than the state of the Surge code when it was open sourced (which is one of the
things that gave Claes and Paul pause about proceeding). For instance

1. It uses windows-only APIs in fundamental ways
2. It is older C++, not using modern C++-11 and 14isms
3. It only compiles as a VST2 on windows, which of course doesn't work for a FOSS project
4. That VST2 on windows doesn't work even if you do get it to compile.

But that gives us a plan of attack

1. Try and get it to build and link all platforms by having a dumb windows stub library, so we at
   least know what windows APIs matter. This repo meets that constraint.
2. Build a 'headless' short circuit, bound to python at first, which lets us test and individually run
    the individual components of the sampler, rename stuff, clean up bugs, refactor code, etc...
3. Rewrite the front end using JUCE and that sampler backend
4. Call it ShortCircuit3 if we get it done

Ambitious.

# An important note about licensing

Right now the copyright to the SC3 code is held by Claes (on the code) and Paul (on his diffs since Claes shared the
code). And this repository you are looking at is licensed under GPL3. We know that, if we end up with ShortCircuit3,
we will end up with a GPL3 plugin, since it will depend on JUCE which is itself GPL3 if used in an open source
context.

But, Paul and Claes are discussing refactoring the code to be a mix of MIT and GPL3 code, with some critical
things like sample format loaders, some DSP code, and some utilities at least being released under MIT. We don't 
know where that line is yet, though, so this repo is GPL3. 
Since we both agree to shifting license in the future, we can of course re-license our code. 
But this means we cannot take quite the same stance
on inbound PR as we do with Surge, where we merge contributions under GPL3 and individuals hold their copyright.
If we were to do that with SC3 and want to re-license a subset of the code to MIT, the administrative burden would 
be high.

This is not a problem. CLAs exist and the like. But it does mean if you want to contribute to this repo we need
to chat and we probably want to execute some form of acknowledgement that you would be happy for your change to be
re-licensed at our discretion. Basically: If you think you want to work on this, either talk to @baconpaul on our
surge discord, or open an issue here and we can chat. My guess is we would just use one of the Canonical CLAs to have
you assign copyright to Claes and me, but we may do something else.

# How to build what we have

You know the drill

```
git submodule update --init --recursive
cmake -Bbuild
cmake --build build --config Release --target sc3py
cd build
python3 (and import sc3py code)
```

