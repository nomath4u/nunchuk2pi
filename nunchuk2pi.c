/* by xerpi (c) 2013 */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <linux/uinput.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "uinput.h"
#include "nunchuk.h"

void nunchuk2pi_init();
void nunchuk2pi_exit();
void catch_signal(int signal);
void nunchuk_print_data(struct nunchuk* n);


int main(int argc, char *argv[])
{
	nunchuk2pi_init();
	struct nunchuk n;

	while(1) {
		if(nunchuk_is_initiated()) {
			nunchuk_read_data(&n);
			usleep(100);
			nunchuk_print_data(&n);
			usleep(100 * 1000);
		} else {
			nunchuk_init();
			usleep(1000 * 1000);
		}
	}
	
	nunchuk2pi_exit();
	return 0;
}


void nunchuk_print_data(struct nunchuk* n)
{
 	printf("joyX: %d  joyY: %d  Z: %d  C: %d\n", n->X, n->Y, n->Z, n->C);
 	printf("aX: %d  aY: %d  aZ: %d\n", n->aX, n->aY, n->aZ);
}


void nunchuk2pi_init()
{
	printf("nunchuk2pi initiated!\n");
	
	signal(SIGTERM, catch_signal);
	signal(SIGKILL, catch_signal);
	signal(SIGINT, catch_signal);
	
	if(uinput_init() < 0) {
		printf("Could not open uinput\n");
		exit(EXIT_FAILURE);
	}
	if(nunchuk_init() < 0) {
		printf("Could not init nunchuk\n");
		exit(EXIT_FAILURE);
	}
}

void nunchuk2pi_exit()
{
	uinput_exit();
	nunchuk_exit();
	printf("\nn2pi exit\n");	
}

void catch_signal(int signal)
{
	printf("Signal %i caught.\n", signal);
	nunchuk2pi_exit();
	exit(-1);
}
