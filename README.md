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

Just like with Surge, we welcome any and all contributions from anyone who wants to work on open source music 
software.

At the initial open-sourcing, the copyright to the SC3 code was held by Claes (on the initial code) and Paul (on his 
diffs since Claes shared the code).  This repository is licensed under GPL3. We know that, if we end up with ShortCircuit3,
we will end up with a GPL3 plugin, since it will depend on JUCE or VST3SDK which are GPL3 if used in an open source
context.

But, Paul and Claes are discussing refactoring the code to be a mix of MIT and GPL3 code, with some critical
things like sample format loaders, some DSP code, and some utilities at least being released under MIT in a subordinate 
library (and from a separate repo). We don't know where that line is yet, but we do want to reserve the right to re-license any
or all of the code here under an MIT license at our discretion.

As a result, we are asking individual contributors who want to contribute to SC3 to sign a CLA. 
We have used the Canonical/Harmony 1.0 CLA http://selector.harmonyagreements.org with
the following choices:

1. You retain copyright to your code. *We do not need you to assign copyright to us*.
2. You grant us a license to distribute your code under GPL3 or MIT; and your content under CC3 attribution

You can read the entire document [here](doc/ShortCircuit3-Individual-CLA.pdf). 

To agree to this document,
please add your name to the `AUTHORS` list in a git transaction where you indicate in the git log message
that you consent to the CLA. You only need to do this once, and you can do it in your first commit to the repo.

# How to build what we have

You know the drill

```
git submodule update --init --recursive
cmake -Bbuild
cmake --build build --config Release --target sc3py
cd build
python3 (and import sc3py code)
```

