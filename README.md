# pyranode
*Photodiode and Teensy 3.0 pyranometer for measuring solar irradiance*

The conventional wisdom for using a photodiode with an ADC is that the photodiode needs to be properly biased and its output needs to be conditioned for the capabilites of the ADC. Typically, this means one or more opamps and a handful of discrete components in a bespoke circuit. That's a bit of a barrier to entry for a software engineer learning about electronics. Turns out no extra components are needed if one's needs are modest, such as measuring slowly varying sunlight falling on the photodiode.

The photodiode is wired directly from one pin of a [Teensy 3.0](https://www.pjrc.com/store/teensy3.html) to another. One pin drives a logic high 3.3V. The other pin has a weak pulldown resistor activated within the microcontroller. The ADC is run at its slowest setting and a median filter is used to reduce noise. The photodiode I used was sold without a datasheet. I'm not even sure whether the part is a photodiode or phototransistor with two leads.

The instrument I made is uncalibrated. Its intended purpose is to give me a second source to compare to the output of my photovoltaic system to alert me to outages and to help me understand aging effects. The secondary purpose is to help me learn about IoT, networking, and automation.

The unit is mounted in skylight and connected via USB Serial to a [TP-LINK MR3020 running OpenWrt](https://wiki.openwrt.org/toh/tp-link/tl-mr3020). Once per minute, my server ssh's into the OpenWrt box, runs a command to grab a batch of readings from the Teesy, and logs it to a file. The unit is sensitive enough to detect when the room lights are on. (The mounting location isn't ideal, but this was helpful during testing and was much easier than mounting it outdoors.)
