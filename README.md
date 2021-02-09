# Shortcircuit XT

[![CI Build Status](https://dev.azure.com/surge-synthesizer/surge/_apis/build/status/surge-synthesizer.shortcircuit-xt?branchName=main)](https://dev.azure.com/surge-synthesizer/shortcircuit-xt/_build/latest?definitionId=2&branchName=main)

What? Shortcircuit is open source? Well yes, but there are big big caveats. **Right now there are no downloadable
assets you can get to play music, and there is no runnable open source Shortcircuit plugin**. We estimate this
will be the case for much of 2021, and maybe indefinitely. **You may never be able to run an open source
Shortcircuit**. Might just be too much work. But if you are dev you can help!

## How we got here?

Vember Audio, founded by [@kurasu](https://github.com/kurasu)/Claes Johanson, released Surge and Shortcircuit in the 2000s.
In 2018, Claes, realizing he would not complete Surge 1.6, open sourced it. A community arose around it, leading to Surge we have today.

In late 2020, [@baconpaul](https://github.com/baconpaul) (one of the maintainers of Surge) sent a thank you note to Claes for open sourcing Surge
and they got to chatting about Shortcircuit. That led to Claes sending Paul the source code, and that led to this repository and plan.

# Where we are

This repository contains a snapshot of the Shortcircuit 2 source code which Paul has been chipping away on a bit, but it
requires a lot of work to get into a running state.  Right now, it is only of interest to developers and there are
no downloadable assets or available binaries that make music.

The state of the code is much more primitive than the state of the Surge code when it was open sourced (which is one of the
things that gave Claes and Paul pause about proceeding). For instance:

1. It uses windows-only APIs in fundamental ways.
2. It is older C++, not using modern C++-11 and C++-14-isms.
3. It only compiles as a VST2 on windows, which of course doesn't work for a FOSS project.
4. That VST2 on Windows doesn't work even if you do get it to compile.

But that gives us a plan of attack:

1. Try and get it to build and link all platforms by having a dumb Windows stub library, so we at
   least know which Windows APIs matter. This repo meets that constraint.
2. Build a 'headless' Shortcircuit, bound to Python at first, which lets us test and individually run
    the individual components of the sampler, rename stuff, clean up bugs, refactor code, etc...
3. Rewrite the front-end using JUCE and that sampler back-end.
4. Call it Shortcircuit XT if we get it done.

Ambitious!

# An important note about licensing

Just like with Surge, we welcome any and all contributions from anyone who wants to work on open source music
software.

At the initial open-sourcing, the copyright to the Shortcircuit source was held by Claes (on the initial code) and Paul (on his
diffs since Claes shared the code).  This repository is licensed under GPL3. We know that, if we end up with Shortcircuit XT,
we will end up with a GPL3 plugin, since it will depend on JUCE or VST3SDK, which are GPL3 if used in an open source
context.

But, Paul and Claes are discussing refactoring the code to be a mix of MIT and GPL3 code, with some critical
things like sample format loaders, some DSP code, and some utilities at least being released under MIT in a subordinate
library (and in a separate repo). We don't know where that line is yet, but we do want to reserve the right to re-license any
or all of the code here under an MIT license at our discretion.

As a result, we are asking individual contributors who want to contribute to Shortcircuit XT to sign a CLA.
We have used the Canonical/Harmony 1.0 CLA http://selector.harmonyagreements.org with the following choices:

1. You retain copyright to your code. *We do not need you to assign copyright to us*.
2. You grant us a license to distribute your code under GPL3 or MIT; and your content under CC3 attribution

You can read the entire document [here](doc/ShortCircuitXT-Individual-CLA.pdf).

To agree to this document,
please add your name to the `AUTHORS` list in a git transaction where you indicate in the git log message
that you consent to the CLA. You only need to do this once, and you can do it in your first commit to the repo.

# How to build what we have?

You know the drill:

```
git submodule update --init --recursive
cmake -Bbuild
cmake --build build --config Release --target ShortCircuit3_Standalone
```

This will build the rudimentary standalone app. Other targets exist for Python as a wrapper, as well as `sc3-test`
which runs the existing test suite.

# Issues building
* Windows - Some JUCE targets have an issue if you are using nmake (specifically you might get an error that
  looks like this: `'Synth\""' is not recognized as an internal or external command`). To solve this, use the Ninja
  generator by passing `-G Ninja` to the first cmake command (you might need to download Ninja from
  https://ninja-build.org/ if you don't already have it.)

