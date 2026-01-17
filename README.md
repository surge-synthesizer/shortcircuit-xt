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
cmake --build ignore/build --config Release --target shortcircuit-products
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

# A note about licensing

Just like with Surge XT, we welcome any and all contributions from anyone who wants to work on open source music
software.

At the initial open-sourcing of Shortcircuit 2 (later renamed to Shortcircuit XT), the copyright to the Shortcircuit
source was held by Claes (@kurasu) on the initial code and Paul (@baconpaul) on his diffs since Claes shared the code.
The current repository and program have GPL3 dependencies, so Shortcircuit XT binaries are distributed under the terms
of GPL3, due to the transitive nature of GPL3.

But, at the outset of the project, Paul and Claes both wanted the option to take the source which we developed for
Shortcircuit-as-Shortcircuit and potentially use it or fragments of it in a variety of other projects. So, since project
inception, users who contributed have agreed to a CLA [here](doc/ShortcircuitXT-Individual-CLA.pdf) which kept copyright
on all diffs with the author of
each change, but which gave the maintainers of this project the option to distribute all contributions under an MIT or
GPL3 license. This CLA was based on [http://selector.harmonyagreements.org](Canonical/Harmony 1.0).

As we approached the beta, we decided to exercise that option. As such - as of January 18th, 2026 - we've decided that:

1. The source in the `shortcircuit-xt` repo, outside of the `libs/` folder, is licensed under the MIT license, provided
   here in the file "LICENSE". The copyright to any particular line is still held by the author of that line as
   described on GitHub.
2. Each dependency in `libs/` folder has a license in the particular library repository. All these licenses are
   compatible with MIT source code and resulting GPL3 distribution.
3. The resulting combined product - Shortcircuit XT binaries and other associated CLI tools - are distributed under the
   GNU GPL v3 license or later, found at `resources/LICENSE.gpl3`
3. Users no longer have to agree to the CLA in order to contribute to the project. Their contributions are owned by them
   and governed under the MIT license.
4. In the event you choose to use Shortcircuit XT code in your project, you must either modify the code to break each of
   the GPL3 dependencies, license the GPL3 dependency in a non-GPL3 context, or distribute your final product under GPL3
   license.

If this is unclear and you have a relevant question related to your contribution to the project, please open an issue
here on GitHub, or ask on our [Discord](https://discord.gg/RcHTt5M55M)