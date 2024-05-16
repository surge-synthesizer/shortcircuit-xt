# How the UI Hangs Together

The SCXT UI works based on a few core ideas:

1. We have a custom widget set in `sst-jucegui` written in juce with styles and a data model/render-action separation
2. We have an engine-side model which is only mutable in the engine using the Serialization thread described
   in `messaging.h`
3. The UI then ends up as a 'dumb' terminal, sending and receiving messages from the engine and using those messages to
   maintain a local data set which drives the widgets by attaching the `sst-jucegui` datamodel to the message stream
4. Laying out a screen is simple construction of widgets and attachment objects, then positioning them, and we have a
   variety of functions which automate this attachment.
5. A few data items (VUMeters, SampleWaveforms) could be streamed as messages, but for efficiency, are now presented as
   read-only shared memory (a `const &`) for level and waveform rendering

There's quite a bit of nuance in making this all work; but the majority of the SCXT infrastructure code hides that
nuance behind template functions or macros so you don't need to think about that. This document works to describe 1-5
above without unpacking that internal detail.

## The `sst-jucegui` datamodel/render-event separation

`sst-jucegui` has three core concepts. A stylesheet, which is out of scope for this document, a set of components
which are items which process UI events and render to screen regions, and data items, which are sources to drive
components.

There are three core data classes you need to
understand, [here](https://github.com/surge-synthesizer/sst-jucegui/tree/2e645d112f88768fc0b9675b77ec6af839772d64/include/sst/jucegui/data)

`Continuous` and `ContinuousModulatable` contain one or two continuous values. `Discrete` contains one discrete value.

These data sources have core methods to describe them (`getMin/Max/Default`) and a way to retrieve the value `getValue`
which are astract virtual functions. They also contain two core action methods `setValueFromGUI` and `setValueFromModel`
which allow a component (first case) or model (second case) to indicate that the data has been changed. The classes
also have a variety of other housekeeping methods which for the sake of this we can ignore, and also methods to convert
values to and from strings.

A component is a `juce::Component` or subclass which has a method to set the source, for
instance `setSource(Continous *)`,
and members to get the source and so in on the paint and event methods. This separation means multiple components can
talk
to one data source, but also allows you easily changing renderers for a source.

The job of a component is to interact with the data source appropriately. For instance, you could write a (bad) UI
component
which when you clicked it, it set the value to a random value, and painted color more red the closer you were to max.
For instance

```cpp
void paint(juce::Graphics &g) override
{
    if (!data)
        return
    auto v01 = data->getValue01(); // (which is (data->getValue()-data->getMin())/(data->getMax()-data->getMin())
    g.fillAll(juce::Colour(v01 * 255, 128, 128));
}

void mouseDown(const juce::MouseEvent &) override
{
    if (!data)
        return;
    
    onBeginEdit();
    data->setValueFromGUI((rand() % 2048) / 2048.f * (data->getMax()-data->getMin()) + data->getMin());
    onEndEdit();
}
```

You get the idea. The base class `ContinuousParamEditor` in components handles most of this for in practice, and yuo
would want styles and sto on, but you can see that this renderer is totally decoupled from the store, communication
model, and nature of the underlying data.

## The engine-side streaming bidirectionally

If you
read [this extensinve comment in `messaging.h`](https://github.com/surge-synthesizer/shortcircuit-xt/blob/7d9759fa449abd1d7c9c39a76b72bf3ac7226a81/src/messaging/messaging.h#L47)
you can understand our threading model. This threading model allows things like clean maniuplation of the complicated
engine structure in a thread safe way, off thread sapmle loading, but also ui interactions. In order for the UI to
work the client (in this case, the UI) has to interact with the serialization thread (the non-audio part of the engine)
in three ways. Receive values, recieve metadata, and send values or actions.

For the rest of this docluemtn we use `ST` to mean serialization thread and `CT` to mean client gui thread.

### How values become streamable

SCXT uses `taocpp/json` to allow any C++ object to define a stream- and unstream- operator. There's some
infrastructure behind this, but the upshot is if you want a value to be saveable in a patch or sendable
to the UI it needs to stream or be a member of an object which streams.

The model is basically already set up, and as of this writing we are mostly transitioned to some
clean macros which hide the details. For instance, [here is how the `engine::Group::GroupOutputInfo' data
strcture defines streaming](https://github.com/surge-synthesizer/shortcircuit-xt/blob/7d9759fa449abd1d7c9c39a76b72bf3ac7226a81/src/json/engine_traits.h#L121).

This means that SCXT can generate a streamable object from an instance of `engine::Group::GroupOutputInfo'. If you add
a field you need to go ahead and add it to the streaming definitino first. For instance, here's the
[commit where we add the oversample-by-group field](https://github.com/surge-synthesizer/shortcircuit-xt/commit/63244d09884672f72b0b5022c6ff7390e9476764).

### Values: Serlizalization to Client

Once an object is streamable, the `ST` can send a copy of its internal state to the `CT` by streaming the object,
sending a message to the `CT`, and having the `CT` unpack it. Because of some details of how `taocpp/json` works
this works at a very high message rate quite nicely. But the activity of actually streaming a value and doing the
hookup and so-on requires some annoying hookup work. We have implemented that once and now have a set of
messages `s2c` which take an object on the `ST`, pack it up, send it to the `CT`, unpack it, and call a method
on the recipient object (which in our case is an instance of `SCXTEditor`) with the object.

Following the above example, the serialization engine can send a pair of data elements for output information.
Is the output region active in the UI (which is driven by selection) and what is the value of the output region.
Since `taocpp/json` can stream `std::tuple` automatically, we implement
the [definition of this message here](https://github.com/surge-synthesizer/shortcircuit-xt/blob/7d9759fa449abd1d7c9c39a76b72bf3ac7226a81/src/messaging/client/group_messages.h#L39)
and that
calls `onGroupOutputInfoUpdated` [defined here](https://github.com/surge-synthesizer/shortcircuit-xt/blob/7d9759fa449abd1d7c9c39a76b72bf3ac7226a81/src-ui/components/SCXTEditorResponseHandlers.cpp#L205).

Clearly we now have a mechanism which would allow us to make a call on the `ST` to send a bundle to the `CT`. We are 1/3
of the
way to building a protocol!

### Metadata: Serlialization to Client

Metadata is special. We can stream it but we also want to define it for individual fields. Again there's lots behind
the scenes here, but if an object is streamable we can define the metadata for its elements and stream that separately
from the values to allow configurable UIs. In other words, our compound records can attach metadata to make them self
describing, and the UI can consume this for labels etc.

We use the `sst-basic-blocks` ParamMetadata class for this and attach it to fields in a couple of ways (Processors are
different) but mostly through an `SC_DESCIBE`
macro. [Here's how we describe GroupOutputInfo](https://github.com/surge-synthesizer/shortcircuit-xt/blob/7d9759fa449abd1d7c9c39a76b72bf3ac7226a81/src/engine/group.h#L226)

### Values and Actions: Client to Serialization

Hey I bet we can do this all the other way too, righ! Sure! We have `c2s` messages which are the opposite. They define
a message, a payload, and a method to call when the client sends the payload.

These fall into two categories, value-types for indivial values, and custom actions. The
`group_messages.h` linked above shows both. We define an operator (`UpdateGroupOutputFloatValue')`which updates
one float inside a GroupOutputInfo using shorthand and lots of magic, but we also define
a `renameGroup` operator with a custom payload (here an address and a name) and a
custom function, which renames the group and then updates the client. Worth reading! Probably
expand this when someone reads the document. The client can just call this function and
it packs up data and sneds it over. For instance, here we
are [calling that `renameGroup`](https://github.com/surge-synthesizer/shortcircuit-xt/blob/7d9759fa449abd1d7c9c39a76b72bf3ac7226a81/src-ui/components/multi/detail/GroupZoneTreeControl.h#L305)

## The UI model; events, attachments, and local copies

OK cool, so we have widgets / data models separated in sst-jucegui. We have engine/client separated in the serliazation
architecture. Now if we just had a way to have the client-side endpoint automatically make an instance of a jucegui
data model for a given field, it would take one or two lines to make a knob bidirectionally connected to the
back end data model. Just.

Making that paragraph 'true' is the work of the ui connectors. These introduce a set of classes, which we've
colloqually called `attachments` which are an instance of a jucegui data model (so speak the data interface of
jucegui on one side) but are implemented with the serializatio nmodel on the other side (so get-value reads a field,
get min/max reads the metadata, and setValueFromGUI calls a c2s method to update, and setValueFromModel responds
to an udpate from th server, roughly). All automatically.

Doign this is pretty tricky, but once its done the API can be pretty compact. You end up with a pattern where
you have for each UI control a pair of objects, a `std::unique_ptr` to a component and a `std::unique_ptr` to an
attachment. We have helper functions that will construct both and hook them up in many common cases, and that
leads to code
like [this](https://github.com/surge-synthesizer/shortcircuit-xt/blob/7d9759fa449abd1d7c9c39a76b72bf3ac7226a81/src-ui/components/multi/AdsrPane.cpp#L46)
which attaches each element of the ADSR pane to the view object so we cna render and update it. That's it.
No onChange callback, no data streaming. Just create and attach a slider to a value.

And "why a slider"? Well why a slider because the object `sliders.A` is a `std::unique_ptr<Slider>`. Change
that to a `std::unqiue_ptr<Knob>` in the header and you would get knobs.

Types. Arent' they great!

## Building and Laying out a pane

So now its clear what to do. For each region of the UI, make sure it has access to the data which i sudpated
by the server, make your widgets and attachments, connect htme, and lay them out however you want. Just write
the friggin code. The ADSRPane is a good example.

## The shared memory cheater-bits

There's a few places where this model has a hard time scaling. They are

- Updates to the VU meters, since we have a lot of VU meters. A naive approach would send a message updating
  every block, or would have to know the ui refresh rate, or so on
- The sample waveform. We *coudl* stream this but its a lot of data and we are in process

So on these two, we cheat a bit, and the UI has a `const &` to a blob of shared memory.
If you are debugging or extending this, basically ask on discord.
