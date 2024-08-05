# Core Architecture Concepts

## The Three "Big" Parts

There are three 'big' parts architectural to short circuit

- The engine, which has several functions
    - Storage of part/group/zone/bus configuration
    - The top of the audio processor graph
    - The "Owner" of the code local components like the selection manager, communications handle, etc
- The "Serialization Thread" which handles I/O and communication with the UI via the message bus
- The UI itself

In addition to these architectural concepts there's lots of DSP code,
loads of infrastructure to load sample formats, and all that sort of stuff.
That's outside the scope of this document, which just helps you navigate the
architectural choices.

## The Engine

## The Serialization Thread

That strategy is documented
in [messaging.h](https://github.com/surge-synthesizer/shortcircuit-xt/blob/main/src/messaging/messaging.h#L47) and
that documentation is not repeated here

## The UI itself

The central class for the UI is `SCXTEditor`

The operating model of the UI components is described in [this document](UIConcepts.md).
