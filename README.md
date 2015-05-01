# Fireflies v2.07

This is an ancient project of mine that I decided to upload to github. I
haven't even compiled it in a while, but I suspect it would take a bit of work.

For information on how to compile, read COMPILE.

## Screenshots
![screenshot](http://mpcomplete.org/proj/fireflies/screenshot-1.jpg)
![screenshot](http://mpcomplete.org/proj/fireflies/screenshot-2.jpg)
![screenshot](http://mpcomplete.org/proj/fireflies/screenshot-3.jpg)
![screenshot](http://mpcomplete.org/proj/fireflies/screenshot-4.jpg)

## MODES:
New to 2.0 are modes.  There are two types, per-swarm modes, and major
modes.  Per-swarm modes affect a single swarm (but two swarms may decide to
switch to the same mode), while major modes affect the whole scene.  They
are activated with a certain probability, which is entirely relative.  For
example, if mode A has probability=1, and mode B has probability=5, that
means mode B is 5 times as likely to occur as mode A.  The mode numbers,
names, and descriptions are as follows:

Per-Swarm modes:
Normal mode (0) - normal swarm flight
Stop mode (1) - the bait comes to a halt, and the flies circle in around
	it.
Loop mode (2) - the bait travels in a loop, and the flies chase it.
Psychadelic mode (3) - color cycling speed for this swarm is increased, and
	the tails look like rainbows.
Glow mode (4) - the tails increase by glow_factor (an option).
Hyperspeed mode (5) - the bait and flies get hyper, and increase speed and
	acceleration.
Faded mode (6) - the colors fade

Major Modes:
All-Swarm mode (0) - activate a per-swarm mode for all swarms.
FlyKill mode (1) - kill a bunch of fireflies.
FlyBirth mode (2) - create a bunch of fireflies.
Windy mode (3) - wind speed increases.
Matrix mode (4) - Matrix-style pause and rotate.
Swarmsplit mode (5) - a random swarm is split in 2.
Swarmmerge mode (6) - 2 random swarms are merged.

## SDL standalone program:
If you have SDL, and you compiled fireflies with SDL support as a
standalone program (as opposed to a screensaver), you can enjoy a few neat
features (mostly there so I could test things, but what the hell, they're
fun for all).

Mouse input:
* Left-click and drag to move the camera left/right/up/down.
* Right-click and drag up/down to zoom the camera in/out.
* Hold control and Left-click and drag to rotate the camera about the origin.

Keyboard input:
* q = quit
* p = pause the screensaver (you can move the camera, but nothing else happens)
* t = display elapsed time in seconds to the console window (this won't
      work in windows if you don't have a console window open)
* up arrow = fast forward 2x
* down arrow = fast forward 1/2x
* right arrow = fast forward +1x
* left arrow = fast forward -1x
* 0-6 = activate major mode corresponding to that digit
* shift + 0-6 = activate swarm mode corresponding to that digit
* control + 0-6 = stop swarm mode corresponding to that digit
