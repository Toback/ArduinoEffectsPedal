#define outputSelector 8
#define CS 9
#define WR 10
#define SIZE 2000

//audio storage
byte buffer1[SIZE];

//buffer recording variables
boolean loading = true;

//pot checking storage
int wetDryPot;
int wet = 16;
int dry = 16-wet;
int newWetDry = wet;
int reverbPot = 0;

//data retrieval variables
unsigned int i = 0;//index variable
unsigned int iReverb = 0;//reverb variable

void setup(){
  //Serial.begin(115200); 
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

  
  cli();//disable interrupts
  
  //set up continuous sampling of analog pin 0
  
  //clear ADCSRA and ADCSRB registers
  ADCSRA = 0;
  ADCSRB = 0;
  
  ADMUX = 0;//Clear ADMUX register
  ADMUX |= (1 << REFS0); //set reference voltage
  ADMUX |= (1 << ADLAR); //left align the ADC value- so I can read highest 8 bits from ADCH register only
  //since I'm reading A0, I don't need to specifiy which analog pin I want to read from (0 is default)
  
  ADCSRA |= (1 << ADPS2) | (1 << ADPS0); //set ADC clock with 32 prescaler- 16mHz/32=500kHz
  ADCSRA |= (1 << ADATE); //enabble auto trigger
  ADCSRA |= (1 << ADIE); //enable interrupts when measurement complete
  ADCSRA |= (1 << ADEN); //enable ADC
  ADCSRA |= (1 << ADSC); //start ADC measurements
  
  sei();//enable interrupts
}

ISR(ADC_vect) {//when new ADC value ready
  
  if(loading == true)
  {
    buffer1[i] = ADCH;
    i++;
    if(i>=SIZE)
    {
      //Serial.print(iReverb);
      //Serial.println();
      i=0;
      iReverb=10;
      loading=false;
    }
  }
  else
  {
    buffer1[i] = ADCH;
    //Serial.print(iReverb);
    PORTD = (wet*int(buffer1[iReverb]) + dry*int(ADCH))>>4;
    i=(i+1)%SIZE;
    iReverb=(iReverb+1)%SIZE;
  }
}

void loop(){
  cli();//disable interrupts
  delayMicroseconds(1000);
  sei();//enable interrupts
}

void checkReverb(){
  //PORTB &= 247;//set pin 11 low
  reverbPot = 0;
  while(digitalRead(11)){
    reverbPot++;//from 0 to ~185
  }
  if (reverbPot < 10){
    reverbPot = 0;
  }
  else if (reverbPot > 160){
    reverbPot = 160;
  }
  iReverb = (i + reverbPot*9)%SIZE;
}

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


