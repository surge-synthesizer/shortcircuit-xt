# SST Project 3

Under development. You know the drill

```
git submodule update --init --recursive
cmake -Bbuild
cmake --build build --config Release 
```

or debug if you want

Right now the standalone hardcodes a path to a sample. Edit wrappers\juce\SC3PluginProcessor.cpp and look for the string with a y: and change that and you will
get horrible harpsi action

