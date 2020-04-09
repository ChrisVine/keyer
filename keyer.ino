/*

  Copyright (C) 2020 Chris Vine

  This file is licensed under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance with the
  License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
  implied.  See the License for the specific language governing
  permissions and limitations under the License.

  -------------------------------------------------------------------

  General description:

  This is the G3XXF morse keyer for the Arduino.  It has been tested
  with the Arduino Uno and the arduino-1.8.12 development kit.  It
  will take either single or dual lever paddles.  If used with dual
  lever paddles, by default it operates in "last-pressed" mode rather
  than iambic mode.

  Last-pressed mode
  -----------------

  Last-pressed mode means that if both dot and dash paddles are
  pressed at the same time, the last paddle pressed will be the one
  which sends.  This is coupled with a memory so that if the dot paddle
  is pressed and released while a dash is being sent, a dot will be
  entered into memory and will be sent after the dash has finished
  (and similarly if the dash paddle is pressed while a dot is being
  sent).

  Iambic mode
  -----------

  As an alternative, the program can be configured so that the keyer
  operates in iambic mode, whereby the dots and dashes alternate when
  both levers are pressed.  When in iambic mode, the memory facility
  operates differently than in last-pressed mode, to match the
  behaviour of most iambic keyers.  In last-pressed mode, the memory
  is edge triggered - a dot or dash is placed in memory when a key is
  pressed: after that, it becomes the "last-pressed" key and takes
  precedence until the other paddle is released and pressed again.
  However, in iambic mode, if both paddles are pressed at the same
  time, a dot is entered into memory as soon as a dash begins, and
  likewise with a dash as soon as a dot begins.

  In the author's opinion, last-pressed mode is much superior to
  iambic mode for serious CW sending when using dual paddles.  Give it
  a try if you are not used to it.

  With single lever paddles, it makes no difference whether the
  program is compiled for last-pressed mode or iambic mode.  Many
  people prefer single lever to dual lever paddles, and that is fine.
 
  Autospacing
  -----------

  An autospacing option is also provided.  If autospacing is off, then
  any new letter will begin as soon as a paddle is pressed following
  the conclusion of the previous letter, in the same way as any
  ordinary keyer.  If autospacing is on, some automatic spacing will
  take place.  In particular, if a paddle is pressed more than 1 space
  and less than 3 spaces after the previous dot or dash finished, it
  will be assumed that a letter space was wanted and the character
  will not be sent until 3 spaces have elapsed.  If a paddle is
  pressed more than 3 and less than 5 spaces after the previous dot or
  dash finished, it will be assumed the operator wanted a letter space
  but was a little late, so the next dot or dash will be sent
  immediately.  If a paddle is pressed more than 5 spaces and less
  than 7 spaces after the previous dot or dash finished, it will be
  assumed that a word space was wanted and the character will not be
  sent until 7 spaces have elapsed.  After 7 spaces, any pressing of a
  paddle will cause the corresponding dot or dash to be sent
  immediately.

  Pinouts, etc
  ------------
 
  The dot paddle should be connected to pin 4 of the Arduino and the
  dash paddle to pin 5, with the common line connected to ground.

  Morse output is on pin 8, which is configured in this program as a
  low impedance source with 0v (key up) or the working positive
  voltage (+5v on the Arduino Uno) (key down): if the transmitter has
  a positive keying line (as most modern ones do) then it can feed the
  base of an NPN transistor through a current limiting resistor and
  the collector used to key the transmitter.  Most modern transmitters
  with a positive key line have modest voltage and current
  requirements on that line, and a 10K Ohm resistor to the base of a
  general purpose NPN transistor such as a BC547 will often do.  Check
  to see.

  Analogue pin A3 is used for speed control.  It should be connected
  to the wiper of a potentiometer which has 0v and the working
  positive voltage (+5v on an Arduino Uno) at its end points.  A 470K
  Ohm potentiometer works fine.  It is probably best not to make it
  too much higher in resistance or capacitive effects may appear.

  Paddle debouncing is taken care of.  However, because current
  through the paddles is miniscule, it would pay to keep paddle
  contacts clean.
*/


/* user options */
constexpr bool autospace = true;
constexpr bool iambic = false;
constexpr int debounce = 2;  // read further below before changing this

/* pin setup */
constexpr int output = 8;
constexpr int dash_paddle = 5;
constexpr int dot_paddle = 4;
constexpr int speed_var = A3;

/* types */
enum class SendState {off, on, space, pending};
enum class PaddleState {off, on};
enum class LastPressedState {none, dot, dash};
enum class Mem {none, dot, dash};

/* globals */
int counter = 0;
int bit_length;  // length of single dot in millisecs (range 30-200)
SendState dot_send_state = SendState::off;
SendState dash_send_state = SendState::off;
PaddleState dot_paddle_state = PaddleState::off;
PaddleState dash_paddle_state = PaddleState::off;
LastPressedState last_pressed_state = LastPressedState::none;
Mem mem = Mem::none;
int spaces = 0;  // number of spaces elapsed since the last dot or dash

/* functions */

// 'dot_paddle_state', 'dash_paddle_state' and associated global
// variables hold a debounced value; we implement the debouncing in
// the is_dot_pressed() and is_dash_pressed() functions.  The main
// problem with bounce occurs not when a paddle is pressed, but when
// it is released and causes a false memory trigger, so we are
// principally concerned with debouncing transitions from pressed to
// unpressed state.
//
// In is_dot_pressed() and is_dash_pressed(), we reset 'bounce_count'
// whenever 'previous' == 'latest' or there is a transition from
// unpressed to pressed, as this deals best with "trains" of bounces.
// By default the 'debounce' configuration variable is set to 2.  What
// this means is that there must be two iterations through the loop
// with a released paddle shown as unpressed, with no intervening
// spurious pressed result, before the paddle is treated as released.
// So when a bounce occurs on release, a typical sequence would be (i)
// pressed -> unpressed detected; (ii) spurious unpressed -> pressed
// detected; (iii) pressed -> unpressed detected; (iv) still
// unpressed; (v) still unpressed, unpressed state accepted.  At 1 ms
// between iteration of the loop, this is starting to become a
// meaningful proportion of a dot length at 40 words per minute (30 ms
// dot length at 40 wpm).  So if you are a speed freak you could try
// reducing 'debounce' to 1 and keep your paddle contacts clean.
// Alternatively if you are not interested in such high speeds and
// have no aversion to dirty contacts, and you think you are getting
// paddle bounce issues, you could try increasing 'debounce' to 3 or
// 4.

bool is_dot_pressed() {
  static bool previous;
  static int bounce_count;
  bool latest = digitalRead(dot_paddle) == LOW;
  if (previous == latest) { // no change
    bounce_count = 0;    
    return latest;
  }
  if (latest)  { // transition from unpressed to pressed
    bounce_count = 0;
    previous = true;
    return true;
  }
  // if we have reached here we are in a transition from pressed to
  // unpressed state
  if (bounce_count < debounce) {
    ++bounce_count;
    return true;
  }
  else {
    bounce_count = 0;
    previous = false;
    return false;
  }
}

bool is_dash_pressed() {
  static bool previous;
  static int bounce_count;
  bool latest = digitalRead(dash_paddle) == LOW;
  if (previous == latest) { // no change
    bounce_count = 0;    
    return latest;
  }
  if (latest)  { // transition from unpressed to pressed
    bounce_count = 0;
    previous = true;
    return true;
  }
  // if we have reached here we are in a transition from pressed to
  // unpressed state
  if (bounce_count < debounce) {
    ++bounce_count;
    return true;
  }
  else {
    bounce_count = 0;
    previous = false;
    return false;
  }
}

void set_keydown() {
  digitalWrite(output, HIGH);
}

void set_keyup() {
  digitalWrite(output, LOW);
}

void set_bit_length() {
  bit_length = 30 + analogRead(speed_var) / 6;
}

void setup() {
  pinMode(output, OUTPUT);
  set_keyup();
  pinMode(dot_paddle, INPUT_PULLUP);
  pinMode(dash_paddle, INPUT_PULLUP);
}

void loop() {

  if (counter % 10 == 0) set_bit_length();

  bool dot_pressed = is_dot_pressed();
  bool dash_pressed = is_dash_pressed();

  /* set last_pressed_state to the correct value */
  if (dot_pressed && dot_paddle_state == PaddleState::off)
    last_pressed_state = LastPressedState::dot;
  else if (dash_pressed && dash_paddle_state == PaddleState::off)
    last_pressed_state = LastPressedState::dash;
  else if (!dot_pressed && last_pressed_state == LastPressedState::dot)
    last_pressed_state = LastPressedState::none;
  else if (!dash_pressed && last_pressed_state == LastPressedState::dash)
    last_pressed_state = LastPressedState::none;
  dot_paddle_state = dot_pressed ? PaddleState::on : PaddleState::off;
  dash_paddle_state = dash_pressed ? PaddleState::on : PaddleState::off;

  /* do we need to initiate another dot or dash or set the memory? */
  if constexpr (iambic) {
    if ((dot_pressed || mem == Mem::dot) && dot_send_state == SendState::off) {
      if (dash_send_state == SendState::off
          && (!dash_pressed || mem == Mem::dot))
        dot_send_state = SendState::pending;
      else if (mem == Mem::none)
        mem = Mem::dot;
    }
    if ((dash_pressed || mem == Mem::dash) && dash_send_state == SendState::off) {
      if (dot_send_state == SendState::off
          && (!dot_pressed || mem == Mem::dash))
        dash_send_state = SendState::pending;
      else if (mem == Mem::none)
        mem = Mem::dash;
    }
  }
  else {  
    if ((dot_pressed || mem == Mem::dot) && dot_send_state == SendState::off) {
      if (dash_send_state == SendState::off
          && (last_pressed_state != LastPressedState::dash || mem == Mem::dot))
        dot_send_state = SendState::pending;
      else if (last_pressed_state == LastPressedState::dot
               && mem == Mem::none)
        mem = Mem::dot;
    }
    if ((dash_pressed || mem == Mem::dash) && dash_send_state == SendState::off) {
      if (dot_send_state == SendState::off
          && (last_pressed_state != LastPressedState::dot || mem == Mem::dash))
        dash_send_state = SendState::pending;
      else if (last_pressed_state == LastPressedState::dash
               && mem == Mem::none)
        mem = Mem::dash;
    }
  }

  /* send any new dot or dash at the correct space interval */
  if (dot_send_state == SendState::pending
      && (!autospace
          || (!counter && spaces == 1)
          || (spaces >= 3 && spaces < 5)
          || (spaces >= 7))) {
    if (mem == Mem::dot) mem = Mem::none;
    dot_send_state = SendState::on;
    set_keydown();
    counter = 0;
    spaces = 0;
  }
  else if (dash_send_state == SendState::pending
           && (!autospace
               || (!counter && spaces == 1)
               || (spaces >= 3 && spaces < 5)
               || (spaces >= 7))) {
    if (mem == Mem::dash) mem = Mem::none;
    dash_send_state = SendState::on;
    set_keydown();
    counter = 0;
    spaces = 0;
  }

  /* Complete the sending of a dot or dash and its trailing space and
     any additional spaces required for autospacing */
  else {
    ++counter;
    if (dot_send_state == SendState::on) {
      if (counter >= bit_length) {
        dot_send_state = SendState::space;
        counter = 0;
        set_keyup();
      }
    }
    else if (dot_send_state == SendState::space) {
      if (counter >= bit_length) {
        dot_send_state = SendState::off;
        if (autospace) spaces = 1;
        counter = 0;
      }
    }
    else if (dash_send_state == SendState::on) {
      if (counter >= bit_length * 3) {
        dash_send_state = SendState::space;
        counter = 0;
        set_keyup();
      }
    }
    else if (dash_send_state == SendState::space) {
      if (counter >= bit_length) {
        dash_send_state = SendState::off;
        if (autospace) spaces = 1;
        counter = 0;
      }
    }
    else if (counter >= bit_length) {
        ++spaces;
        counter = 0;
    }
    
    if (spaces > 32766) spaces = 7; // avoid overrun
  }
  
  delay(1);
}
