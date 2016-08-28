/*
This Program has been modified by Ryzee119 for use with a GBA mod design by me.
https://github.com/Ryzee119/GBA_Emulator-by-Ryzee119-
http://imgur.com/a/kjFmS

All credits to ADAFRUIT for developing this.
Originally pulled from https://github.com/adafruit/Adafruit-Retrogame

ADAFRUIT RETROGAME UTILITY: remaps buttons on Raspberry Pi GPIO header
to virtual USB keyboard presses.  Great for classic game emulators!
Retrogame is interrupt-driven and efficient (usually under 0.3% CPU use)
and debounces inputs for glitch-free gaming.

Connect one side of button(s) to GND pin (there are several on the GPIO
header, but see later notes) and the other side to GPIO pin of interest.
Internal pullups are used; no resistors required.  Avoid pins 8 and 10;
these are configured as a serial port by default on most systems (this
can be disabled but takes some doing).  Pin configuration is currently
set in global table; no config file yet.  See later comments.

Must be run as root, i.e. 'sudo ./retrogame &' or configure init scripts
to launch automatically at system startup.

Requires uinput kernel module.  This is typically present on popular
Raspberry Pi Linux distributions but not enabled on some older varieties.
To enable, either type:

    sudo modprobe uinput

Or, to make this persistent between reboots, add a line to /etc/modules:

    uinput

Prior versions of this code, when being compiled for use with the Cupcade
or PiGRRL projects, required CUPCADE to be #defined.  This is no longer
the case; instead a test is performed to see if a PiTFT is connected, and
one of two I/O tables is automatically selected.

Written by Phil Burgess for Adafruit Industries, distributed under BSD
License.  Adafruit invests time and resources providing this open source
code, please support Adafruit and open-source hardware by purchasing
products from Adafruit!


Copyright (c) 2013 Adafruit Industries.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <dirent.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/signalfd.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <wiringPi.h>
#include <softPwm.h>


// START HERE ------------------------------------------------------------
// This table remaps GPIO inputs to keyboard values.  In this initial
// implementation there's a 1:1 relationship (can't attach multiple keys
// to a button) and the list is fixed in code; there is no configuration
// file.  Buttons physically connect between GPIO pins and ground.  There
// are only a few GND pins on the GPIO header, so a breakout board is
// often needed.  If you require just a couple extra ground connections
// and have unused GPIO pins, set the corresponding key value to GND to
// create a spare ground point.

#define GND -1
struct {
	int pin;
	int key;
} *io, // In main() this pointer is set to one of the two tables below.
   ioGBA[] = {
	// Input	Output (from /usr/include/linux/input.h)
	{   27,     KEY_LEFT     	}, 
	{   22,     KEY_RIGHT    	},
	{   17,     KEY_DOWN     	},
	{    5,     KEY_UP       	},
	{   19,     KEY_Z        	},  // A
	{   26,     KEY_X        	},  // B
	{   24,     KEY_RIGHTSHIFT 	},  // Select
	{   23,     KEY_ENTER    	},  // Start
	{    6,	    KEY_O			},  // Left Bumper
	{   21,	    KEY_P	 		},  // Right Bumper
	{   -1,     -1          	}}; // END OF LIST, DO NOT CHANGE
	// MAME must be configured with 'z' & 'x' as buttons 1 & 2 -
	// this was required for the accompanying 'menu' utility to
	// work (catching crtl/alt w/ncurses gets totally NASTY).


// A "Vulcan nerve pinch" (holding down a specific button combination
// for a few seconds) issues an 'esc' keypress to MAME (which brings up
// an exit menu or quits the current game).  The button combo is
// configured with a bitmask corresponding to elements in the above io[]
// array.  The default value here uses elements 6 and 7 (credit and start
// in the Cupcade pinout).  If you change this, make certain it's a combo
// that's not likely to occur during actual gameplay (i.e. avoid using
// joystick directions or hold-for-rapid-fire buttons).
// Also key auto-repeat times are set here.  This is for navigating the
// game menu using the 'gamera' utility; MAME disregards key repeat
// events (as it should).
const unsigned long vulcanMaskA = (1L << 6) | (1L << 0), //Select + Left
					vulcanMaskB = (1L << 6) | (1L << 1); //Select + Right
const int           vulcanKey  = KEY_ESC, // Keycode to send
                    vulcanTimeA = 21,    // Pinch time in milliseconds
					vulcanTimeB = 22,    // Pinch time in milliseconds
                    repTime1   = 200,     // Key hold time to begin repeat
                    repTime2   = 100;     // Time between key repetitions


// A few globals ---------------------------------------------------------

extern char
  *__progname,                       // Program name (for error reporting)
  *program_invocation_name;          // Full name as invoked (path, etc.)
char
   sysfs_root[] = "/sys/class/gpio", // Location of Sysfs GPIO files
   running      = 1;                 // Signal handler will set to 0 (exit)
volatile unsigned int
  *gpio;                             // GPIO register table
const int
   debounceTime = 20;                // 20 ms for button debouncing
int brightness = 50;				 // Brightness% of TFT. Initial value is 80%
const int bright_interval = 10;		 // Amount brightness changes each press.

// Some utility functions ------------------------------------------------

// Set one GPIO pin attribute through the Sysfs interface.
int pinConfig(int pin, char *attr, char *value) {
	char filename[50];
	int  fd, w, len = strlen(value);
	sprintf(filename, "%s/gpio%d/%s", sysfs_root, pin, attr);
	if((fd = open(filename, O_WRONLY)) < 0) return -1;
	w = write(fd, value, len);
	close(fd);
	return (w != len); // 0 = success
}

// Un-export any Sysfs pins used; don't leave filesystem cruft.  Also
// restores any GND pins to inputs.  Write errors are ignored as pins
// may be in a partially-initialized state.
void cleanup() {
	char buf[50];
	int  fd, i;
	sprintf(buf, "%s/unexport", sysfs_root);
	if((fd = open(buf, O_WRONLY)) >= 0) {
		for(i=0; io[i].pin >= 0; i++) {
			// Restore GND items to inputs
			if(io[i].key == GND)
				pinConfig(io[i].pin, "direction", "in");
			// And un-export all items regardless
			sprintf(buf, "%d", io[i].pin);
			write(fd, buf, strlen(buf));
		}
		close(fd);
	}
}

// Quick-n-dirty error reporter; print message, clean up and exit.
void err(char *msg) {
	printf("%s: %s.  Try 'sudo %s'.\n", __progname, msg,
	  program_invocation_name);
	cleanup();
	exit(1);
}

// Detect Pi board type.  Doesn't return super-granular details,
// just the most basic distinction needed for GPIO compatibility:
// 0: Pi 1 Model B revision 1
// 1: Pi 1 Model B revision 2, Model A, Model B+, Model A+
// 2: Pi 2 Model B

static int boardType(void) {
	FILE *fp;
	char  buf[1024], *ptr;
	int   n, board = 1; // Assume Pi1 Rev2 by default

	// Relies on info in /proc/cmdline.  If this becomes unreliable
	// in the future, alt code below uses /proc/cpuinfo if any better.
#if 1
	if((fp = fopen("/proc/cmdline", "r"))) {
		while(fgets(buf, sizeof(buf), fp)) {
			if((ptr = strstr(buf, "mem_size=")) &&
			   (sscanf(&ptr[9], "%x", &n) == 1) &&
			   (n == 0x3F000000)) {
				board = 2; // Appears to be a Pi 2
				break;
			} else if((ptr = strstr(buf, "boardrev=")) &&
			          (sscanf(&ptr[9], "%x", &n) == 1) &&
			          ((n == 0x02) || (n == 0x03))) {
				board = 0; // Appears to be an early Pi
				break;
			}
		}
		fclose(fp);
	}
#else
	char s[8];
	if((fp = fopen("/proc/cpuinfo", "r"))) {
		while(fgets(buf, sizeof(buf), fp)) {
			if((ptr = strstr(buf, "Hardware")) &&
			   (sscanf(&ptr[8], " : %7s", s) == 1) &&
			   (!strcmp(s, "BCM2709"))) {
				board = 2; // Appears to be a Pi 2
				break;
			} else if((ptr = strstr(buf, "Revision")) &&
			          (sscanf(&ptr[8], " : %x", &n) == 1) &&
			          ((n == 0x02) || (n == 0x03))) {
				board = 0; // Appears to be an early Pi
				break;
			}
		}
		fclose(fp);
	}
#endif

	return board;
}

// Filter function for scandir(), identifies possible device candidates
// for simulated keypress events (separate from actual USB keyboard(s)).
static int filter1(const struct dirent *d) {
	if(!strncmp(d->d_name, "input", 5)) { // Name usu. 'input' + #
		// Read contents of 'name' file inside this subdirectory,
		// if it matches the retrogame executable, that's probably
		// the device we want...
		char  filename[100], line[100];
		FILE *fp;
		sprintf(filename, "/sys/devices/virtual/input/%s/name",
		  d->d_name);
		memset(line, 0, sizeof(line));
		if((fp = fopen(filename, "r"))) {
			fgets(line, sizeof(line), fp);
			fclose(fp);
		}
		if(!strncmp(line, __progname, strlen(__progname))) return 1;
	}

        return 0;
}

// A second scandir() filter, checks for filename of 'event' + #
static int filter2(const struct dirent *d) {
	return !strncmp(d->d_name, "event", 5);
}

// Main stuff ------------------------------------------------------------

#define PI1_BCM2708_PERI_BASE 0x20000000
#define PI1_GPIO_BASE         (PI1_BCM2708_PERI_BASE + 0x200000)
#define PI2_BCM2708_PERI_BASE 0x3F000000
#define PI2_GPIO_BASE         (PI2_BCM2708_PERI_BASE + 0x200000)
#define BLOCK_SIZE            (4*1024)
#define GPPUD                 (0x94 / 4)
#define GPPUDCLK0             (0x98 / 4)

int main(int argc, char *argv[]) {

	// A few arrays here are declared with 32 elements, even though
	// values aren't needed for io[] members where the 'key' value is
	// GND.  This simplifies the code a bit -- no need for mallocs and
	// tests to create these arrays -- but may waste a handful of
	// bytes for any declared GNDs.
	char                   buf[50],      // For sundry filenames
	                       c,            // Pin input value ('0'/'1')
	                       board;        // 0=Pi1Rev1, 1=Pi1Rev2, 2=Pi2
	int                    fd, fd2,      // For mmap, sysfs, uinput
	                       i, j, j3,     // Asst. counter
	                       bitmask,      // Pullup enable bitmask
	                       timeout = -1, // poll() timeout
	                       intstate[32], // Last-read state
	                       extstate[32], // Debounced state
	                       lastKey = -1; // Last key down (for repeat)
	unsigned long          bitMask, bit; // For Vulcan pinch detect
	volatile unsigned char shortWait;    // Delay counter
	struct input_event     keyEv, synEv; // uinput events
	struct pollfd          p[35];        // File descriptors for poll()
	sigset_t               sigset;       // Signal mask
	wiringPiSetup();
	softPwmCreate(7,brightness,100);
	//printf("PWM created for pin 7\n");
	// Catch all signal types so GPIO cleanup on exit is possible
	sigfillset(&sigset);
	sigprocmask(SIG_BLOCK, &sigset, NULL);
	// pollfd #0 is for catching signals...
	p[0].fd      = signalfd(-1, &sigset, 0);
	p[0].events  = POLLIN | POLLERR | POLLHUP;
	p[0].revents = 0;

	// pollfd #1 and #2 will be used for detecting changes in the
	// config file and its parent directory.  Config files ARE NOT
	// YET IMPLEMENTED, but I'm working toward this incrementally.
	// This change detection will let you edit the config and have
	// immediate feedback without needing to kill the process or
	// reboot the system.
	p[1].fd     = -1;
	p[1].events =  0;
	p[2].fd     = -1;
	p[2].events =  0;

	// p[0-2] NEVER CHANGE for the remainder of the application's
	// lifetime.  p[3...n] are then related to GPIO states, and will
	// be reconfigured each time the config file is loaded (to-do).

	// Select io[] table.
	io = ioGBA;

	// If this is a "Revision 1" Pi board (no mounting holes),
	// remap certain pin numbers in the io[] array for compatibility.
	// This way the code doesn't need modification for old boards.
	board = boardType();
	if(board == 0) {
		for(i=0; io[i].pin >= 0; i++) {
			if(     io[i].pin ==  2) io[i].pin = 0;
			else if(io[i].pin ==  3) io[i].pin = 1;
			else if(io[i].pin == 27) io[i].pin = 21;
		}
	}

	// ----------------------------------------------------------------
	// Although Sysfs provides solid GPIO interrupt handling, there's
	// no interface to the internal pull-up resistors (this is by
	// design, being a hardware-dependent feature).  It's necessary to
	// grapple with the GPIO configuration registers directly to enable
	// the pull-ups.  Based on GPIO example code by Dom and Gert van
	// Loo on elinux.org

	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0)
		err("Can't open /dev/mem");
	gpio = mmap(            // Memory-mapped I/O
	  NULL,                 // Any adddress will do
	  BLOCK_SIZE,           // Mapped block length
	  PROT_READ|PROT_WRITE, // Enable read+write
	  MAP_SHARED,           // Shared with other processes
	  fd,                   // File to map
	  (board == 2) ?
	   PI2_GPIO_BASE :      // -> GPIO registers
	   PI1_GPIO_BASE);

	close(fd);              // Not needed after mmap()
	if(gpio == MAP_FAILED) err("Can't mmap()");
	// Make combined bitmap of pullup-enabled pins:
	for(bitmask=i=0; io[i].pin >= 0; i++)
		if(io[i].key != GND) bitmask |= (1 << io[i].pin);
	gpio[GPPUD]     = 2;                    // Enable pullup
	for(shortWait=150;--shortWait;);        // Min 150 cycle wait
	gpio[GPPUDCLK0] = bitmask;              // Set pullup mask
	for(shortWait=150;--shortWait;);        // Wait again
	gpio[GPPUD]     = 0;                    // Reset pullup registers
	gpio[GPPUDCLK0] = 0;
	(void)munmap((void *)gpio, BLOCK_SIZE); // Done with GPIO mmap()

	// ----------------------------------------------------------------
	// All other GPIO config is handled through the sysfs interface.

	sprintf(buf, "%s/export", sysfs_root);
	if((fd = open(buf, O_WRONLY)) < 0) // Open Sysfs export file
		err("Can't open GPIO export file");
	for(i=0,j=3; io[i].pin >= 0; i++) { // For each pin of interest...
		sprintf(buf, "%d", io[i].pin);
		write(fd, buf, strlen(buf));             // Export pin
		pinConfig(io[i].pin, "active_low", "0"); // Don't invert
		if(io[i].key == GND) {
			// Set pin to output, value 0 (ground)
			if(pinConfig(io[i].pin, "direction", "out") ||
			   pinConfig(io[i].pin, "value"    , "0"))
				err("Pin config failed (GND)");
		} else {
			// Set pin to input, detect rise+fall events
			if(pinConfig(io[i].pin, "direction", "in") ||
			   pinConfig(io[i].pin, "edge"     , "both"))
				err("Pin config failed");
			// Get initial pin value
			sprintf(buf, "%s/gpio%d/value",
			  sysfs_root, io[i].pin);
			// The p[] file descriptor array isn't necessarily
			// aligned with the io[] array.  GND keys in the
			// latter are skipped, but p[] requires contiguous
			// entries for poll().  So the pins to monitor are
			// at the head of p[], and there may be unused
			// elements at the end for each GND.  Same applies
			// to the intstate[] and extstate[] arrays.
			if((p[j].fd = open(buf, O_RDONLY)) < 0)
				err("Can't access pin value");
			j3 = j - 3; // Signal fds occupy first 3 slots
			intstate[j3] = 0;
			if((read(p[j].fd, &c, 1) == 1) && (c == '0'))
				intstate[j3] = 1;
			extstate[j3] = intstate[j3];
			p[j].events  = POLLPRI; // Set up poll() events
			p[j].revents = 0;
			j++;
		}
	} // 'j' is now count of non-GND items in io[] table + 3 fds
	close(fd); // Done exporting


	// ----------------------------------------------------------------
	// Set up uinput

	// Traditionally we've used /dev/uinput for generating key events,
	// but see notes below....
	if((fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK)) < 0)
		err("Can't open /dev/uinput");
	if(ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0)
		err("Can't SET_EVBIT");
	for(i=0; io[i].pin >= 0; i++) {
		if(io[i].key != GND) {
			if(ioctl(fd, UI_SET_KEYBIT, io[i].key) < 0)
				err("Can't SET_KEYBIT");
		}
	}
	if(ioctl(fd, UI_SET_KEYBIT, vulcanKey) < 0) err("Can't SET_KEYBIT");
	struct uinput_user_dev uidev;
	memset(&uidev, 0, sizeof(uidev));
	snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "retrogame");
	uidev.id.bustype = BUS_USB;
	uidev.id.vendor  = 0x1;
	uidev.id.product = 0x1;
	uidev.id.version = 1;
	if(write(fd, &uidev, sizeof(uidev)) < 0)
		err("write failed");
	if(ioctl(fd, UI_DEV_CREATE) < 0)
		err("DEV_CREATE failed");

	// SDL2 (used by some newer emulators) wants /dev/input/eventX
	// instead -- BUT -- this is the odd thing, and I don't fully
	// understand the why -- eventX only exists if a physical USB
	// keyboard has been connected, OR IF /dev/uinput in the code
	// above has been opened and set up.  Skip the prior steps and
	// it won't happen (this wasn't necessary in earlier versions,
	// could just go right to this step for SDL2, so not sure what's
	// up).  Since we might be running on an earlier system, we'll
	// start with uinput by default, then override the value of fd
	// only *if* eventX exists, should cover both cases.

	// The 'X' in eventX is a unique identifier (typically a numeric
	// digit or two) for each input device, dynamically assigned as
	// USB input devices are plugged in or disconnected (or when the
	// above code in retrogame runs).  As it's dynamically assigned,
	// we can't rely on a fixed number -- it will vary if there's a
	// keyboard connected when the program starts.


	struct dirent **namelist;
	int             n;
	char            evName[100] = "";

	if((n = scandir("/sys/devices/virtual/input",
	  &namelist, filter1, NULL)) > 0) {
		// Got a list of device(s).  In theory there should
		// be only one that makes it through the filter (name
		// matches retrogame)...if there's multiples, only
		// the first is used.  (namelist can then be freed)
		char path[100];
		sprintf(path, "/sys/devices/virtual/input/%s",
		  namelist[0]->d_name);
		for(i=0; i<n; i++) free(namelist[i]);
		free(namelist);
		// Within the given device path should be subpath with
		// the name 'eventX' (X varies), again theoretically
		// should be only one, first in list is used.
		if((n = scandir(path, &namelist, filter2, NULL)) > 0) {
			sprintf(evName, "/dev/input/%s",
			  namelist[0]->d_name);
			for(i=0; i<n; i++) free(namelist[i]);
			free(namelist);
		}
	}

	if(!evName[0]) { // Nothing found?  Use fallback method...
		// Kinda lazy skim for last item in /dev/input/event*
		// This is NOT guaranteed to be retrogame, but if the
		// above method fails for some reason, this may be
		// adequate.  If there's a USB keyboard attached at
		// boot, it usually instantiates in /dev/input before
		// retrogame, so even if it's then removed, the index
		// assigned to retrogame stays put...thus the last
		// index mmmmight be what we need.
		char        name[32];
		struct stat st;
		for(i=99; i>=0; i--) {
			sprintf(name, "/dev/input/event%d", i);
			if(!stat(name, &st)) break; // last valid device
		}
		strcpy(evName, (i >= 0) ? name : "/dev/input/event0");
	}


	if((fd2 = open(evName, O_WRONLY | O_NONBLOCK)) >= 0)
		fd = fd2; // Override fd; write to eventX instead

	// Initialize input event structures
	memset(&keyEv, 0, sizeof(keyEv));
	keyEv.type  = EV_KEY;
	memset(&synEv, 0, sizeof(synEv));
	synEv.type  = EV_SYN;
	synEv.code  = SYN_REPORT;
	synEv.value = 0;

	// 'fd' is now open file descriptor for issuing uinput events


	// ----------------------------------------------------------------
	// Monitor GPIO file descriptors for button events.  The poll()
	// function watches for GPIO IRQs in this case; it is NOT
	// continually polling the pins!  Processor load is near zero.

	while(running) { // Signal handler can set this to 0 to exit
		// Wait for IRQ on pin (or timeout for button debounce)
		if(poll(p, j, timeout) > 0) { // If IRQ...
			// Scan backwards so buttons have top priority
			// (first three elements of p[] are for signals,
			// OK if these are handled less quickly).
			for(i=j-1; i>=0; i--) { // From last non-GND to 0
				if(p[i].revents) { // Event received?
					if(i > 2) { // Is a button
						timeout = debounceTime;
						// Read current pin state,
						// store in internal state
						// flag, but don't issue to
						// uinput yet -- must wait
						// for debounce!
						lseek(p[i].fd, 0, SEEK_SET);
						read(p[i].fd, &c, 1);
						if(c == '0')
							intstate[i-3] = 1;
						else if(c == '1')
							intstate[i-3] = 0;
					} else { // Is a signal
						timeout = -1;
						if(i == 0) {
							struct
							  signalfd_siginfo
							  info;
							read(p[0].fd, &info,
							  sizeof(info));
							if(info.ssi_signo ==
							  SIGHUP) {
								// Reload
								// config
								printf(
								  "SIGHUP\n");
							} else {
								// Other
								// signal --
								// abort
								// program
								running = 0;
							}
						} else if(i == 1) {
							// File change,
							// reload config
						} else {
							// Directory change,
							// maybe reload config
						}
					}
					p[i].revents = 0;
				}
			}
			c = 0; // Don't issue SYN event
			// Else timeout occurred
		} else if(timeout == debounceTime) { // Button debounce timeout
			// 'j' (number of non-GNDs) is re-counted as
			// it's easier than maintaining an additional
			// remapping table or a duplicate key[] list.
			bitMask = 0L; // Mask of buttons currently pressed
			bit     = 1L;
			for(c=i=j=0; io[i].pin >= 0; i++, bit<<=1) {
				if(io[i].key != GND) {
					// Compare internal state against
					// previously-issued value.  Send
					// keystrokes only for changed states.
					if(intstate[j] != extstate[j]) {
						extstate[j] = intstate[j];
						keyEv.code  = io[i].key;
						keyEv.value = intstate[j];
						write(fd, &keyEv,
						  sizeof(keyEv));
						c = 1; // Follow w/SYN event
						if(intstate[j]) { // Press?
							// Note pressed key
							// and set initial
							// repeat interval.
							lastKey = i;
							timeout = repTime1;
						} else { // Release?
							// Stop repeat and
							// return to normal
							// IRQ monitoring
							// (no timeout).
							lastKey = timeout = -1;
						}
					}
					j++;
					if(intstate[i]) bitMask |= bit;
				}
			}
			j += 3; // Signal fds occupy first 3 slots of p[]

			// If the "Vulcan nerve pinch" buttons are pressed,
			// set long timeout -- if this time elapses without
			// a button state change, esc keypress will be sent.
			if((bitMask & vulcanMaskA) == vulcanMaskA){
				timeout = vulcanTimeA;
				//printf("VulcanA detected\n");
			} else if((bitMask & vulcanMaskB) == vulcanMaskB){
				timeout = vulcanTimeB;
				//printf("VulcanB detected\n");
			}
				
		} else if(timeout == vulcanTimeA) { // Vulcan timeoutA occurred
			// Send keycode (MAME exits or displays exit menu)
			//keyEv.code = vulcanKey;
			//for(i=1; i>= 0; i--) { // Press, release
			//	keyEv.value = i;
			//	write(fd, &keyEv, sizeof(keyEv));
			//	usleep(10000); // Be slow, else MAME flakes
			//	write(fd, &synEv, sizeof(synEv));
			//	usleep(10000);
			//}

			brightness-=bright_interval; //Subtract brightness interval from current brightness
			//printf("Doing vulcanA: %d\n",brightness);
			//basic clipping
			if (brightness<0){
				brightness=0;
            } else if (brightness>100){
				brightness=100;
			}
			softPwmWrite(7,brightness); //TFT blacklight pin is wiringPi pin 7
			timeout = -1; // Return to normal processing
			c       = 0;  // No add'l SYN required
		} else if(timeout == vulcanTimeB) {
			brightness+=bright_interval; //Subtract brightness interval from current brightness
			//printf("Doing vulcanB: %d\n",brightness);
			//basic clipping
			if (brightness<0){
				brightness=0;
            } else if (brightness>100){
				brightness=100;
			}
			softPwmWrite(7,brightness); //TFT blacklight pin is wiringPi pin 7
			timeout = -1; // Return to normal processing
			c       = 0;  // No add'l SYN required
		} else if(lastKey >= 0) { // Else key repeat timeout
			if(timeout == repTime1) timeout = repTime2;
			else if(timeout > 30)   timeout -= 5; // Accelerate
			c           = 1; // Follow w/SYN event
			keyEv.code  = io[lastKey].key;
			keyEv.value = 2; // Key repeat event
			write(fd, &keyEv, sizeof(keyEv));
		}
		if(c) write(fd, &synEv, sizeof(synEv));
	}

	// ----------------------------------------------------------------
	// Clean up

	ioctl(fd, UI_DEV_DESTROY); // Destroy and
	close(fd);                 // close uinput
	cleanup();                 // Un-export pins

	puts("Done.");

	return 0;
}
