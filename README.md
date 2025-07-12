# MIDIVerb_RE

Reverse-engineering the MIDIVerb audio effects processor

![](./img/MV1_top.png)

## Background

In 1986 [Alesis](https://en.wikipedia.org/wiki/Alesis) released the [Midiverb](https://www.vintagedigital.com.au/alesis-midiverb/), a low-cost audio effects processor that provided affordable DSP-based reverberation effects that had previously only been available at high cost to musicians and recording engineers. The secret to its success was the amazing engineering effort by [Keith Barr](https://valhalladsp.com/2010/08/25/rip-keith-barr/), founder of Alesis who had invested years of effort in understanding the technology and optimizing it for the chips and processes available at that time. Since then the state of the art has advanced well beyond what was possible in the 1980s and algorithmic reverbs have improved in cost and capability to the point that they're almost an afterthought in many audio processing tools. Despite this, it's still worthwhile studying how the original MIDIVerb worked to get an understanding of exactly what's possible with minimal DSP technology and also to emulate those algorithms for that retro sound so many people remember fondly.

## Reverse Engineering

In the spring of 2021, my colleague Paul Schreiber borrowed several original MIDIVerb units and sent me one for analysis with the goal of creating an emulation for use in a product we were supporting. We opened them up and began to analyze the circuit board, tracing signals and studying the circuits. Paul started a schematic diagram in Diptrace and sent me the first page with the sections containing the Intel 8031 microcontroller circuits. He had a business to run so I took over the remaining task of completing the schematic which entailed many hours of continuity tests, reading TTL datasheets and puzzling over the underlying intent of the logic. Once I had the schematic completed and the flow of logic organized in a way that made sense, I started the task actually **understanding** how it worked. This mean not only knowing the circuit, but also the DSP microcode for the 63 algorithms and how to convert that into the high-level language I was using for the firmware in our product. After some trial and error I had a simple emulator written as a command-line application running under Linux which could process ordinary .WAV files through the MIDIVerb algorithms and the goal was achieved. As part of this effort I'd also created a "disassembler" for the microcode that was helpful in understanding the signal flow of the individual algorithms.

## Videos

I spent some time preparing a [slide deck](./docs/MV_Slides.pdf) that gave a high-level overview of the MIDIVerb design and Paul produced several Youtube videos in which he presented the slides with commentary. Those videos can be found here:

[Synthesis Technology Patreon preview: Alesis MidiVerb I (History) - YouTube](https://www.youtube.com/watch?v=2yYiWOHwHSo)

[Reverse-Engineering the Alesis MidiVerb I: Where to start? - YouTube](https://www.youtube.com/watch?v=z4cIt1VPAjU)

[Synthesis Technology: MidiVerb 1 schematic analysis - YouTube](https://www.youtube.com/watch?v=JNPpU08YZjk)

[Synthesis Technology: Reverse Engineering the Alesis MidiVerb 1: DSP Software - YouTube](https://www.youtube.com/watch?v=5DYbirWuBaU)

## Schematics

The schematics consist of the digital and analog/power sections and can be found in the [schematics](./schematics) directory where they are available both in Diptrace .dch format as well as a [PDF file](./schematics/MIDIVerb_Schematic.pdf) combining both.
