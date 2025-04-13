# Shortcircuit XT

![Release Build Status](https://github.com/surge-synthesizer/shortcircuit-xt/actions/workflows/build-release.yml/badge.svg)

Welcome to Shortcircuit XT!

Shortcircuit was a popular creative sampler in the mid 2000s from vembertech, the
company that originally created surge. In 2021, Claes Johansen, the principle
at vembertech (and now at Bitwig), open sourced a version of Shortcircuit2
and gave the source to the Surge Synth Team.

After several false starts getting this code running, we made a decision in
early 2023 to preserve much of the design and DSP code from SC2, but to
rebuild a sampler in the spirit of the first two versions of Shortcircuit using
modern C++ and JUCE. This project, which you are looking at here, is called
Shortcircuit XT (SCXT) and is underway.

Right now, you can download our alpha version of the
instrument [here](https://github.com/surge-synthesizer/shortcircuit-xt/releases/tag/Nightly)
or you can build it using the instructions below.

## What Works, and How Can I Help?

Well first, thanks for asking! As we proceed through summer 2023, more and more
stuff is starting to work, but we have a lot to do to get to a working beta.
We have a rather complete wireframe of the final product, though, and have made
quite a few core decisions about selection, mechanics, playback, and more, some of which
are reflected in the code.

If you are going to use the product now, though, many things will be incomplete or
mysteriously not work. Really the best way to particpate is to join the [surge synth team
discord](https://discord.gg/RcHTt5M55M)
and come say hi in the #sc-development channel.

There's lots of ways you can help. Of course, developers are always welcome. But testers,
design partners, and just generally nice people who want to shoot-the-you-know-what about
a sample-initiated instrument can join in the fun. Also, folks who loved and used SC1 and 2
are welcome to come and see what we are thinking is different and the same, and help us
determine if we are right!

## I would love to build it myself. What do I do?

To build, first configure your machine. Basically set up your machine the same way you would
[to build Surge XT](https://github.com/surge-synthesizer/surge#setting-up-for-your-os) then
fork this repo and:

```bash
git clone <this repo or your fork>
cd shortcircuit-xt
git submodule update --init --recursive
cmake -Bignore/build -DCMAKE_BUILD_TYPE=Release
cmake --build ignore/build --config Release --target scxt_plugin_All
```

Our production build uses clang on macos, and gcc on linux. We
will test with a wide variety of compilers, including msvc on windows and several gcc versions.
If you are using a new compiler and have changes to the CMake or so on, please
do send them to us.

## How we got here?

Vember Audio, founded by [@kurasu](https://github.com/kurasu)/Claes Johanson, released Surge and Shortcircuit in the
2000s. In 2018, Claes, realizing he would not complete Surge 1.6, open sourced it. A community arose around it, leading
to Surge we have today.

In late 2020, [@baconpaul](https://github.com/baconpaul) (one of the maintainers of Surge) sent a thank you note to
Claes for open sourcing Surge and they got to chatting about Shortcircuit. That led to Claes sending Paul the source
code, and that led to this repository and plan. We had a couple of false starts, but are
confident in the path and architecture we have now!

# An important note about licensing

Just like with Surge, we welcome any and all contributions from anyone who wants to work on open source music software.

At the initial open-sourcing, the copyright to the Shortcircuit source was held by Claes (on the initial code) and
Paul (on his diffs since Claes shared the code). This repository is licensed under GPL3. We know that, if we end up with
Shortcircuit XT, we will end up with a GPL3 plugin, since it will depend on JUCE or VST3SDK, which are GPL3 if used in
an open source context.

But, Paul and Claes are discussing refactoring the code to be a mix of MIT and GPL3 code, with some critical things like
sample format loaders, some DSP code, and some utilities at least being released under MIT in a subordinate library (and
in a separate repo). We don't know where that line is yet, but we do want to reserve the right to re-license any or all
of the code here under an MIT license at our discretion.

As a result, we are asking individual contributors who want to contribute to Shortcircuit XT to sign a CLA. We have used
the Canonical/Harmony 1.0 CLA http://selector.harmonyagreements.org with the following choices:

1. You retain copyright to your code. *We do not need you to assign copyright to us*.
2. You grant us a license to distribute your code under GPL3 or MIT; and your content under CC3 attribution

You can read the entire document [here](doc/ShortcircuitXT-Individual-CLA.pdf).

To agree to this document, please add your name to the `AUTHORS` list in a git transaction where you indicate in the git
log message that you consent to the CLA. You only need to do this once, and you can do it in your first commit to the
repo.
