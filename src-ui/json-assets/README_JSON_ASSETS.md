# JSON Layouts

Much of the SCXT ui is relatively fixed. Things like the
mapping pane and bus layout and stuff. But a few components
are typed and varied, currently the processors and bus
effects (but in the future a per-sample pane may be
the same, especiily if we add rendering algos).

To make it so we don't have to write buckets of
tedious C++ to lay these out, in July 24, Paul
hacked together a quick JSON parse to read files
per type and then lay out the widgets. At the
same time he wronte the first version of this
document, which is woefully incomplete as of
the first commit.

## Core Concepts - Mechanics

- Each 'screen' has a json file which lives in `src-ui/json-assets`
- Those json files have a common high level model but based on the
  type of thing they are mapping to, will have different features.
  For instance, voice processors have float and int parameters
  where as bus processors intermix them, so the specification
  means subtly different things.
- The JSON files are baked into the program using CMakeRC
  so they are part of the resulting DLL
- At development time, you can also load the files at each
  rebuild, allowing dynamic editing of the file without restarting
  the plugin
    - Method 1: Run ShortCircuit with the working directory as the
      root of the repo, and it will auto-detect the files
    - Method 2: Set the environment variabe `SCXT_LAYOUT_SOURCE_DIR`
      to the json-assets directory (so to `<scroot>/src-ui/json-assets`)
    - With either of these methods in place, editing the source and
      then rebuilding (so select a different zone and select back)
      for procs or a different bus and back for FX) will reload
      the JSON and re-layout

## Core Concepts - Syntax

THe JSON file is a top level object which contains two
sub objects, `defaults` and `components`. `components` is an
array of component specs and `defaults` provides the values
which all component specs adopt.

The `components` array lays out the on-screen components
mapped to indices or constructed explicitly and positions
them. The meanings of the innards of the compnents array
are somewhat different for voice and bus effects but they
share many features.

## A Component - Bus Effect

Document this after I do a few more and collect the result
and scan for consistency

## A Component - Voice Effect

Document this after I do a few more and collect the result
and scan for consistency

## Theme Color Maps

Pretty self explanatory