# GBA_Emulator-by-Ryzee119-

This was designed by /u/Ryzee
Originally posted on Reddit here:
https://www.reddit.com/r/DIY/comments/4zbqwr/this_is_my_attempt_at_making_a_portable_emulator/

Build Photos:
http://imgur.com/a/kjFmS

I am releasing these design files in good faith that people will use them to learn something about electronics and maybe bring an old Gameboy Advance (GBA) back to life.

I don’t want to see people taking this design and knocking it off for a quick buck.
If you don’t want to deal with all this work another redditor /u/Flav0r has designed something quite similar to this. http://gamepieadvance.com/ I understand he may be working on DIY Kits.

##DISCLAIMER:
This project is NOT for the fresh beginner or the faint of heart. It requires moderate/high level soldering skills. There are many small and fine pitch components that need to be soldered. I recommend you don’t attempt this unless you have experience in soldering, a basic ability to troubleshoot electronic circuits if things aren’t working and an ability to make mods on the fly to suit your needs. i.e cutting plastic where required.
This also requires taking apart an original GBA to repurpose the case, buttons and de-soldering of several components on the GBA motherboard. This can very tricky without the right equipment.
As a minimum the following tools are recommended (All parts should be of good quality from reputable suppliers!):

1.	A decent temperature controlled soldering station with suitable tips for fine work
2.	Solder flux
3.	Leaded solder
4.	Fine tweezers (anti-magnetic)
5.	Solder wick
6.	Multimeter for testing


This is a **FIRST PROTOTYPE** board which just happened to work reasonably ok. This has not undergone any extensive testing.

I take no responsibility for the following:

1.	You went ahead and purchased everything, soldered it up and it didn’t work.
2.	You soldered an SMD chip and bent and broke the pins.
3.	If a component in the BOM is not available or becomes obsolete, use your best judgement to find a replacement.
4.	A component was missing from the BOM. (I did check it. But it’s your responsibility to check it again against the schematic)
5.	The thing burst into flame and explodes in your hands and burns your house down.
6.	It doesn’t fit in your GBA case. (It was designed to one I had lying around. I don’t know if Nintendo did subtle changes in their cases over the years)
7.	Hours of lost productivity because you're slaying bosses and saving princesses.

Any advice and support I provide from this point is purely out of the kindness of my heart.

## ISSUES OBSERVED WITH THE BOARD:
Again, this is a first prototype. I have observed the following issues:

1. Slight buzzing from the speaker. More noticeable when volume is high and no sound is being played. I’ve added some extra decoupling caps to the area but this may or may not do a thing.

2. DC boost converter struggles at low input voltages. Although I tried to design it for this; if you use Alkaline batteries you will probably be disappointed. Once the input voltage drops below ~2.5V expect the random reboot at high loads. I recommend good quality Lithium AA batteries. Because of lithium discharge curves, the low battery indictor circuit is not particularly useful.

3. The closest screen I could find to the original is a 3.2" TFT. It is slightly bigger than the viewing window. The left and bottom edges of the screen are cut off slightly. Borders can be setup in RetroArch so the games don't have this issue, however it is visible in the game selection screens.

4. Probably more. This has not been extensively tested.

## Controls
So if you managed to get everything up and running. Congrats! If you followed the software setup guide it has the following key combos setup:

1. SELECT+START = Exit current rom
2. SELECT+A = Save current state
3. SELECT+B = Load current state
4. SELECT+RIGHT SHOULDER = High speed
5. SELECT+LEFT SHOULDER = Rewind gameplay around 3-4 seconds
6. SELECT+LEFT/RIGHT = INCREASE/DECREASE Screen brightness

