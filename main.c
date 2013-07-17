/*****************************************************************************
* File          : main.c
* Created       : Apr 7, 2013
*
* Title         : ---
* Author        : goepfert
* Contact       : info@SolderLab.de
*
* Version       : 1.0
* Last Changed  : Apr 7, 2013 by goepfert
* Remarks       : ---
*
* Description   : ---
*
*****************************************************************************/

#include "irmp.h"
#include "pwm.h"
#include "colors.h"
#include "uart_stream.h"
#include "helper.h"

#include <inttypes.h>

#define DEBUGLED 0,C

volatile uint32_t millis = 0;
volatile uint8_t comp = 0;
uint32_t last_millis = 0;

int16_t duration = 200;

rgb_t rgb_color;
hsv_t hsv_color;
uint8_t white_value = 0;

typedef enum ACTION {
	NONE, HUE_FADE
} action_t;

action_t action = NONE;

void timer1_init(void);
void printIR(IRMP_DATA* irmp_data);
void change_hue(float val);
void change_saturation(float val);
void change_value(float val);
void change_white(int16_t val);
void update_color();

int main(void) {

	out(DEBUGLED);
	off(DEBUGLED);

	uart_init(9600);
	uart_flush();
	stdout= &mystdout;

	irmp_init();
	IRMP_DATA irmp_data;

	timer1_init();

	pwm_initialize();
	sei ();

	printf("\n\n== IR Remote ==\n");

	while (1) {
		if (irmp_get_data(&irmp_data)) {

			if (pwm_status == OFF && irmp_data.command != 1)
				continue;

			if (!(irmp_data.flags & IRMP_FLAG_REPETITION)) {

				printIR(&irmp_data);

				switch (irmp_data.command) {

				case (1):
					if (pwm_status == ON) {
						pwm_disable_all();
						off(DEBUGLED);
						printf("OFF\n");
					} else {
						pwm_enable_all();
						on(DEBUGLED);
						printf("ON\n");
					}
					action = NONE;
					break;

				case (30):
					change_white(1);
					break;
				case (36):
					change_white(-1);
				break;

				case (4):
					change_white(-255);
					break;
				case (14):
					rgb_color.r=0; rgb_color.g=0; rgb_color.b=0;
					hsv_color = RgbToHsv(&rgb_color);
					update_color();
					break;

				case (40):
					change_hue(5.);
					action = NONE;
					break;
				case (41):
					change_hue(-5.);
					action = NONE;
					break;
				case (44):
					change_saturation(0.1);
					action = NONE;
					break;
				case (45):
					change_saturation(-0.1);
					action = NONE;
					break;
				case (9):
					change_value(0.1);
					action = NONE;
					break;
				case (5):
					change_value(-0.1);
					action = NONE;
					break;
				case (7):
					duration += 10;
					break;
				case (8):
					duration -= 10;
					break;

				case (16):
					colors_gen(RED, &rgb_color, &hsv_color);
					update_color();
					action = NONE;
					break;
				case (17):
					colors_gen(GREEN, &rgb_color, &hsv_color);
					update_color();
					action = NONE;
					break;
				case (18):
					colors_gen(BLUE, &rgb_color, &hsv_color);
					update_color();
					action = NONE;
					break;
				case (19):
					colors_gen(CYAN, &rgb_color, &hsv_color);
					update_color();
					action = NONE;
					break;
				case (20):
					colors_gen(MAGENTA, &rgb_color, &hsv_color);
					update_color();
					action = NONE;
					break;
				case (21):
					colors_gen(YELLOW, &rgb_color, &hsv_color);
					update_color();
					action = NONE;
					break;
				case (22):
					colors_gen(WHITE, &rgb_color, &hsv_color);
					update_color();
					action = NONE;
					break;
				case (23):
					colors_gen(BROWN, &rgb_color, &hsv_color);
					update_color();
					action = NONE;
					break;
				case (24):
					colors_gen(RANDOM, &rgb_color, &hsv_color);
					update_color();
					action = NONE;
					break;

				case (15):
					action = (action == HUE_FADE) ? NONE : HUE_FADE;
					break;

				default:
					break;
				}
			} else {
				switch (irmp_data.command) { // if you press the button longer
				case (30):
					change_white(5);
					break;
				case (36):
					change_white(-5);
					break;
				case (40):
					change_hue(0.5);
					break;
				case (41):
					change_hue(-0.5);
					break;
				case (44):
					change_saturation(0.01);
					break;
				case (45):
					change_saturation(-0.01);
					break;
				case (9):
					change_value(0.01);
					break;
				case (5):
					change_value(-0.01);
					break;
				case (7):
					duration += 5;
					break;
				case (8):
					duration -= 5;
					break;
				default:
					break;
				}
			}
		} //end remote switch

		switch (action) {

		case (HUE_FADE):
			if (duration <= 0)
				duration = 0;
			if ((millis - last_millis) > duration) {
				change_hue(1.);
				last_millis = millis;
			}
			break;

		default:
			break;

		} // action switch
	}
	//end while loop
}
//end main

void change_hue(float val) {

	float tmp = hsv_color.h + val;

	if (tmp > 360) {
		tmp -= 360.;
	} else if (tmp < 0) {
		tmp = 360. - fabs(tmp);
	}

	hsv_color.h = tmp;
	rgb_color = HsvToRgb(&hsv_color);
	update_color();
}

void change_saturation(float val) {

	float tmp = hsv_color.s + val;

	if (tmp > 1.0)
		tmp = 1.;
	if (tmp < 0.)
		tmp = 0.001;

	hsv_color.s = tmp;
	rgb_color = HsvToRgb(&hsv_color);

	update_color();
}

void change_value(float val) {

	float tmp = hsv_color.v + val;

	if (tmp > 1.0)
		tmp = 1.;
	if (tmp < 0.)
		tmp = 0.;

	hsv_color.v = tmp;
	rgb_color = HsvToRgb(&hsv_color);

	update_color();
}

void update_color() {
	pwm_setdutycycle(CHAN_RED, (uint8_t) (rgb_color.r * 100 + 0.5));
	pwm_setdutycycle(CHAN_GREEN, (uint8_t) (rgb_color.g * 100 + 0.5));
	pwm_setdutycycle(CHAN_BLUE, (uint8_t) (rgb_color.b * 100 + 0.5));
}

void change_white(int16_t inc) {

	int16_t tmp = white_value + inc;

	if(tmp > 255)
		tmp = 255;
	if(tmp < 0)
		tmp = 0;

	white_value = tmp;
	pwm_setdutycycle(CHAN_WHITE, white_value);
}

void timer1_init(void) {

	OCR1A = (F_CPU / F_INTERRUPTS) - 1;   // compare value: 1/15000 of CPU frequency
	TIMSK1 |= (1 << OCIE1A);              // OCIE1A: Interrupt by timer compare
	TCCR1B = (1 << WGM12) | (1 << CS10);  // switch CTC Mode on, set prescaler to 1
}

void printIR(IRMP_DATA* irmp_data) {

	printf("protocol: %s\n", irmp_protocol_names[irmp_data->protocol]);
	printf("address: %u\n", irmp_data->address);
	printf("command: %u\n", irmp_data->command);
	printf("flags: %u\n", irmp_data->flags);
}

ISR(TIMER1_COMPA_vect) {
	comp++;
	if (!(comp % F_INTERRUPTS / 1000)) {
		millis++;
		comp = 0;
	}
	irmp_ISR();
}

