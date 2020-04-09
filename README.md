Introduction
------------

The keyer.ino file contains the sketch code for for the G3XXF morse
keyer for the Arduino.  It has been tested with the Arduino Uno and
the arduino-1.8.12 development kit.  It will take either single or
dual lever paddles.  If used with dual lever paddles, by default it
operates in "last-pressed" mode rather than iambic mode.

Last-pressed mode
-----------------

Last-pressed mode means that if both dot and dash paddles are pressed
at the same time, the last paddle pressed will be the one which sends.
This is coupled with a memory so that if the dot paddle is pressed and
released while a dash is being sent, a dot will be entered into memory
and will be sent after the dash has finished (and similarly if the
dash paddle is pressed while a dot is being sent).

Iambic mode
-----------

As an alternative, the program can be configured so that the keyer
operates in iambic mode, whereby the dots and dashes alternate when
both levers are pressed.  When in iambic mode, the memory facility
operates differently than in last-pressed mode, to match the behaviour
of most iambic keyers.  In last-pressed mode, the memory is edge
triggered - a dot or dash is placed in memory when a key is pressed:
after that, it becomes the "last-pressed" key and takes precedence
until the other paddle is released and pressed again.  However, in
iambic mode, if both paddles are pressed at the same time, a dot is
entered into memory as soon as a dash begins, and likewise with a dash
as soon as a dot begins.

In the author's opinion, last-pressed mode is much superior to iambic
mode for serious CW sending when using dual paddles.  Give it a try if
you are not used to it.

With single lever paddles, it makes no difference whether the program
is compiled for last-pressed mode or iambic mode.  Many people prefer
single lever to dual lever paddles, and that is fine.
 
Autospacing
-----------

An autospacing option is also provided.  If autospacing is off, then
any new letter will begin as soon as a paddle is pressed following the
conclusion of the previous letter, in the same way as any ordinary
keyer.  If autospacing is on, some automatic spacing will take place.
In particular, if a paddle is pressed more than 1 space and less than
3 spaces after the previous dot or dash finished, it will be assumed
that a letter space was wanted and the character will not be sent
until 3 spaces have elapsed.  If a paddle is pressed more than 3 and
less than 5 spaces after the previous dot or dash finished, it will be
assumed the operator wanted a letter space but was a little late, so
the next dot or dash will be sent immediately.  If a paddle is pressed
more than 5 spaces and less than 7 spaces after the previous dot or
dash finished, it will be assumed that a word space was wanted and the
character will not be sent until 7 spaces have elapsed.  After 7
spaces, any pressing of a paddle will cause the corresponding dot or
dash to be sent immediately.

Pinouts, etc
------------
 
Using this sketch, the dot paddle should be connected to pin 4 of the
Arduino and the dash paddle to pin 5, with the common line connected
to ground.

Morse output is on pin 8, which is configured in this program as a low
impedance source with 0v (key up) or the working positive voltage (+5v
on the Arduino Uno) (key down): if the transmitter has a positive
keying line (as most modern ones do) then pin 8 can feed the base of
an NPN transistor through a current limiting resistor and the
collector used to key the transmitter.  Most modern transmitters with
a positive key line have modest voltage and current requirements on
that line, and a 10K Ohm resistor to the base of a general purpose NPN
transistor such as a BC547 will often do.  Check to see.

Analogue pin A3 is used for speed control.  It should be connected to
the wiper of a potentiometer which has 0v and the working positive
voltage (+5v on an Arduino Uno) at its end points.  A 470K Ohm
potentiometer works fine.  It is probably best not to make it too much
higher in resistance or capacitive effects may appear.

The pin numbers can of course be changed to taste.

Paddle debouncing is taken care of.  However, because current through
the paddles is miniscule, it would pay to keep paddle contacts clean.
