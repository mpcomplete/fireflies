#include "main.h"
#include "scene.h"

#include "canvas_base.h"
#ifdef HAVE_SDL
#include "canvas_sdl.h"
#endif
#ifdef HAVE_GLX
#include "canvas_glx.h"
#endif

#include <iostream>
#include <stdlib.h>

#ifdef WIN32
#include <windows.h>
#else
#include <argp.h>
#endif

CanvasBase *canvas;
Scene scene;

enum {CANVAS_SDL, CANVAS_GLX} canvas_type = CANVAS_SDL;
int window_id = 0;
int mspf = 1000/30;
bool full_screen = false;

#ifdef WIN32
// mingw doesn't have argp. implement half-assed version

#define OPTION_HIDDEN 1
#define OPTION_DOC 2

#define ARGP_KEY_ARG 0
#define ARGP_ERR_UNKNOWN 0

#define ARGP_LONG_ONLY 1

typedef int (*argp_parser_t)(int, char*, struct argp_state*);

struct argp_option {
    const char *name;
    int key;
    const char *arg;
    int flags;
    const char *doc;
    int group;
};

struct argp {
    const struct argp_option *options;
    argp_parser_t parser;
    const char *args_doc;
    const char *doc;
    // there are more members, but this is all I need
};

struct argp_state {
    int argc;
    char **argv;
    int next;
    char *name;
    // there are more members, but this is all I need
};

void argp_help(struct argp *argp_s)
{
    cerr << argp_s->doc << endl;

    for (int i = 0; argp_s->options[i].name; i++) {
	if (argp_s->options[i].flags & OPTION_HIDDEN)
	    continue;
	if (argp_s->options[i].flags & OPTION_DOC)
	    cerr << argp_s->options[i].name << endl;
	else {
	    if (isprint(argp_s->options[i].key))
		cerr << "  -" << (char)argp_s->options[i].key << ",";
	    else
		cerr << "     ";
	    cerr << " --" << argp_s->options[i].name << "\t";
	    cerr << argp_s->options[i].doc << endl;
	}
    }
}

// doesn't work exactly as real argp, but suits this program
int argp_parse(struct argp *argp_s, int argc, char *argv[], int, int *, void *)
{
    char *opt, *arg=0;
    int optind = 0;

    struct argp_state state;
    state.argc = argc;
    state.argv = argv;
    state.name = argv[0];

    for (int i = 1; i < state.argc; i = state.next) {
	opt = state.argv[i];
	arg = 0;
	optind = -1;
	state.next = i+1;

	if (opt[0] != '-') {
	    if ((*argp_s->parser)(ARGP_KEY_ARG, 0, &state) < 0)
		return -1;
	    continue;
	}
	opt++;
	if (*opt == '-')
	    opt++;

	if ((arg = strchr(opt, '=')))
	    *arg++ = 0;

	if (opt[1] == 0) { // short option
	    for (int j = 0; argp_s->options[j].name; j++) {
		if (opt[0] == argp_s->options[j].key) {
		    optind = j;
		    break;
		}
	    }
	    if (optind >= 0)
		;
	    else if (opt[0] == 'h') {
		argp_help(argp_s);
		return -1;
	    }
	    else if (opt[0] == 'u') {
		argp_help(argp_s);
		return -1;
	    }
	    else if (opt[0] == 'V') {
		extern char *argp_program_version;
		cerr << argp_program_version << endl;
		return -1;
	    }
	}
	else { // long option
	    for (int j = 0; argp_s->options[j].name; j++) {
		if (strcmp(opt, argp_s->options[j].name)==0) {
		    optind = j;
		    break;
		}
	    }
	    if (optind >= 0)
		;
	    else if (strcmp(opt, "help") == 0) {
		argp_help(argp_s);
		return -1;
	    }
	    else if (strcmp(opt, "usage") == 0) {
		argp_help(argp_s);
		return -1;
	    }
	    else if (strcmp(opt, "version") == 0) {
		extern char *argp_program_version;
		cerr << argp_program_version << endl;
		return -1;
	    }
	}
	if (optind < 0) { // doesn't exist
	    cerr << state.name << ": unrecognized option `"
		 << state.argv[i] << "'" << endl;
	    cerr << "Try `" << state.name << " --help' or `"
		 << state.name << " --usage' for more information" << endl;
	    return -1;
	}
	if (argp_s->options[optind].arg != 0 && arg == 0) {
	    state.next = i+2;
	    if (i+1 >= state.argc) {
		cerr << state.name << ": option requires an argument -- "
		    << opt << endl;
		return -1;
	    }
	    arg = state.argv[i+1];
	}
	if ((*argp_s->parser)(argp_s->options[optind].key, arg, &state) < 0)
	    return -1;
    }
    return 0;
}

#endif

#define OPT_FULLSCREEN 1
#define OPT_FPS 2
#define OPT_FASTFORWARD 3

char *mode_help =
"\n"
"Per-swarm modes and their default probabilities:\n"
"  1: normal                                         p=20\n"
"  2: stop (bait stays still for a brief period)     p=10\n"
"  3: circle (bait loops in a circle briefly)        p=10\n"
"  4: rainbow (color cycle speeds up)                p=10\n"
"  5: glow (tails glow as their width increases)     p=15\n"
"  6: hyperspeed (swarm speeds up)                   p=10\n"
"  7: faded (colors fade)                            p=10\n"
"Major modes and their default probabilities:\n"
"  1: all swarms pick a random per-swarm mode        p=5\n"
"  2: kill some flies                                p=10\n"
"  3: make some new flies                            p=10\n"
"  4: wind speed picks up                            p=10\n"
"  5: matrix-style pause and rotate of the scene     p=10\n"
"  6: split a random swarm into two swarms           p=10\n"
"  7: merge two random swarms into one               p=10\n"
"Note: mode probabilities are relative. For example, if A has p=5, and B has\n"
"p=1, that simply means A is 5 times more likely to occur than B.";

const char *argp_program_version =
"Fireflies " PACKAGE_VERSION " by Mattperry <mpcomplete@gmail.com>";

static char doc[] =
"Moving swarms of delicious eye candy.";

static struct argp_option options[] = {
{"window-id",	'W', "WinID", OPTION_HIDDEN},
{"root",	'r', 0, 0, "Draw on the root window"},

{"fullscreen",	OPT_FULLSCREEN, 0, 0, "Full screen mode"},
{"fps",		OPT_FPS, "NUM", 0, "Frames per second (default = 30 fps)"},
{"fastforward", OPT_FASTFORWARD, "NUM", 0, "Fast forward factor (default = 1)"},
{"minbaits",	'b', "NUM", 0, "Minimum baits (default = 2)"},
{"maxbaits",	'B', "NUM", 0, "Maximum baits (default = 5)"},
{"minflies",	'f', "NUM", 0, "Minimum total fireflies (default = 100)"},
{"maxflies",	'F', "NUM", 0, "Maximum total fireflies (default = 175)"},
{"size",	's', "NUM", 0, "Firefly size (default = 15)"},
{"bspeed",	'V', "NUM", 0, "Bait speed (default = 50)"},
{"baccel",	'A', "NUM", 0, "Bait acceleration (default = 600)"},
{"fspeed",	'v', "NUM", 0, "Firefly speed (default = 100)"},
{"faccel",	'a', "NUM", 0, "Firefly acceleration (default = 300)"},
{"colorspeed",	'c', "NUM", 0, "Swarm's color cycling speed (default = 15)"},
{"taillength",	't', "NUM", 0, "Firefly's tail length (default = 23)"},
{"tailwidth",	'T', "NUM", 0, "Firefly's tail width (default = 25)"},
{"tailopacity",	'o', "NUM", 0,
    "Firefly's tail opacity/brightness ([0-100] default = 60)"},
{"glowfactor",	'g', "NUM", 0,
    "Factor by which tailwidth increases during glow (default = 20)"},
{"wind",	'w', "NUM", 0, "Wind speed (default = 30)"},
{"drawbait",	'd', 0, 0, "Draw the baits that the fireflies chase"},
{"modeswarm",	'm', "MODENUM VAL", 0,
    "Change the frequency of per-swarm mode MODENUM to VAL"},
{"modemajor",	'M', "MODENUM VAL", 0,
    "Change the frequency of major mode MODENUM to VAL"},
{mode_help, 0, 0, OPTION_DOC, 0, -1},
{0, 0, 0, 0}
};

int parse_opt(int key, char *arg, struct argp_state *state)
{ 
    switch (key) {
	case 'W':
	    canvas_type = CANVAS_GLX;
	    window_id = strtol(arg, 0, 0);
	    break;
	case 'r':
	    canvas_type = CANVAS_GLX;
	    break;
	case OPT_FULLSCREEN:
	    full_screen = true;
	    break;
	case OPT_FPS:
	    mspf = 1000/atoi(arg);
	    break;
	case OPT_FASTFORWARD:
	    scene.fast_forward = atoi(arg);
	    if (scene.fast_forward == 0) {
		cerr << state->name
		     << ": -fastforward must be > 0" << endl;
		return -1;
	    }
	    break;
	case 'b':
	    scene.minbaits = (unsigned)atoi(arg);
	    break;
	case 'B':
	    scene.maxbaits = (unsigned)atoi(arg);
	    break;
	case 'f':
	    scene.minflies = (unsigned)atoi(arg);
	    break;
	case 'F':
	    scene.maxflies = (unsigned)atoi(arg);
	    break;
	case 's':
	    scene.fsize = atoi(arg)/10.0;
	    break;
	case 'V':
	    scene.bspeed = atoi(arg);
	    break;
	case 'A':
	    scene.baccel = atoi(arg);
	    break;
	case 'v':
	    scene.fspeed = atoi(arg);
	    break;
	case 'a':
	    scene.faccel = atoi(arg);
	    break;
	case 'c':
	    scene.hue_rate = atoi(arg);
	    break;
	case 't':
	    scene.tail_length = atoi(arg)/10.0;
	    break;
	case 'T':
	    scene.tail_width = atoi(arg)/10.0;
	    break;
	case 'o':
	    scene.tail_opaq = atoi(arg)/100.0;
	    if (scene.tail_opaq < 0 || scene.tail_opaq > 1) {
		cerr << state->name
		     << ": -tailopacity must be in the range [0,100]" << endl;
		return -1;
	    }
	    break;
	case 'g':
	    scene.glow_factor = atoi(arg)/10.0;
	    break;
	case 'w':
	    scene.wind_speed = atoi(arg)/10.0;
	    break;
	case 'd':
	    scene.draw_bait = true;
	    break;
	case 'm': {
	    int which = atoi(arg);
	    unsigned val;
	    if (state->next < state->argc) {
		val = (unsigned)atoi(state->argv[state->next]);
		state->next++;
	    }
	    else {
		cerr << state->name << ": option -modebait requires 2 arguments" << endl;
		return -1;
	    }
	    scene.bmodes.change(which, (double)val);
	    break;
		  }
	case 'M': {
	    int which = atoi(arg);
	    unsigned val;
	    if (state->next < state->argc) {
		val = (unsigned)atoi(state->argv[state->next]);
		state->next++;
	    }
	    else {
		cerr << state->name << ": option -modebait requires 2 arguments" << endl;
		return -1;
	    }
	    scene.smodes.change(which, (double)val);
	    break;
		  }
	default:
	    return ARGP_ERR_UNKNOWN;
	    break;
    }
    return 0;
}

static struct argp argp_s = { options, parse_opt, 0, doc };

int main(int argc, char **argv)
{
    if (argp_parse (&argp_s, argc, argv, ARGP_LONG_ONLY, 0, 0) != 0)
	return 0;

    switch (canvas_type) {
    case CANVAS_GLX:
#ifdef HAVE_GLX
	canvas = new CanvasGLX(&scene, full_screen, mspf, window_id);
#else
	cerr << argv[0] << ": cannot make GLX window (you must have GLX support enabled)" << endl;
	return 1;
#endif
	break;
    case CANVAS_SDL:
#ifdef HAVE_SDL
	canvas = new CanvasSDL(&scene, full_screen, mspf, "Fireflies", "fireflies");
#else
	cerr << argv[0] << ": cannot make SDL window (you must have SDL support enabled)" << endl;
	return 1;
#endif
	break;
    }

    if (canvas->init() < 0) {
	cerr << "Can't init display." << endl;
	return 0;
    }

    scene.create();

    return canvas->loop();
}
