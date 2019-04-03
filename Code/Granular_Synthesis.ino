/* Granular Synthesis for Arduino Musical Effects Pedal
 * by Asher Toback
 * January 2017
 * https://github.com/Toback/ArduinoEffectsPedal
 * -----------------------------------------------------------------------
 * Implements the granular synthesis effect on an Arduino Uno. Does this 
 * by creating two arrays called input and output buffers. The input 
 * buffer stores incoming audio signals captured by the Arduino while the
 * output buffer simultaneously sends its contents to a DAC integrated 
 * circuit to be played by a connected speaker. Once there is no more 
 * room in the input buffer to record more audio, the input and output 
 * buffers trade places, so that the stored audio can then be sent out to
 * a speaker and more audio can be stored.
 * 
 * The effect that makes this granular synthesis is that the stored signal 
 * in the output buffer isn't sent out directly to the speaker, but 
 * instead a sub-section of the buffer, called a grain, is looped over
 * repeatedly until the input buffer is full. Thus, not all of the recorded 
 * audio in a buffer played, and instead only certain parts of the stored
 * signal are repeated over and over. This creates a hiccuping, 
 * tearing-like sound that is unique to granular synthesis. See the .wav 
 * files in the 'Results' folder on GitHub for an example.
 * 
 * The following potentiometers, called pots, are tunable by the user in 
 * real time 
 * wet/dry: amount of altered signal you want vs. direct signal. 
 *          They both must add to 16 and be between 0 and 16.
 * grain size: A way to have variable buffer length. Grain of 500 means
 *             you take 500 samples before you start outputting the
 *             result to the DAC. 
 * pitch shift: We must index through out arrays to output the 8-bit 
 *              values to our DAC. This controls how quickly we index
 *              through the array. scale == 10 means output at the same
 *              rate which you input. scale > 10 means you go slower,
 *              where 20 is exactly half the speed frequency of 10.
 *              scale < 10 means you index more quickly, and thus 
 *              increase the frequency. 
 *
 * Code inspiration from Amanda Ghassaei 
 * http://www.instructables.com/id/Arduino-Vocal-Effects-Box/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

//audio storage
byte buffer1[500];
byte buffer2[500];

//buffer recording variables
boolean toggle = 0;
boolean rec = 1;

//pot checking storage
int scale = 10;
int scalePot;
int newScale = scale;
byte multiplier = 0;
int grain = 150;
int grainPot;
int newGrain = grain;
int wetDryPot;
int wet = 16;
int dry = 16-wet;
int newWetDry = wet;

//data retrieval variables
unsigned int i = 0;//index variable. 
		   //Should be thought of as time
int iscale = 0;//index variable
int iscalerev = grain-1;

//clipping indicator variables
boolean clipping;
int clippingCounter = 5000;

//reverse switch variables
boolean forward = 0;
boolean newForward = forward;

#define outputSelector 8
#define CS 9
#define WR 10

void setup(){
  DDRD=0xFE;//set digital pins 0-7 as outputs
  DDRB=0xFD;//set digital pins 10-13 as outputs, 9 as input
  DDRC=0x00;//set all analog pins as inputs
  
  pinMode(outputSelector,OUTPUT);
  pinMode(CS,OUTPUT);
  pinMode(WR,OUTPUT);
  
  //select output (LOW for DACA and HIGH for DACB)
  digitalWrite(outputSelector,LOW);
  
  //set CS and WR low to let incoming data from digital pins 0-7 go to DAC output
  digitalWrite(CS,LOW);
  digitalWrite(WR,LOW);
  cli();//diable interrupts
  
  //set up continuous sampling of analog pin 0
  
  //clear ADCSRA and ADCSRB registers
  ADCSRA = 0;
  ADCSRB = 0;
  
  ADMUX = 0;//Clear ADMUX register
  ADMUX |= (1 << REFS0); //set reference voltage
  ADMUX |= (1 << ADLAR); //left align the ADC value- so I can read highest 
                         //8 bits from ADCH register only since I'm reading
                         //A0, I don't need to specify which analog pin I
                         //want to read from (0 is default)
  
  ADCSRA |= (1 << ADPS2) | (1 << ADPS0); //set ADC clock with 32 prescaler
					 //16mHz/32=500kHz
  ADCSRA |= (1 << ADATE); //enabble auto trigger
  ADCSRA |= (1 << ADIE); //enable interrupts when measurement complete
  ADCSRA |= (1 << ADEN); //enable ADC
  ADCSRA |= (1 << ADSC); //start ADC measurements
  
  sei();//enable interrupts
}



ISR(ADC_vect) {//when new ADC value ready
  
  //If we still need to store values in our buffer...
  if (rec){
    //Check which buffer is active...
    if (toggle){
      buffer1[i] = ADCH;//store incoming
      if (ADCH == 0 || ADCH == 1023){//if clipping
        PORTB |= 32;//set pin 13 high
        clippingCounter = 5000;//reset clipping counter
        clipping = 1;//currently clipping
      }
      if (forward){//forward
        //mix wet and dry signals. ADCH is exactly what we just
        //received, thus it's the dry signal. buffer2 is what we
        //recorded previously, and what we're altering.
        PORTD = (wet*int(buffer2[iscale])+dry*int(ADCH))>>4;//send out DAC
      }
      else{//reverse
        PORTD = (wet*int(buffer2[iscalerev])+dry*int(ADCH))>>4;//send out DAC
      }
    }
    else{
      buffer2[i] = ADCH;//store incoming
      if (ADCH == 0 || ADCH == 1023){//if clipping
        PORTB |= 32;//set pin 13 high
        clippingCounter = 5000;//reset clipping counter
        clipping = 1;//currently clipping
      }
      if (forward){
        PORTD = (wet*int(buffer1[iscale])+dry*int(ADCH))>>4;//send out DAC
      }
      else{
        PORTD = (wet*int(buffer1[iscalerev])+dry*int(ADCH))>>4;//send out DAC
      }
    }
  }
  else{
    if (toggle){
      if (forward){
        PORTD = (wet*int(buffer2[iscale])+dry*int(ADCH))>>4;//send out DAC
      }
      else{
        PORTD = (wet*int(buffer2[iscalerev])+dry*int(ADCH))>>4;//send out DAC
      }
    }
    else{
      if (forward){
        PORTD = (wet*int(buffer1[iscale])+dry*int(ADCH))>>4;//send out DAC
      }
      else{
        PORTD = (wet*int(buffer1[iscalerev])+dry*int(ADCH))>>4;//send out DAC
      }
    }
  }
  i++;//increment i
  
  //Multiplier is how many times we've gone through our grain. We
  //do this instead of an if statement for iscale greater than our
  //grain because it's faster.
  iscale = i*10/scale-grain*multiplier; 
  iscalerev = grain-iscale-1;
  if (i==grain){ //Stop recording if we get to the end of our grain
                 //with the actual index (aka time).
    rec = 0;//stop recording
  }

/*
If i is greater than grain*scale/10 either :
1.(when scale>=10) When scale is greater than 10 then we're 
outputting a lower frequency than the original, which means we're
taking longer to output than to receive signals. Thus, we've 
already stopped recording data into our input buffer so we need to
switch buffers because we've finished outputting our output buffer. 
2. (when scale<10) We're now indexing through our output buffer
quicker than we're filling our input buffer. So, we need to loop
through our grain a few times before we switch buffers. So, whenever
iscale gets to the end of our grain (iscale>=grain) we increment
our multiplier and set iscale back to 0. from then on we constantly
need to check if our input buffer is full (i==grain), when it is
we have to switch buffers.
*/
  if (i>=(grain*scale/10)){ //We've reached or gone passed the end
                            //of our scaled grain. 
    if (scale<10){ //If our altered frequency is higher pitched,
                   //we need to check if either our scaled index
                   //our regular index have reached the end of our
                   //grain.
      if (i==grain){ //If our actual index has gone through the 
                     //whole grain then we need to switch buffers.
        i = 0;
        iscale = 0;
        iscalerev = grain-1;
        forward = newForward;//update direction
        scale = newScale;//update scale
        grain = newGrain;//update grain
        dry = newWetDry;//update wet dry
        wet = 16-dry;
        toggle ^= 1;//,,try removing this
        rec = 1;
        multiplier = 0;
      }
      else if (iscale>=grain){ //If only our scaled index has
                               //has reached the end of our grain
                               //then we need to loop back on our
                               //grain and output more of it while
                               //we fill the rest of our input 
                               //buffer. 
        iscale = 0;
        iscalerev = grain-1;
        multiplier++;
      }
    }
    else{ //scale >=10 Thus, we're outputting an equal or 
          //lower frequency. Thus, we must've indexed through our 
          //whole grain and so we switch buffers
      i = 0;
      iscale = 0;
      iscalerev = grain-1;
      forward = newForward;//update direction
      scale = newScale;//update scale
      grain = newGrain;//update grain
      dry = newWetDry;//update wet dry
      wet = 16-dry;
      toggle ^= 1;//try removing this
      rec = 1;
      multiplier = 0;
    }
  }
  if (clipping){
    clippingCounter--;//decrement clipping counter
  }
}

//We divide by newScale so must add at least 1 to ensure no
//division by 0.
void checkScale(){
  PORTB &= 251;//set pin 10 low
  scalePot = 0;
  while(digitalRead(10)){
    scalePot++;
  }
  newScale = scalePot+2;
}

//Grain sizes range from 25 to 500 and are in increments of 25.
void checkGrainSize(){
  PORTB &= 247;//set pin 11 low
  grainPot = 0;
  while(digitalRead(11)){
    grainPot++;
  }
  if (grainPot < 1){
    grainPot = 1;
  }
  else if (grainPot > 20){
    grainPot = 20;
  }
  newGrain = grainPot*25;
}

//wet and dry must individually be between 0 and 16,
//and must sum to 16.
void checkWetDry(){
  PORTB &= 239;//set pin 12 low
  wetDryPot = 0;
  while(digitalRead(12)){
    wetDryPot++;//from 0 to ~185
  }
  if (wetDryPot < 10){
    wetDryPot = 0;
  }
  else if (wetDryPot > 160){
    wetDryPot = 160;
  }
  newWetDry = wetDryPot/10;//scale down to 16
}

void checkRev(){//check reverse switch
  if (digitalRead(9)){
    newForward = 1;
  }
  else{
    newForward = 0;
  }
}

void loop(){
  if (clipping){//if currently clipping
    if (clippingCounter<=0){//if enough time has passed since clipping
      clipping = 0;//not currently clipping
      PORTB &= 223;//turn off clipping led indicator (pin 13)
    }
  }
  DDRB=0xFD;//set pins 10-12 as outputs
  PORTB |= 28;//set pins 10-12 high
  delay(1);//wait for capacitor to discharge
  checkRev();//check reverse switch
  DDRB= 0xE1;//set pins 10-12 as inputs
  checkScale();//check if user adjusted pots
  checkGrainSize();//check if user adjusted pots
  checkWetDry();//check if user adjusted pots
}

