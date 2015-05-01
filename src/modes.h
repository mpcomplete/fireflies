#ifndef _MODES_H
#define _MODES_H

#define SMODE_NOTHING		0
#define SMODE_SWARMS		1
#define SMODE_FLYKILL		2
#define SMODE_FLYBIRTH		3
#define SMODE_WINDY		4
#define SMODE_MATRIX		5
#define SMODE_SWARMSPLIT	6
#define SMODE_SWARMMERGE	7
#define NUM_SMODES		8

#define BMODE_NOTHING		0
#define BMODE_NORMAL		1
#define BMODE_STOP		2
#define BMODE_ATTRACTOR		3
#define BMODE_RAINBOW		4
#define BMODE_GLOW		5
#define BMODE_HYPERSPEED	6
#define BMODE_FADED		7
#define NUM_BMODES 		8

class Bait;

// force b to start behaving in manner described by "mode"
void bait_start_mode(Bait *b, int mode);
// cancel effects of mode "mode" - may cancel other modes too
void bait_stop_mode(Bait *b, int mode);

// activate the scene mode "mode"
void scene_start_mode(int mode);

#endif // _MODES_H
