#include <avr/io.h>
#include <avr/interrupt.h>

#include <util/delay.h>

#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "bits.h"


#define ENC_MAX ((10 * 4))


static int8_t enc_last_r,
              enc_last_g,
              enc_last_b;


const uint16_t pwmtable[ENC_MAX + 1] = {
  0,
  1, 1, 1, 1,
  1, 2, 2, 3,
  3, 3, 4, 5,
  6, 6, 7, 9,
  10, 12, 13, 15,
  18, 21, 24, 27,
  31, 36, 42, 48,
  55, 63, 73, 84,
  96, 111, 127, 146,
  168, 193, 222, 255,
};

const uint8_t dithertable[4] = {
  0b00000000,
  0b00100010,
  0b10101010,
  0b11011101,
};


int8_t enc_read(uint8_t port, uint8_t phase_a, uint8_t phase_b) {
  int8_t val = 0b00;
  if (port & (1 << phase_a)) val |= 0b11;
  if (port & (1 << phase_b)) val ^= 0b01;
  return val;
}

uint8_t enc_process(uint8_t curr, int8_t diff) {
  if (diff & 0b01) {
    if (diff & 0b10) {
      return (curr > 0) ? curr - 1 : 0;
    } else {
      return (curr < ENC_MAX) ? curr + 1 : ENC_MAX;
    }
  }

  return curr;
}


int main() {

  // Setup PWM output
  bit_set(DDRB, PB3);
  bit_set(DDRD, PD3);
  bit_set(DDRD, PD5);
  bit_set(DDRD, PD6);

  TCCR0A = (1 << COM0A1)  // OC0A set on 0, clear on match
	       | (0 << COM0A0)  // ...
         | (1 << COM0B1)  // OC0B set on 0, clear on match
	       | (0 << COM0B0)  // ...
	       | (1 << WGM00)   // PWM, phase-correct
         | (0 << WGM01)   // ...
	       ;
  TCCR0B = (0 << WGM02)   // PWM, phase-correct
         | (0 << CS02)    // Prescaler of clk/8
         | (1 << CS01)    // ...
         | (0 << CS00)    // ...
         ;

  TCCR2A = (1 << COM2A1)  // OC2A set on 0, clear on match
	       | (0 << COM2A0)  // ...
         | (1 << COM2B1)  // OC2B set on 0, clear on match
	       | (0 << COM2B0)  // ...
	       | (1 << WGM20)   // PWM, phase-correct
         | (0 << WGM21)   // ...
	       ;
  TCCR2B = (0 << WGM22)   // PWM, phase-correct
         | (0 << CS22)    // Prescaler of clk/8
         | (1 << CS21)    // ...
         | (0 << CS20)    // ...
         ;

	OCR0A = 0x00; // Mapped to PD6
	OCR0B = 0x00; // Mapped to PD5
	OCR2A = 0x00; // Mapped to PB3
	OCR2B = 0x00; // Mapped to PD3

  // Setup encoders
  bit_clr(DDRC, PC0); // Inputs
  bit_clr(DDRC, PC1);
  bit_clr(DDRC, PC2);
  bit_clr(DDRC, PC3);
  bit_clr(DDRC, PC4);
  bit_clr(DDRC, PC5);

  bit_set(PORTC, PC0); // Enable pull-up
  bit_set(PORTC, PC1);
  bit_set(PORTC, PC2);
  bit_set(PORTC, PC3);
  bit_set(PORTC, PC4);
  bit_set(PORTC, PC5);

  // Prime encoder state
  enc_last_r = enc_read(PINC, PC0, PC1);
  enc_last_g = enc_read(PINC, PC2, PC3);
  enc_last_b = enc_read(PINC, PC4, PC5);

  // Setup buttons
  bit_clr(DDRB, PB0); // Inputs
  bit_clr(DDRB, PB1);
  bit_clr(DDRB, PB2);

  bit_set(PORTB, PB0); // Enable pull-up
  bit_set(PORTB, PB1);
  bit_set(PORTB, PB2);

  // Configure input timer
  TCCR1A = (0 << COM1A1) // Outputs not connected
         | (0 << COM1A0) // ...
         | (0 << COM1B1) // ...
         | (0 << COM1B0) // ...
         | (0 << WGM11)  // CTC mode
         | (0 << WGM10)
         ;
  TCCR1B = (0 << WGM13)  // CTC mode
         | (1 << WGM12)  // ...
         | (0 << CS12)   // Prescaler of clk/64
         | (1 << CS11)   // ...
         | (1 << CS10)   // ...
         ;
  OCR1AH = 0x00;
  OCR1AL = (uint8_t)(F_CPU / 64.0 * 1e-3 - 0.5); // 1ms

  bit_set(TIMSK1, OCIE1A);

  // Go
  sei();

  // Nothing left to do...
  for (;;) ;
}

ISR(TIMER1_COMPA_vect) {
  // Overflowing loop counter
  static uint8_t loop = 0;

  // Target color
  static uint8_t target_r = 0,
                 target_g = 0,
                 target_b = 0;

  // Scan an process encoders
  {
    // Read encoder status
    const uint8_t enc = PINC;
    const int8_t enc_r = enc_read(enc, PC5, PC4);
    const int8_t enc_g = enc_read(enc, PC1, PC0);
    const int8_t enc_b = enc_read(enc, PC3, PC2);

    // Process encoder changes to update target color
    target_r = enc_process(target_r, enc_last_r - enc_r);
    target_g = enc_process(target_g, enc_last_g - enc_g);
    target_b = enc_process(target_b, enc_last_b - enc_b);

    // Remember last encoder state
    enc_last_r = enc_r;
    enc_last_g = enc_g;
    enc_last_b = enc_b;
  }

  // Scan and process buttons
  {
    static uint8_t trk = 0x00,
                   ct0 = 0xFF,
                   ct1 = 0xFF;
    static uint16_t rpt;

    // Read changed button state
    uint8_t btn = trk ^ ~PINB;

    // Debounce
    ct0 = ~(ct0 & btn);
    ct1 = ct0 ^ ct1;
    btn &= ct0 & ct1;

    // Update button statud
    trk ^= btn;

    // Handle pressed buttons
    uint8_t btn_pressed = trk & btn;
    if (btn_pressed & (1 << 1)) target_r = (!target_r) * ENC_MAX;
    if (btn_pressed & (1 << 0)) target_g = (!target_g) * ENC_MAX;
    if (btn_pressed & (1 << 2)) target_b = (!target_b) * ENC_MAX;

    // Handle long pressing of any button
    if (!(trk & ((1 << 0) | (1 << 1) | (1 << 2)))) rpt = 750;
    else if (--rpt == 0) {
      target_r = target_g = target_b = 0x00;
    }
  }

  // Update PWM outouts
  {
    static uint8_t current_r = 0,
                   current_g = 0,
                   current_b = 0;
    
    // Fade values to target
    if (loop % 32 == 0) {
      int8_t step(const uint8_t a, const uint8_t b) {
        const int16_t i = (int16_t)a - (int16_t)b;
        return i ? i > 0 ? 1 : -1 : 0;
      }

      current_r += step(target_r, current_r);
      current_g += step(target_g, current_g);
      current_b += step(target_b, current_b);
    }

    // Extract white part from color
    const uint8_t w = (current_r < current_g)
      ? (current_r < current_b)
        ? current_r
        : current_b
      : (current_g < current_b)
        ? current_g
        : current_b;
    
    const uint8_t r = current_r - w;
    const uint8_t g = current_g - w;
    const uint8_t b = current_b - w;

    // Dither colors for additional PWM resolution
    uint8_t process(const uint8_t val) {
      const uint8_t pwm = pwmtable[val];
      const bool x = (dithertable[pwm & 0b11] >> (loop % 8)) & 0b1;
      return (pwm >> 2) + x;
    }

    OCR0A = process(w);
    OCR0B = process(b);
    OCR2A = process(g);
    OCR2B = process(r);
  }

  loop++;
}
