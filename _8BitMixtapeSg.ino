#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

/*


         ##          ##
           ##      ##
         ##############
       ####  ######  ####
     ######################
     ##  ##############  ##
     ##  ##          ##  ##
           ####  ####


*/

#define SONGS_COUNT 9

volatile unsigned long t; // long
volatile unsigned long u; // long
volatile uint8_t snd; // 0...255

volatile uint8_t pot1; // 0...255
volatile uint8_t pot2; // 0...255
volatile uint8_t pot3; // 0...255

volatile uint8_t songs = 0;

volatile uint8_t btn1_previous = 1;
volatile uint8_t btn2_previous = 1;

//ADMUX ADC

volatile uint8_t adc1 = _BV(ADLAR) | _BV(MUX0); //PB2-ADC1 pot1
volatile uint8_t adc2 = _BV(ADLAR) | _BV(MUX1); //PB4-ADC2 pot2
volatile uint8_t adc3 = _BV(ADLAR) | _BV(MUX0) | _BV(MUX1); //PB3-ADC3 pot3

#define ENTER_CRIT()    {byte volatile saved_sreg = SREG; cli()
#define LEAVE_CRIT()    SREG = saved_sreg;}

#define true 1
#define false 0

//button state
#define BUTTON_NORMAL 0
#define BUTTON_PRESS 1
#define BUTTON_RELEASE 2
#define BUTTON_HOLD 3

//button 1 timer
uint8_t button_timer_enabled = false;
unsigned int button_timer = 0;
unsigned int button_last_pressed = 0;


//loop mode on button 1 hold
uint8_t start_loop = false;
unsigned int loop_timer = 0;
unsigned int loop_max = 0;
unsigned int hold_timer = 0;


void adc_init()
{
  ADCSRA |= _BV(ADIE); //adc interrupt enable
  ADCSRA |= _BV(ADEN); //adc enable
  ADCSRA |= _BV(ADATE); //auto trigger
  ADCSRA |= _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2); //prescale 128
  ADMUX  = adc1;
  ADCSRB = 0;
}

void adc_start()
{
  ADCSRA |= _BV(ADSC); //start adc conversion
}

void timer_init()
{
         
//no prescale, enable 16 Mhz
   
   clock_prescale_set(clock_div_1);


//pwm timer

    TCCR0A = 3<<WGM00;                      // Fast PWM
    TCCR0B = 1<<WGM02 | 2<<CS00;            // 1/8 prescale
    TIMSK = 1<<OCIE0A;                      // Enable compare match
    OCR0A = 61;


//pwm timer 

    PLLCSR |= (1 << PLLE) | (1 << PCKE);    // Enable PLL and async PCK for high-speed PWM
    TCCR1 |= _BV(CTC1);
    TCCR1 |= _BV(CS10);
    TCCR1 |= _BV(PWM1A);
    TCCR1 |= 3<<COM1A0;

//speaker out PB1

    DDRB |= 1<<DDB1; //set PB1 as output
    PORTB &= ~(1 << PB1); //set PB1 output 0
    
    
}


void button_init()
{
  //button is pb4 and pb0
  DDRB &= ~(1 << PB4 | 1 << PB0); //set to input
  PORTB |= (1 << PB4 | 1 << PB0); //pin btn
}


int button_is_pressed(uint8_t button_pin, uint8_t button_bit)
{
  /* the button is pressed when BUTTON_BIT is clear */
  if (!bit_is_clear(button_pin, button_bit))
  {
    //_delay_ms(25);
    if (!bit_is_clear(button_pin, button_bit)) return 1;
  }

  return 0;
}

void disable_button_timer()
{
  button_timer_enabled = false;
  button_timer = 0;
}

void enable_button_timer()
{
  button_timer_enabled = true;
  button_timer = 0;
}

int main(void)
{
  timer_init();// initialize timer & Pwm
  adc_init(); //init adc
  button_init();
  sei(); //enable global interrupt
  adc_start(); //start adc conversion

  // run forever

  while (1)
  {
    /*
      uint8_t btn1_now = button_is_pressed(PINB, PB1);
      if ( btn1_previous != btn1_now && btn1_now == 1 ) {
        songs++;
        if (songs > 3) songs = 0;
        btn1_previous = btn1_now;
      }else{
        btn1_previous = btn1_now;
      }
    */
    switch (songs)
    {

      case 0:
        snd = (t * (5 + (pot1 / 5))&t >> 7) | (t * 3 & t >> (10 - (pot2 / 5)));
        //snd = (t*t/(256-pot1))&(t>>((t/(1024-pot2))%16))^t%64*(0xC0D3DE4D69>>(t>>9&30)&t%32)*t>>18;
        break;

      case 1:
        snd = (t|(t>>(9+(pot1/2))|t>>7))*t&(t>>(11+(pot2/2))|t>>9);
        break;

      case 2:
        snd = (t * 9 & t >> 4 | t * 5 & t >> (7 + (pot1 / 2)) | t * 3 & t / (1024 - (pot2 / 2))) - 1;
        //snd = (t*t/(256-pot1))&(t>>((t/(1024-pot2))%16))^t%64*(0xC0D3DE4D69>>(t>>9&30)&t%32)*t>>18;
        break;

      case 3:
        snd = (t >> 6 | t | t >> (t >> (16 - (pot1 / 2)))) * 10 + ((t >> 11) & (7 + (pot2 / 2)));
        break;

      case 4:
        snd = t * (((t >> (11 - (pot2 / 2))) & (t >> 8)) & ((123 - pot1) & (t >> 3)));
        //snd = (t*t/(256-pot1))&(t>>((t/(1024-pot2))%16))^t%64*(0xC0D3DE4D69>>(t>>9&30)&t%32)*t>>18;
        break;

      case 5:
        snd = t * (t ^ t + (t >> 15 | 1) ^ (t - (1280 - (pot1 / 2))^t) >> (10 - (pot2 / 5)));
        break;

      case 6:
        snd = ((t & 4096) ? ((t * (t ^ t % (255 + pot1)) | (t >> 4)) >> 1) : (t >> (3 + (pot2 / 2))) | ((t & 8192 - pot2) ? t << 2 : t));
        //snd = (t*t/(256-pot1))&(t>>((t/(1024-pot2))%16))^t%64*(0xC0D3DE4D69>>(t>>9&30)&t%32)*t>>18;
        break;

      case 7:
        snd = (t&t>>12)*(t>>(4+pot1)|t>>8)^(t>>6+pot2); 
        break;

      case 8:
        snd = t * (((t >> 9) ^ ((t >> 9) - (1 + (pot1 / 2))) ^ 1) % (13 + (pot2 / 2)));
        break;

      case 9:
        snd = t * (((t >> (12 + (pot1 / 2))) | (t >> 8)) & ((63 - (pot2 / 2)) & (t >> 4)));
        break;

    }

  }
  return 0;
}

ISR(TIMER1_COMPA_vect)
{

  uint8_t btn1_now = button_is_pressed(PINB, PB4);
  uint8_t btn2_now = button_is_pressed(PINB, PB0);

  uint8_t button1_state = 0;
  uint8_t button2_state = 0;

  //button state

  //  button 1
  if ( btn1_previous != btn1_now && btn1_now == 0 ) {
    button1_state  = BUTTON_PRESS;
  } else if ( btn1_previous != btn1_now && btn1_now == 1  )
  {
    button1_state  = BUTTON_RELEASE;
  } else if (btn1_previous == btn1_now && btn1_now == 0)
  {
    button1_state  = BUTTON_HOLD;
  } else {
    button1_state  = BUTTON_NORMAL;
  }

  //  button 2

  if ( btn2_previous != btn2_now && btn2_now == 0 ) {
    button2_state  = BUTTON_PRESS;
  } else if ( btn2_previous != btn2_now && btn2_now == 1  )
  {
    button2_state  = BUTTON_RELEASE;
  } else if (btn2_previous == btn2_now && btn2_now == 0)
  {
    button2_state  = BUTTON_HOLD;
  } else {
    button2_state  = BUTTON_NORMAL;
  }

  //end of button state
  //start button 1 action

  if (button1_state == BUTTON_RELEASE)
  {
    if (songs < SONGS_COUNT)
    {
      songs++;
    } else {
      songs = 0;
    }
  }

  //end button 1 action
  //start button 2 action

  if (button2_state == BUTTON_RELEASE)
  {
    if (songs > 0)
    {
      songs--;
    } else {
      songs = SONGS_COUNT;
    }

  }

  //end button 2 action

  //sound generator pwm out
  OCR1A = snd;
  t++;

  //button logic
  btn1_previous = btn1_now;
  btn2_previous = btn2_now;

}

ISR(ADC_vect)
{
  //http://joehalpin.wordpress.com/2011/06/19/multi-channel-adc-with-an-attiny85/

  static uint8_t firstTime = 1;
  static uint8_t val;

  val = ADCH;

  if (firstTime == 1)
    firstTime = 0;
  else if (ADMUX  == adc1) {
    pot1 = val;
    ADMUX = adc3;
  }
  else if ( ADMUX == adc3) {
    pot2  = val;
    ADMUX = adc1;
  }

}
