# IRQGuard
This repository contains the implementation of the IRQGuard framework discussed 
in the research paper "No Leakage Without State Change: Repurposing Configurable CPU Exceptions to Prevent Microarchitectural Attacks" (ACSAC 2024). TODO: paper link

The framework is designed to detect and prevent ongoing microarchitectural side-channel attacks.

## Supported Platforms
The PoC implementation supports only Intel x86 processors with at least performance monitoring version 4.
IRQGuard is developed and tested on Ubuntu and currently tested up to Linux kernel 5.15.0. 

## Building
Follow the instructions of `./irqguard`.

## Contact
If there are questions regarding this tool, please send an email to `daniel.weber (AT) cispa.saarland` or message `@weber_daniel` on Twitter.

## Research Paper
The paper is available at TODO.
You can cite our work with the following BibTeX entry:
TODO

## Disclaimer
We are providing this code as-is. 
You are responsible for protecting yourself, your property and data, and others from any risks caused by this code. 
This code may cause unexpected and undesirable behavior to occur on your machine. 
