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
#include <wiringPi.h>
#include <stdbool.h>
#include "uinput.h"
#include "nunchuk.h"

#define RED 22
#define YELLOW 4 
#define BLUE 27
#define GREEN 17
#define NUMBUTTONS 4


struct nunchuk nun, last_nun;

void nunchuk2pi_init();
void nunchuk2pi_exit();
void catch_signal(int signal);
void nunchuk_print_data(struct nunchuk* n);
void read_callback(int read_success);
void button_init(void);
void button_signals(void);


int buttons[NUMBUTTONS] = {RED, YELLOW, BLUE, GREEN};

int main(int argc, char *argv[])
{
	nunchuk2pi_init();
	nunchuk_init_nunchuk();
	button_init();

	while(1) {
		button_signals();
		//usleep(1000 * 1000);
		usleep(1000 );
	}
	
	nunchuk2pi_exit();
	return 0;
}

void read_callback(int read_success)
{
	if(read_success) {
		//nunchuk_print_data(&nun);
		int x = nun.X-128;
		int y = -(nun.Y-128);
	//	if(abs(x) < 15) x = 0;
	//	if(abs(y) < 15) y = 0;
		//x = (int)((float)x/7.5f);
		//y = (int)((float)y/7.5f);
		//send_rel_mouse(x, y);
		if(x > 75){
			send_key_press(KEY_RIGHT);
			printf("R");
		} else{
			send_key_release(KEY_RIGHT);
		}
		if(x < -75){
			send_key_press(KEY_LEFT);
			printf("L");
		} else{
			send_key_release(KEY_LEFT);
		}
		if(y < -75){
			send_key_press(KEY_UP);
			printf("U");
		} else{
			send_key_release(KEY_UP);
		}
		if(y > 75){
			send_key_press(KEY_DOWN);
			printf("D");
		} else{
			send_key_release(KEY_DOWN);
		} 
		
		/*if(nun.C && !last_nun.C) {
			send_key_press(KEY_X);
		}
		if(!nun.C && last_nun.C) {
			send_key_release(KEY_X);
		}
		if(nun.Z && !last_nun.Z) {
			send_key_press(KEY_C);
		}
		if(!nun.Z && last_nun.Z) {
			send_key_release(KEY_C);
		}*/
		send_report();
		last_nun = nun;
	}
}

void nunchuk_print_data(struct nunchuk* n)
{
 	printf("joyX: %d  joyY: %d  Z: %d  C: %d\n", n->X, n->Y, n->Z, n->C);
 	printf("aX: %d  aY: %d  aZ: %d\n", n->aX, n->aY, n->aZ);
}

void nunchuk2pi_init()
{
	printf("nunchuk2pi init:\n");
	
	signal(SIGTERM, catch_signal);
	signal(SIGKILL, catch_signal);
	signal(SIGINT, catch_signal);
	
	if(uinput_init() < 0) {
		printf("Could not open uinput.\n");
		exit(EXIT_FAILURE);
	}
	if(nunchuk_init(&nun) < 0) {
		printf("Could not init nunchuk.\n");
		exit(EXIT_FAILURE);
	}
	nunchuk_set_read_callback(read_callback);
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

void button_init(){
	wiringPiSetupGpio();
	int i = 0;
	for(i = 0; i < NUMBUTTONS; i++){
		pinMode( buttons[i], INPUT);
		pullUpDnControl(buttons[i], PUD_UP);
	}
	pullUpDnControl(2, PUD_UP);
	pullUpDnControl(3, PUD_UP);
}

void button_signals(){
	int i = 0;
	static bool red_pressed = false;
	static bool yellow_pressed = false;
	static bool blue_pressed = false;
	static bool green_pressed = false;

	for(i = 0; i < NUMBUTTONS; i++){
		switch (buttons[i]){
			case RED:
				if(!digitalRead(RED)){
					if(!red_pressed){
						send_key_press(KEY_Z);
						red_pressed = true;
						printf("Red Pressed\n");
						//delay(50);
					}
				}else{
					if(red_pressed){
						send_key_release(KEY_Z);
						printf("Red Released\n");
					}
					red_pressed = false;
					//delay(50);
				}
				break;
			case YELLOW:
				if(!digitalRead(YELLOW)){
					if(!yellow_pressed){
						send_key_press(KEY_X);
						yellow_pressed = true;
						//delay(50);
					}
				}else{
					if(yellow_pressed){
						send_key_release(KEY_X);
					}
					yellow_pressed = false;
					//delay(50);
				}
				break;
			case BLUE:
				if(!digitalRead(BLUE)){
					if(!blue_pressed){
						send_key_press(KEY_A);
						blue_pressed = true;
						//delay(50);
					}
				}else{
					if(blue_pressed){
						send_key_release(KEY_A);
					}
					blue_pressed = false;
					//delay(50);
				}
				break;
			case GREEN:
				if(!digitalRead(GREEN)){
					if(!green_pressed){
						send_key_press(KEY_S);
						green_pressed = true;
						//delay(50);
					}
				}else{
					if(green_pressed){
						send_key_release(KEY_S);
					}
					green_pressed = false;
					//delay(50);
				}
				break;
		}
	}
}
