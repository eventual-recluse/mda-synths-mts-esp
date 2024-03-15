# MDA JX10 analog synth and DX10 FM synth in JUCE with basic MTS-ESP microtuning support
These plugins were adapted from the JUCE conversions of the JX10 and DX10 plugins by Matthijs Hollemans: [https://github.com/hollance/mda-plugins-juce/](https://github.com/hollance/mda-plugins-juce/). The plugins were originally written by Paul Kellett.

Basic [MTS-ESP microtuning](https://oddsound.com/usingmtsesp.php) support has been added to these two plugins. See [https://oddsound.com/usingmtsesp.php](https://oddsound.com/usingmtsesp.php) for more information about MTS-ESP.

## Building the plugins

Before building, the Source directories of each plugin must contain both libMTSClient.cpp and libMTSClient.h, which are available from the Client folder of the [ODDSound MTS-ESP library](https://github.com/ODDSound/MTS-ESP/tree/main/Client). The plugins can then be built with the Projucer from JUCE. The ODDSound MTS-ESP library can be found at [https://github.com/ODDSound/MTS-ESP/](https://github.com/ODDSound/MTS-ESP/).


Further information from the original README follows:

# MDA plug-ins in JUCE

This repo contains the [MDA freeware plug-ins](http://mda.smartelectronix.com) implemented in [JUCE](https://juce.com).

The [original plug-in source code](https://sourceforge.net/projects/mda-vst/) is for VST2, which is no longer supported. For my own elucidation and enjoyment, I decided to rewrite these plug-ins in JUCE.

The MDA plug-ins were originally written by Paul Kellett, about two decades ago. I also consulted the AU versions by [Sophia Poirier](http://destroyfx.org).

These source examples are provided mainly for **learning purposes**! As Paul said of his original source code release:

> This code is definitely not an example of how to write plug-ins! It's obvious that I didn't know much C++ when I started, and some of the optimizations might have worked on a 486 processor but are not relevant today.  The code is very raw with no niceties like parameter de-zipping, but maybe you'll find some useful stuff in there.

I've tried to fix some of these issues but did not add any new features such as parameter dezippering. The code here is 20 years old, so it may no longer be the most optimal way to implement these algorithms. Consider this project to be a kind of [plug-in archeology](https://audiodev.blog/plugin-archeology/). ;-)

That said, it's still **a good place to get started** if you're learning about audio DSP, which is why I added lots of comments to describe what's going on. If you have a [basic understanding of JUCE](https://www.youtube.com/c/TheAudioProgrammer), you should be able to follow along.

Some notes:

- The code has been cleaned up a bit and documented, and occasionally bug fixed.
- These plug-ins have no UI and use the default generic JUCE UI.
- I'm not using the standard JUCE coding style because it's ugly. ;-)
- The code has only been tested with Xcode on a Mac using JUCE 7, but should work on Windows too.

This source code is licensed under the [MIT license](LICENSE.txt) but keep in mind that JUCE has its own licensing terms. JUCE conversion done by Matthijs Hollemans.

You can find an archived copy of the [original documentation here](https://web.archive.org/web/20211028012511/http://mda.smartelectronix.com/vst/help/mdaplugs.htm).

> **Note:** The documentation of the parameters was taken from the original website but is not always complete or correct.

## How the code is structured

Read this section if you're new to JUCE or audio programming! Or if you're wondering why the code is structured the way it is...

Each plug-in is in its own folder. Go into the folder, open the **.jucer** file in Projucer and then launch your IDE from the Projucer project. Build the VST3 target and open it in JUCE's AudioPluginHost for testing.

Since there is no UI for these plug-ins, the only source files are **PluginProcessor.h** and **.cpp**.

### PluginProcessor

The main functions in PluginProcessor are:

- `createParameterLayout()`: this is where the parameters are defined
- `prepareToPlay()`: initializes the plug-in
- `resetState()`: sets things back to zero, clears out any delay lines, and so on. This method is called when the plug-in is initialized in `prepareToPlay` but also when the plug-in is `reset`.
- `update()`: takes the current parameter values and recomputes whatever depends on these parameters. This method is called from `processBlock` but only when parameters have actually changed.
- `processBlock()`: does the actual audio processing

The PluginProcessor has a number of member variables. Their names begin with an underscore, such as `_level`.

There are two types of member variables:

1. "cooked" parameters
2. rendering state

The cooked parameters are filled in by the `update()` method. This method is called by the audio thread at the start of `processBlock()`.  It reads the current parameter values from the APVTS and then puts the cooked version into the member variable. For example:

```c++
void MDARingModAudioProcessor::update()
{
    // Convert from decibels into a linear gain value.
    float level = apvts.getRawParameterValue("Level")->load();
    _level = juce::Decibels::decibelsToGain(level);
}
```

The second type of variable keeps track of rendering state. This is something like the current phase of an oscillator or the delay unit of a filter. These variables are given their initial value by `resetState()` and will be changed by `processBlock`.

Inside `processBlock()` we first read the member variables into local variables (for state) or constants (for parameters). Then the processing loop uses these local variables instead of the member variables. If the audio processing loop updates any of the state (which it usually does), the latest values get copied back into the corresponding member variables after the loop. I'm not sure how useful it is to copy the member variables into local variables, since both will be implemented as a load from a register using an offset, but this is how the original plug-ins did it.

### Parameter calculations

The MDA plug-ins were VST2, meaning that the parameters are always 0 - 1. Since JUCE allows us to have parameters in any range, they were changed to whatever felt more more natural (in dB, Hz, etc).

Let's say the plug-in defines an Output Level parameter. Like all VST2 parameters, this has a range of 0 - 1. But in the UI we want to display this as decibels. In the original code, the `getParameterDisplay()` function converted this 0 - 1 into a decibel value:

```c++
int2strng((VstInt32)(40.0 * fParam - 20.0), text);
```

Here, `fParam` is the 0 - 1 value. The formula used here will show a range from -20 dB (when `fParam = 0`) to +20 dB (when `fParam = 1`). Note that this `getParameterDisplay()` function is only used for drawing the UI, not for anything else.

In the audio processing code, the parameter is still 0 - 1. This value, which represents a range of decibels, is converted into a linear gain so that we can multiply it with the audio.

```c+++
g3 = (float)(pow(10.0, 2.0 * fParam - 1.0));
```

You may be wondering where this formula comes from. To go from decibels to a linear value, we do `pow(10, dB / 20)`. That is similar to what happens here, but not exactly the same. That's because our parameter isn't in decibels, it's 0 - 1. We first need to convert it to decibels:

```c++
  pow(10, dB / 20)
= pow(10, (40 * fParam - 20) / 20)
= pow(10, (40 * fParam / 20) - (20 / 20))
= pow(10, 2 * fParam - 1)
```

And that is indeed the formula the original plug-in used.

Fortunately, we don't need to do any of this. In the JUCE implementation, we can simply define the parameter to go from -20 to +20 dB. And then the `update()` function does:

```c++
float outputLevel = apvts.getRawParameterValue("Output")->load();  // -20 to +20 dB
g3 = juce::Decibels::decibelsToGain(outputLevel);
```

We can let JUCE handle the math, and we get to work with parameters that have a more natural range than 0 - 1.

However, sometimes it's a little more tricky. In the MDA Delay plug-in, the delay time in samples was calculated using the formula:

```c++
ldel = (VstInt32)(size * fParam0 * fParam0);
```

Here, `size` is the maximum length of the delay buffer in samples, and `fParam0` is a value between 0 and 1 as usual. Notice that `fParam0` gets squared, I'll explain why shortly.

The `getParameterDisplay()` for this parameter was defined as:

```c++
int2strng((VstInt32)(ldel * 1000.0f / getSampleRate()), text);
```

Rather than deriving this from `fParam0` directly, it takes the computed delay in samples `ldel`, and converts it to a time in milliseconds that is shown to the user.

In the JUCE version, I replaced this by a parameter that lets you directly choose the delay time in milliseconds, which seemed like a simpler approach. Instead of going from 0 - 1, the parameter goes to 0 to 500 ms. Makes sense, right?

However, recall that the audio processing logic squares the parameter value in the formula `size * fParam0 * fParam0`. Since that parameter goes from 0 - 1, this creates a nice little x^2 curve. This kind of thing is usually done to make it easier to pick smaller delays. For example, at a sample rate of 44100, the maximum delay length is 22050 samples. With the parameter set to 0.5, the delay is not 11025 (= half) but 5512 samples (= half squared). It makes the slider non-linear, which is what you want for things like times and frequencies.

But we are not working with a normalized value from 0 - 1 anymore, our parameter is in milliseconds already. Squaring that number doesn't make any sense.

Remember that squaring was only done to make the slider work more logarithmically, giving more room to smaller delays than to larger delays. In JUCE, we can achieve this by giving the slider a skew factor. Now our slider is non-linear too, just like in the original plug-in. But we don't have to do any of the math for it ourselves, JUCE takes care of this for us.

We can simply calculate the delay time linearly, without squaring the parameter:

```c++
const float samplesPerMsec = float(getSampleRate()) / 1000.0f;
float ldelParam = apvts.getRawParameterValue("L Delay")->load();  // 0 - 500 ms
ldel = int(ldelParam * samplesPerMsec);
```

### Denormals

Many plug-ins have code to flush denormals, for example like the following. This sets the variable to zero if the number becomes too small:

```c++
if (std::abs(f) > 1.0e-10f) {
    _filter = f;
} else {
    _filter = 0.0f;
}
```

I left this code in the plug-in even though it is not strictly necessary, as we use `juce::ScopedNoDenormals` to automatically flush denormals to zero.
