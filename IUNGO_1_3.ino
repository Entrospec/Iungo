#include <digitalWriteFast.h>

#include <MIDI.h>  
#include <EEPROM.h>
#include <avr/interrupt.h>
#include <avr/io.h>

MIDI_CREATE_DEFAULT_INSTANCE();

// defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#define Button1 6
#define Button2 5
#define Button3 4
#define Button4 3
#define Button5 2

#define LED1 11
#define LED2 10
#define LED3 9
#define LED4 8
#define LED5 7

#define cvIn A1            // select the input pin for cv in
volatile int cvValue = 0;  // variable to store the value coming from cv In
#define gateIn A0          // input pin for gate
  
byte del = 950;
byte delByte;
volatile unsigned long velocity;

volatile byte Bank[] = {0,0,0,0,0};
volatile byte wait = 0;
volatile byte arpnumber[] = {0,0,0,0,0};

byte locked = 0;

byte i;
byte j;
byte k;
byte l;
byte m;
byte n;
byte q;

unsigned long prevMillis = 0;
unsigned long start_time = 0;
unsigned long time_slice1 = 0;
unsigned long time_slice2 = 0;

volatile unsigned long counter = 0;

byte started = 0;
volatile int shift = 1;
int nudge = 0;
byte nudged = 0;
byte nudgeLED = 0;
//byte clockMode = 1;
int interval = 250;             // interval at which to blink (milliseconds)
int ledState = LOW;             // ledState used to set the LED
byte mod[] = {1, 1};            // mod[0]: 1 = chord, 2 = seq, 3 = polyseq, 4 = clock, 5 = vapoly

byte Switch[] = {0,0,0,0,0};


byte Release = 0;
byte hold = 0;

byte trans[] = {0,0,0,0,0,0};
byte transchan[] = {0,0,0,0,0,0};

byte MidiNotes1[] = {128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128};
byte MidiNotes2[] = {128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128};
byte MidiNotes3[] = {128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128};
byte MidiNotes4[] = {128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128};
byte MidiNotes5[] = {128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128};

byte MidiNotes[] = {128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128};

byte channels1[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  
byte channels2[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};   
byte channels3[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  
byte channels4[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  
byte channels5[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  

byte pchange1[] = {128, 128};
byte pchange2[] = {128, 128};
byte pchange3[] = {128, 128};
byte pchange4[] = {128, 128};
byte pchange5[] = {128, 128};

volatile byte program[] = {128,128};

byte newnote = 0;

byte length[] = {0,0,0,0,0};
  
volatile byte transpose = 0;
volatile byte note[] = {128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128};

byte Mutes[] = {1, 1, 1, 1, 1};

byte noteByte;
byte velocityByte;
byte channel;

void setup() {

  cli();
  
  MIDI.turnThruOn();  

  sbi (ADCSRA, ADPS2);
  sbi (ADCSRA, ADPS1);
  cbi (ADCSRA, ADPS0);

  DDRC = (0<<PORTC1);

  Serial.begin(31250);
 
  pinMode (Button1, INPUT); 
  pinMode (Button2, INPUT); 
  pinMode (Button3, INPUT); 
  pinMode (Button4, INPUT); 
  pinMode (Button5, INPUT); 
 
  pinMode (LED1, OUTPUT);
  pinMode (LED2, OUTPUT);
  pinMode (LED3, OUTPUT);
  pinMode (LED4, OUTPUT);
  pinMode (LED5, OUTPUT);
 
  MIDI.begin(MIDI_CHANNEL_OMNI);  // Initialize the Midi Library.
  
  PCICR |= (1<<PCIE1);
  PCMSK1 |= (1<<PCINT8);
  MCUCR = (1<<ISC00)|(1<<ISC01);
  sei();
  
  locked = EEPROM.read(1021);
  if (locked == 0xff){
    locked = 1;
  }
  
  mod[0] = EEPROM.read(1022);
  if (mod[0] == 0xff){
    mod[0] = 1;
  }
}


void readMIDI(int y){
  counter = 0;
  noteByte = 0;
  velocityByte = 1;
  channel = 0;
  byte tran;
  byte MidiNotes[] = {128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128};
  byte channels[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  byte pchange[] = {128, 128};
  byte press[] = {0,0,0,0,0};
  byte skip = 0;
  int count = 0;
  int LED;
  int Button;
 
  int LongPress = 0;
 
  long previousMillis = 0;         

  ledState = LOW;


  if (y==1){
    LED = LED1;
    Button = Button1;
  }
  else if (y==2){
    LED = LED2;
    Button = Button2;
  }
  else if (y==3){
    LED = LED3;
    Button = Button3;
  }
  else if (y==4){
    LED = LED4;
    Button = Button4;
  }
  else if (y==5){
    LED = LED5;
    Button = Button5;
  }
 
  do{
   
    if((y != 1) && (Bank[0] != 1) && (skip != 1)){
      digitalWriteFast (LED1, LOW);
    }
    if((y != 2) && (Bank[1] != 1) && (skip != 2)){
      digitalWriteFast (LED2, LOW);
    }
    if((y != 3) && (Bank[2] != 1) && (skip != 3)){
      digitalWriteFast (LED3, LOW);
    }
    if((y != 4) && (Bank[3] != 1) && (skip != 4)){
      digitalWriteFast (LED4, LOW);
    }
    if((y != 5) && (Bank[4] != 1) && (skip != 5)){
      digitalWriteFast (LED5, LOW);
    }
    
  
    if(y==1 && (LongPress == 4) && (Release == 0)){
      LongPress = 0;
      Release = 5;
      count = 0;
      load();
    }
    if(y==2 && (LongPress == 4) && (Release == 0)){
      LongPress = 0;
      Release = 5;
      count = 0;
      save();
    }
    if(y==3 && (LongPress == 4) && (Release == 0)){
      LongPress = 0;
      Release = 5;
      count = 0;
      Clear();
    }
    if(y==4 && (LongPress == 4) && (Release == 0)){
      LongPress = 0;
      Release = 5;
      count = 0;
      mute();
    }
    if(y==5 && (LongPress == 4) && (Release == 0)){
      LongPress = 0;
      Release = 5;
      count = 0;
      mode();
    }
 
    if((LongPress == 4) && (Release != 0)){
      digitalWriteFast(LED, LOW);
      Release = 5;
    }
 
      if (digitalReadFast(Button1) == HIGH){
        press[0] = 1;
      }
      else {
        press[0] = 0;
      }
      if (digitalReadFast(Button2) == HIGH){
        press[1] = 1;
      }
      else {
        press[1] = 0;
      }
      if (digitalReadFast(Button3) == HIGH){
        press[2] = 1;
      }
      else {
        press[2] = 0;
      }
      if (digitalReadFast(Button4) == HIGH){
        press[3] = 1;
      }
      else {
        press[3] = 0;
      }
      if (digitalReadFast(Button5) == HIGH){
        press[4] = 1;
      }
      else {
        press[4] = 0;
      }
     // if (mod[0] > 2){
      if (press[0] == 1 && press[4] == 1){
        if (mod[0] > 2){
          readpolymidi(1,5);
        }
        Release = 5;
      }
      if (press[0] == 1 && press[3] == 1){
        if (mod[0] > 2){
          readpolymidi(1,4);
        }
        Release = 5;
      }
      if (press[0] == 1 && press[2] == 1){
        if (mod[0] > 2){
          readpolymidi(1,3);
        }
        Release = 5;
      }
      if (press[0] == 1 && press[1] == 1){
        if (mod[0] > 2){
          readpolymidi(1,2);
        }
        Release = 5;
      }
      if (press[1] == 1 && press[4] == 1){
        if (mod[0] > 2){
          readpolymidi(2,5);
        }
        Release = 5;
      }
      if (press[1] == 1 && press[3] == 1){
        if (mod[0] > 2){
          readpolymidi(2,4);
        }
        Release = 5;
      }
      if (press[1] == 1 && press[2] == 1){
        if (mod[0] > 2){
          readpolymidi(2,3);
        }
        Release = 5;
      }
      if (press[2] == 1 && press[4] == 1){
        if (mod[0] > 2){
          readpolymidi(3,5);
        }
        Release = 5;
      }
      if (press[2] == 1 && press[3] == 1){
        if (mod[0] > 2){
          readpolymidi(3,4);
        }
        Release = 5;
      }
      if (press[3] == 1 && press[4] == 1){
        if (mod[0] > 2){
          readpolymidi(4,5);
        }
        Release = 5;
      }
   // }
    
    if ((digitalReadFast(Button1) == HIGH) && (y != 1) && (count > 2) && (skip == 0)){
      digitalWriteFast(LED1, HIGH);
      count = 0;
      skip = 1;
      MidiNotes[counter] = 129;
      counter += 1;
    }
 
    if ((digitalReadFast(Button2) == HIGH) && (y != 2) && (count > 2) && (skip == 0)){
      digitalWriteFast(LED2, HIGH);
      count = 0;
      skip = 2;
      MidiNotes[counter] = 129;
      counter += 1;
    }
 
    if ((digitalReadFast(Button3) == HIGH) && (y != 3) && (count > 2) && (skip == 0)){
      digitalWriteFast(LED3, HIGH);
      count = 0;
      skip = 3;
      MidiNotes[counter] = 129;
      counter += 1;
    }
 
    if ((digitalReadFast(Button4) == HIGH) && (y != 4) && (count > 2) && (skip == 0)){
      digitalWriteFast(LED4, HIGH);
      count = 0;
      skip = 4;
      MidiNotes[counter] = 129;
      counter += 1;
    }
 
    if ((digitalReadFast(Button5) == HIGH) && (y != 5) && (count > 2) && (skip == 0)){
      digitalWriteFast(LED5, HIGH);
      count = 0;
      skip = 5;
      MidiNotes[counter] = 129;
      counter += 1;
    }
 
    if ((skip == 1) && (count > 2)){
      if(digitalReadFast(Button1) == LOW){
        digitalWriteFast(LED1, LOW);
        skip = 0;
      }
    }
 
    if ((skip == 2) && (count > 2)){
      if(digitalReadFast(Button2) == LOW){
        digitalWriteFast(LED2, LOW);
        skip = 0;
      }
    }
   
    if ((skip == 3) && (count > 2)){
      if(digitalReadFast(Button3) == LOW){
        digitalWriteFast(LED3, LOW);
        skip = 0;
      }
    }
 
    if ((skip == 4) && (count > 2)){
      if(digitalReadFast(Button4) == LOW){
        digitalWriteFast(LED4, LOW);
        skip = 0;
      }
    }
 
    if ((skip == 5) && (count > 2)){
      if(digitalReadFast(Button5) == LOW){
        digitalWriteFast(LED5, LOW);
        skip = 0;
      }
    }
 
    unsigned long currentMillis = millis();
    if((currentMillis - previousMillis > interval) && Release < 3) {
    // save the last time you blinked the LED
      previousMillis = currentMillis;  
      count += 1;
      if (digitalReadFast(Button) == HIGH){
        LongPress += 1;
      }

      if (ledState == LOW)
        ledState = HIGH;
      else
        ledState = LOW;
        
      digitalWriteFast(LED, ledState);
    } 
   

    if ((digitalReadFast(Button) == LOW) && (Release < 2)){
      Release = 1;
      LongPress = 0;
    }
    if ((digitalReadFast(Button) == HIGH) && (Release == 1)){
      Release = 2;
    }
    if ((digitalReadFast(Button) == LOW) && (Release == 2)){
      Release = 3;
    }
  
    if(counter > 15){
      Release = 3;
    }
  
    if (MIDI.read()) {
      MidiIn();
      byte type = MIDI.getType();
      switch (type) {
        case midi::NoteOn:
        noteByte = MIDI.getData1();
        velocityByte = MIDI.getData2();
        channel = MIDI.getChannel();
        
        if (velocityByte != 0){
          MidiNotes[counter] = noteByte;
          channels[counter] = channel;
          counter += 1;
          
        }
      
        break;
      
        case midi::ProgramChange:
        pchange[0] = MIDI.getData1(); 
        pchange[1] = MIDI.getChannel();
      
        break;
      }
    } 
    noteByte = 0;
    velocityByte = 1;
    channel = 0;
      
    if(wait == 2){
       MidiOff();
    }
  }
  while (Release < 3);

  if (Release != 5){
    if (y==1){
      if (pchange[0] != 128){
        if ((pchange1[0] == pchange[0]) && (pchange1[1] == pchange[1])){
          pchange1[0] = 128;
          pchange1[1] = 128;
        }
        else{ 
          pchange1[0] = pchange[0];
          pchange1[1] = pchange[1];
        }
      }
      if (MidiNotes[0] != 128){
        if (Bank[0] == 1){
          for (i=0; i < 16; i += 1){
            MIDI.sendNoteOff(note[i],127,channels1[i]);
          } 
        }
        for (i = 0; i < 16; i += 1){                  
          MidiNotes1[i] = MidiNotes[i];
          channels1[i] = channels[i];
          if (MidiNotes[i] == 128){
            length[0] = (i - 1);
            if (arpnumber[0] > length[0]){
              arpnumber[0] = 0;
            }
            
            for (i = i; i < 16; i += 1){
              MidiNotes1[i] = MidiNotes[i];
              channels1[i] = channels[i];
            }
          }
          else length[0] = 15;
        }
      } 
  
      else if (Release != 4){
  
        if (mod[0] == 3){
          for (i = 0; i < 16; i += 1){
            if ((MidiNotes1[i] < 128) && (transchan[0] == channels1[i])){
              MidiNotes1[i] = (MidiNotes1[i] - transpose);
            }
            if ((MidiNotes2[i] < 128) && (transchan[0] == channels2[i])){
              MidiNotes2[i] = (MidiNotes2[i] - transpose);
            }
            if ((MidiNotes3[i] < 128) && (transchan[0] == channels3[i])){
              MidiNotes3[i] = (MidiNotes3[i] - transpose);
            }
            if ((MidiNotes4[i] < 128) && (transchan[0] == channels4[i])){
              MidiNotes4[i] = (MidiNotes4[i] - transpose);
            }
            if ((MidiNotes5[i] < 128) && (transchan[0] == channels5[i])){
              MidiNotes5[i] = (MidiNotes5[i] - transpose);
            }
          }
        }
        else{
          tran = MidiNotes1[0] - trans[0];  
          for (i = 0; i < 16; i += 1){
            if ((MidiNotes1[i] < 128) && (transchan[0] == channels1[i])){
              MidiNotes1[i] = (MidiNotes1[i] - tran);
            }   
          }   
        }
      }
    }

    else if (y==2){
      if (pchange[0] != 128){
        if ((pchange2[0] == pchange[0]) && (pchange2[1] == pchange[1])){
          pchange2[0] = 128;
          pchange2[1] = 128;
        }
        else { 
          pchange2[0] = pchange[0];
          pchange2[1] = pchange[1];
        }
      }
      if (MidiNotes[0] != 128){
        if (Bank[1] == 1){
          for (i=0; i < 16; i += 1){
            MIDI.sendNoteOff(note[i],127,channels2[i]);
          } 
        }
        for (i = 0; i < 16; i += 1){
          MidiNotes2[i] = MidiNotes[i];
          channels2[i] = channels[i];
          if (MidiNotes[i] == 128){
            length[1] = (i - 1);
            if (arpnumber[1] > length[1]){
              arpnumber[1] = 0;
            }
            //i = 16;
            for (i = i; i < 16; i += 1){
              MidiNotes2[i] = MidiNotes[i];
              channels2[i] = channels[i];
            }
          }
          else length[1] = 15;
        }
      }
  
      else if (Release != 4){
  
        if (mod[0] == 3){
          for (i = 0; i < 16; i += 1){
            if ((MidiNotes1[i] < 128) && (transchan[0] == channels1[i])){
              MidiNotes1[i] = (MidiNotes1[i] - transpose);
            }
            if ((MidiNotes2[i] < 128) && (transchan[0] == channels2[i])){
              MidiNotes2[i] = (MidiNotes2[i] - transpose);
            }
            if ((MidiNotes3[i] < 128) && (transchan[0] == channels3[i])){
              MidiNotes3[i] = (MidiNotes3[i] - transpose);
            }
            if ((MidiNotes4[i] < 128) && (transchan[0] == channels4[i])){
              MidiNotes4[i] = (MidiNotes4[i] - transpose);
            }
            if ((MidiNotes5[i] < 128) && (transchan[0] == channels5[i])){
              MidiNotes5[i] = (MidiNotes5[i] - transpose);
            }
          }
        }
        else{
          tran = MidiNotes2[0] - trans[0];
          for (i = 0; i < 16; i += 1){
            if ((MidiNotes2[i] < 128) && (transchan[0] == channels2[i])){
              MidiNotes2[i] = (MidiNotes2[i] - tran);
            }   
          } 
        }
      } 
    }

    else if (y==3){
      if (pchange[0] != 128){
        if ((pchange3[0] == pchange[0]) && (pchange3[1] == pchange[1])){
          pchange3[0] = 128;
          pchange3[1] = 128;
        }
        else{ 
          pchange3[0] = pchange[0];
          pchange3[1] = pchange[1];
        }
      }
      if (MidiNotes[0] != 128){
        if (Bank[2] == 1){
          for (i=0; i < 16; i += 1){
            MIDI.sendNoteOff(note[i],127,channels3[i]);
          } 
        }
        for (i = 0; i < 16; i += 1){
          MidiNotes3[i] = MidiNotes[i];
          channels3[i] = channels[i];
          if (MidiNotes[i] == 128){
            length[2] = (i - 1);
            if (arpnumber[2] > length[2]){
              arpnumber[2] = 0;
            }
            
            for (i = i; i < 16; i += 1){
              MidiNotes3[i] = MidiNotes[i];
              channels3[i] = channels[i];
            }
          }  
          else length[2] = 15;
        }
      }
  
      else if (Release != 4){
  
        if (mod[0] == 3){
          for (i = 0; i < 16; i += 1){
            if ((MidiNotes1[i] < 128) && (transchan[0] == channels1[i])) {
              MidiNotes1[i] = (MidiNotes1[i] - transpose);
            }
            if ((MidiNotes2[i] < 128) && (transchan[0] == channels2[i])){
              MidiNotes2[i] = (MidiNotes2[i] - transpose);
            }
            if ((MidiNotes3[i] < 128) && (transchan[0] == channels3[i])){
              MidiNotes3[i] = (MidiNotes3[i] - transpose);
            }
            if ((MidiNotes4[i] < 128) && (transchan[0] == channels4[i])){
              MidiNotes4[i] = (MidiNotes4[i] - transpose);
            }
            if ((MidiNotes5[i] < 128) && (transchan[0] == channels5[i])){
              MidiNotes5[i] = (MidiNotes5[i] - transpose);
            }
          }
        }
        else {
          tran = MidiNotes3[0] - trans[0]; 
          for (i = 0; i < 16; i += 1){
            if ((MidiNotes3[i] < 128) && (transchan[0] == channels3[i])){
              MidiNotes3[i] = (MidiNotes3[i] - tran);
            }
          }   
        } 
      }
    }

    else if (y==4){
      if (pchange[0] != 128){
        if ((pchange4[0] == pchange[0]) && (pchange4[1] == pchange[1])){
          pchange4[0] = 128;
          pchange4[1] = 128;
        }
        else{
          pchange4[0] = pchange[0];
          pchange4[1] = pchange[1];
        }
      }
      if (MidiNotes[0] != 128){
        if (Bank[3] == 1){
          for (i=0; i < 16; i += 1){
            MIDI.sendNoteOff(note[i],127,channels4[i]);
          } 
        }
        for (i = 0; i < 16; i += 1){
          MidiNotes4[i] = MidiNotes[i];
          channels4[i] = channels[i];
          if (MidiNotes[i] == 128){
            length[3] = (i - 1);
            if (arpnumber[3] > length[3]){
              arpnumber[3] = 0;
            }
            
            for (i = i; i < 16; i += 1){
              MidiNotes4[i] = MidiNotes[i];
              channels4[i] = channels[i];
            }
          }  
          else length[3] = 15;
        }
      }
  
      else if (Release != 4){
  
        if (mod[0] == 3){
          for (i = 0; i < 16; i += 1){
            if ((MidiNotes1[i] < 128) && (transchan[0] == channels1[i])){
              MidiNotes1[i] = (MidiNotes1[i] - transpose);
            }
            if ((MidiNotes2[i] < 128) && (transchan[0] == channels2[i])){
              MidiNotes2[i] = (MidiNotes2[i] - transpose);
            }
            if ((MidiNotes3[i] < 128) && (transchan[0] == channels3[i])){
              MidiNotes3[i] = (MidiNotes3[i] - transpose);
            }
            if ((MidiNotes4[i] < 128) && (transchan[0] == channels4[i])){
              MidiNotes4[i] = (MidiNotes4[i] - transpose);
            }
            if ((MidiNotes5[i] < 128) && (transchan[0] == channels5[i])){
              MidiNotes5[i] = (MidiNotes5[i] - transpose);
            }
          }
        }
        else {
          tran = MidiNotes4[0] - trans[0]; 
          for (i = 0; i < 16; i += 1){
            if ((MidiNotes4[i] < 128) && (transchan[0] == channels4[i])){
              MidiNotes4[i] = (MidiNotes4[i] - tran);
            }  
          }   
        } 
      }
    }

    else if (y==5){
      if (pchange[0] != 128){
        if ((pchange5[0] == pchange[0]) && (pchange5[1] == pchange[1])){
          pchange5[0] = 128;
          pchange5[1] = 128;
        }
        else{ 
          pchange5[0] = pchange[0];
          pchange5[1] = pchange[1];
        }
      }
      if (MidiNotes[0] != 128){
        if (Bank[4] == 1){
          for (i=0; i < 16; i += 1){
            MIDI.sendNoteOff(note[i],127,channels5[i]);
          } 
        }
        for (i=0; i < 16; i += 1){
          MidiNotes5[i] = MidiNotes[i];
          channels5[i] = channels[i];
          if (MidiNotes[i] == 128){
            length[4] = (i - 1);
            if (arpnumber[4] > length[4]){
              arpnumber[4] = 0;
            }
            //i = 16;
            for (i = i; i < 16; i += 1){
              MidiNotes5[i] = MidiNotes[i];
              channels5[i] = channels[i];
            }
          }  
          else length[4] = 15;
        }
      }
  
      else if (Release != 4){
  
        if (mod[0] == 3){
          for (i = 0; i < 16; i += 1){
            if ((MidiNotes1[i] < 128) && (transchan[0] == channels1[i])){
              MidiNotes1[i] = (MidiNotes1[i] - transpose);
            }
            if ((MidiNotes2[i] < 128) && (transchan[0] == channels2[i])){
              MidiNotes2[i] = (MidiNotes2[i] - transpose);
            }
            if ((MidiNotes3[i] < 128) && (transchan[0] == channels3[i])){
              MidiNotes3[i] = (MidiNotes3[i] - transpose);
            } 
            if ((MidiNotes4[i] < 128) && (transchan[0] == channels4[i])){
              MidiNotes4[i] = (MidiNotes4[i] - transpose);
            }
            if ((MidiNotes5[i] < 128) && (transchan[0] == channels5[i])){
              MidiNotes5[i] = (MidiNotes5[i] - transpose);
            }
          }
        }
        else {
          tran = MidiNotes5[0] - trans[0]; 
          for (i = 0; i < 16; i += 1){
            if ((MidiNotes5[i] < 128) && (transchan[0] == channels5[i])){
              MidiNotes5[i] = (MidiNotes5[i] - tran);
            }  
          }   
        } 
      }
    }
  }
}


void load(){
  int x = 800;
  long previousMillis = 0;
  int count = 0;
 
  digitalWriteFast (LED1, HIGH);
  digitalWriteFast (LED2, HIGH);
  digitalWriteFast (LED3, HIGH);
  digitalWriteFast (LED4, HIGH);
  digitalWriteFast (LED5, HIGH);

  do{
    if ((digitalReadFast(Button1) == LOW) && (x == 800)){
      x = 801;
    }
    unsigned long currentMillis = millis();
    if((currentMillis - previousMillis > 250)) {
      previousMillis = currentMillis;  
      count += 1;
    }
    if ((digitalReadFast(Button1) == HIGH) && (x == 801) && (count > 1)){
      x = 0;
      digitalWriteFast(LED1, LOW);
    }
    if ((digitalReadFast(Button2) == HIGH) && (x == 801) && (count > 1)){
      x = 182;
      digitalWriteFast(LED2, LOW);
    }
    if ((digitalReadFast(Button3) == HIGH) && (x == 801) && (count > 1)){
      x = 364;
      digitalWriteFast(LED3, LOW);
    }
    if ((digitalReadFast(Button4) == HIGH) && (x == 801) && (count > 1)){
      x = 546;
      digitalWriteFast(LED4, LOW);
    }
    if ((digitalReadFast(Button5) == HIGH) && (x == 801) && (count > 1)){
      x = 728;
      digitalWriteFast(LED5, LOW);
    }
    if(wait == 2){
      MidiOff();
    }
    if (MIDI.read()) {
      MidiIn();
    }
    if (count > 20){
      x = 666;
    }
  }
  while (x > 799);
      
  if (x != 666){
    MidiOff();
      
    for (i = 0; i < 16; i += 1){
    
      MidiNotes1[i] = EEPROM.read(i+x+0);
      channels1[i] = EEPROM.read(i+x+16);
      MidiNotes2[i] = EEPROM.read(i+x+32);
      channels2[i] = EEPROM.read(i+x+48);
      MidiNotes3[i] = EEPROM.read(i+x+64);
      channels3[i] = EEPROM.read(i+x+80);
      MidiNotes4[i] = EEPROM.read(i+x+96);
      channels4[i] = EEPROM.read(i+x+112);
      MidiNotes5[i] = EEPROM.read(i+x+128);
      channels5[i] = EEPROM.read(i+x+144);
    }
    
    for (i = 0; i < 2; i += 1){
    
      pchange1[i] = EEPROM.read(i+x+160);
      pchange2[i] = EEPROM.read(i+x+162);
      pchange3[i] = EEPROM.read(i+x+164);
      pchange4[i] = EEPROM.read(i+x+166);
      pchange5[i] = EEPROM.read(i+x+168);
      mod[i] = EEPROM.read(i+x+170);
      if (mod[i] == 0xff){                      // No data saved
        mod[i] = 1;
      }
    }
    
    if(mod[1] == 1){
      MIDI.turnThruOn();
    }
    else{
      MIDI.turnThruOff();
    }
    
    for (i = 0; i < 5; i += 1){
      length[i] = EEPROM.read(172+i+x);
      Mutes[i] = EEPROM.read(177+i+x);
      arpnumber[i] = 0;
    }
  }
}


void save(){

  int x = 800;
  long previousMillis = 0;
  int count = 0;
  int i =0;
 
  digitalWriteFast (LED1, HIGH);
  digitalWriteFast (LED2, HIGH);
  digitalWriteFast (LED3, HIGH);
  digitalWriteFast (LED4, HIGH);
  digitalWriteFast (LED5, HIGH);
    

    
  do{
    if ((digitalReadFast(Button2) == LOW) && (x == 800)){
      x = 801;
    }
    unsigned long currentMillis = millis();
    if((currentMillis - previousMillis > 250)) {
      previousMillis = currentMillis;  
      count += 1;
    }
    if ((digitalReadFast(Button1) == HIGH) && (x == 801) && (count > 1)){
      x = 0;
      digitalWriteFast(LED1, LOW);
    }
    if ((digitalReadFast(Button2) == HIGH) && (x == 801) && (count > 1)){
      x = 182;
      digitalWriteFast(LED2, LOW);
    }
    if ((digitalReadFast(Button3) == HIGH) && (x == 801) && (count > 1)){
      x = 364;
      digitalWriteFast(LED3, LOW);
    }
    if ((digitalReadFast(Button4) == HIGH) && (x == 801) && (count > 1)){
      x = 546;
      digitalWriteFast(LED4, LOW);
    }
    if ((digitalReadFast(Button5) == HIGH) && (x == 801) && (count > 1)){
      x = 728;
      digitalWriteFast(LED5, LOW);
    }
    if(wait == 2){
      MidiOff();
    }
    if (MIDI.read()) {
      MidiIn();
    }
    if (count > 20){
      x = 666;
    }
  }
  while (x > 799);
      
  if (x != 666){
    cli();
    
    MidiOff();
        
    for (i = 0; i < 16; i += 1){
      if (MidiNotes1[i] != EEPROM.read(i+x)){
        EEPROM.write((i+x+0), MidiNotes1[i]);
      }
        
      if (channels1[i] != EEPROM.read(i+x+16)){
        EEPROM.write((i+x+16), channels1[i]);
      }
        
      if (MidiNotes2[i] != EEPROM.read(i+x+32)){
        EEPROM.write((i+x+32), MidiNotes2[i]);
      }
        
      if (channels2[i] != EEPROM.read(i+x+48)){
        EEPROM.write((i+x+48), channels2[i]);
      }
        
      if (MidiNotes3[i] != EEPROM.read(i+x+64)){
        EEPROM.write((i+x+64), MidiNotes3[i]);
      }
        
      if (channels3[i] != EEPROM.read(i+x+80)){
        EEPROM.write((i+x+80), channels3[i]);
      }
        
      if (MidiNotes4[i] != EEPROM.read(i+x+96)){
        EEPROM.write((i+x+96), MidiNotes4[i]);
      }
        
      if (channels4[i] != EEPROM.read(i+x+112)){
        EEPROM.write((i+x+112), channels4[i]);
      }
        
      if (MidiNotes5[i] != EEPROM.read(i+x+128)){
        EEPROM.write((i+x+128), MidiNotes5[i]);
      }
        
      if (channels5[i] != EEPROM.read(i+x+144)){
        EEPROM.write((i+x+144), channels5[i]);
      }
    }
       
    for (i = 0; i < 2; i += 1){
        
      if (pchange1[i] != EEPROM.read(i+x+160)){
        EEPROM.write((i+x+160), pchange1[i]);
      }
        
      if (pchange2[i] != EEPROM.read(i+x+162)){
        EEPROM.write((i+x+162), pchange2[i]);
      }
        
      if (pchange3[i] != EEPROM.read(i+x+164)){
        EEPROM.write((i+x+164), pchange3[i]);
      }
        
      if (pchange4[i] != EEPROM.read(i+x+166)){
        EEPROM.write((i+x+166), pchange4[i]);
      }
        
      if (pchange5[i] != EEPROM.read(i+x+168)){
        EEPROM.write((i+x+168), pchange5[i]);
      }
        
      if (mod[i] != EEPROM.read(i+x+170)){
        EEPROM.write((i+x+170), mod[i]);
      }
    }
      
    for (i = 0; i < 5; i += 1){
      if (Mutes[i] != EEPROM.read(177+i+x)){
        EEPROM.write((177+i+x), Mutes[i]); 
      }
      if (length[i] != EEPROM.read(172+i+x)){
        EEPROM.write((172+i+x), length[i]);
      }
      sei();
      
    } 
  }
}


void Clear (){
  byte x = 0;
  byte press[] = {0,0,2,0,0};
  long previousMillis = 0;
  int count = 0;
  digitalWriteFast (LED1, HIGH);
  digitalWriteFast (LED2, HIGH);
  digitalWriteFast (LED3, HIGH);
  digitalWriteFast (LED4, HIGH);
  digitalWriteFast (LED5, HIGH);

  do{ 
    if ((x == 0) && (digitalReadFast(Button3) == LOW)){
      press[2] = 0;
      x = 1;
    }
    unsigned long currentMillis = millis();
    if((currentMillis - previousMillis > 250)) {
      previousMillis = currentMillis;  
      count += 1;
    }
    if ((digitalReadFast(Button1) == HIGH) && (press[0] == 0) && (count > 1)){
      digitalWriteFast(LED1, LOW);
        
      for (i = 0; i < 16; i += 1){
        MidiNotes1[i] = 128;
      }
      for (i = 0; i < 2; i += 1){
        pchange1[i] = 128;
      }
      count = 10;
      press[0] = 1;
    }
    if ((digitalReadFast(Button2) == HIGH) && (press[1] == 0) && (count > 1)){
      digitalWriteFast(LED2, LOW);
        
      for (i = 0; i < 16; i += 1){
        MidiNotes2[i] = 128;
      }
      for (i = 0; i < 2; i += 1){
        pchange2[i] = 128;
      }
      count = 10;
      press[1] = 1;
    }
    if ((digitalReadFast(Button3) == HIGH) && (press[2] == 0) && (count > 1)){
      digitalWriteFast(LED3, LOW);
       
      for (i = 0; i < 16; i += 1){
        MidiNotes3[i] = 128;
      }
      for (i = 0; i < 2; i += 1){
        pchange3[i] = 128;
      }
      count = 10;
      press[2] = 1;
    }
    if ((digitalReadFast(Button4) == HIGH) && (press[3] == 0) && (count > 1)){
      digitalWriteFast(LED4, LOW);
       
      for (i = 0; i < 16; i += 1){
        MidiNotes4[i] = 128;
      }
      for (i = 0; i < 2; i += 1){
        pchange4[i] = 128;
      }
      count = 10;
      press[3] = 1;
    }
    if ((digitalReadFast(Button5) == HIGH) && (press[4] == 0) && (count > 1)){
      digitalWriteFast(LED5, LOW);
     
      for (i = 0; i < 16; i += 1){
        MidiNotes5[i] = 128;
      }
      for (i = 0; i < 2; i += 1){
        pchange5[i] = 128;
      }
      count = 10;
      press[4] = 1;
    }
    if ((press[0] == 1) && (digitalReadFast(Button1) == LOW) && (count > 11)){
      press[0] = 0;
    }
    if ((press[1] == 1) && (digitalReadFast(Button2) == LOW) && (count > 11)){
      press[1] = 0;
    }
    if ((press[2] == 1) && (digitalReadFast(Button3) == LOW) && (count > 11)){
      press[2] = 0;
    }
    if ((press[3] == 1) && (digitalReadFast(Button4) == LOW) && (count > 11)){
      press[3] = 0;
    }
    if ((press[4] == 1) && (digitalReadFast(Button5) == LOW) && (count > 11)){
      press[4] = 0;
    }
    if(wait == 2){
      MidiOff();
    }
    if (MIDI.read()) {
      MidiIn();
    }
    if (count > 16){
      x = 2;
    } 
  }
  while (x != 2);
}

void mute(){
  byte press[] = {0,0,0,2,0};
  long previousMillis = 0;
  int count = 0;
  byte x = 0;
  if (Mutes[0] == 1){
    digitalWriteFast(LED1, HIGH);
  }
  else digitalWriteFast(LED1, LOW);
  if (Mutes[1] == 1){
    digitalWriteFast(LED2, HIGH);
  }
  else digitalWriteFast(LED2, LOW);
  if (Mutes[2] == 1){
    digitalWriteFast(LED3, HIGH);
  }
  else digitalWriteFast(LED3, LOW);
  if (Mutes[3] == 1){
    digitalWriteFast(LED4, HIGH);
  }
  else digitalWriteFast(LED4, LOW);
  if (Mutes[4] == 1){
    digitalWriteFast(LED5, HIGH);
  }
  else digitalWriteFast(LED5, LOW);
  
  do {
    if (Mutes[0] == 0){
      digitalWriteFast(LED1, LOW);
    }
    if (Mutes[1] == 0){
      digitalWriteFast(LED2, LOW);
    }
    if (Mutes[2] == 0){
      digitalWriteFast(LED3, LOW);
    }
    if (Mutes[3] == 0){
      digitalWriteFast(LED4, LOW);
    }
    if (Mutes[4] == 0){
      digitalWriteFast(LED5, LOW);
    }
    if ((digitalReadFast(Button4) == LOW) && (x == 0)){
      press[3] = 0;
      x = 1;
    }
    unsigned long currentMillis = millis();
    if((currentMillis - previousMillis > 250)) {
      previousMillis = currentMillis;  
      count += 1;
    }
  
    if ((digitalReadFast(Button1) == HIGH) && (press[0] == 0) && (count > 1)){
      press[0] = 1; 
      count = 10;   
    }
  
    if ((digitalReadFast(Button2) == HIGH) && (press[1] == 0) && (count > 1)){
      press[1] = 1;
      count = 10;    
    }
  
    if ((digitalReadFast(Button3) == HIGH) && (press[2] == 0) && (count > 1)){
      press[2] = 1; 
      count = 10;  
    }
  
    if ((digitalReadFast(Button4) == HIGH) && (press[3] == 0) && (count > 1)){
      press[3] = 1;
      count = 10; 
    }
  
    if ((digitalReadFast(Button5) == HIGH) && (press[4] == 0) && (count > 1)){
      press[4] = 1;  
      count = 10;
    }
   
    if ((press[0] == 1) && (digitalReadFast(Button1) == LOW)){
      if (Mutes[0] == 0){
        Mutes[0] = 1;
        digitalWriteFast(LED1, HIGH);
      }
      else {
        Mutes[0] = 0; 
        digitalWriteFast(LED1, LOW);   
      }
      press[0] = 0;
      count = 1;
    }
   
    if ((press[1] == 1) && (digitalReadFast(Button2) == LOW)){
      if (Mutes[1] == 0){
        Mutes[1] = 1;
        digitalWriteFast(LED2, HIGH);
      }
      else {
        Mutes[1] = 0;
        digitalWriteFast(LED2, LOW);      
      }
      press[1] = 0;
      count = 1;
    }
   
    if ((press[2] == 1) && (digitalReadFast(Button3) == LOW)){
      if (Mutes[2] == 0){
        Mutes[2] = 1;
        digitalWriteFast(LED3, HIGH);
      }
      else {
        Mutes[2] = 0; 
        digitalWriteFast(LED3, LOW);
      }
      press[2] = 0;
      count = 1;
    }
   
    if ((press[3] == 1) && (digitalReadFast(Button4) == LOW)){
      if (Mutes[3] == 0){
        Mutes[3] = 1;
        digitalWriteFast(LED4, HIGH);
      }
      else {
        Mutes[3] = 0;
        digitalWriteFast(LED4, LOW);
      }
      press[3] = 0;
      count = 1;
    }
   
    if ((press[4] == 1) && (digitalReadFast(Button5) == LOW)){
      if (Mutes[4] == 0){
        Mutes[4] = 1;
        digitalWriteFast(LED5, HIGH);
      }
      else {
        Mutes[4] = 0;
        digitalWriteFast(LED5, LOW);
      }
      press[4] = 0;
      count = 1;
    }
   
    if(wait == 2){
      MidiOff();
    }
    if (MIDI.read()) {
      MidiIn();
    }
    if ((press[3] == 1) && (count > 13)){
      x = 5;
    }  
  }

  while (x != 5);
}


void mode(){
  byte press[] = {0,0,0,0,2};
  byte x = 0;
  int count = 0;
  long previousMillis = 0;
  ledState = HIGH;
  digitalWriteFast(LED5, LOW);
  if (mod[0] == 1){                        //Chord
    digitalWriteFast(LED1,HIGH);
    digitalWriteFast(LED2,LOW);
  }
  if (mod[0] == 2){                        //Seq
    digitalWriteFast(LED2,HIGH);
    digitalWriteFast(LED1,LOW);
  }
  if ((mod[0] == 3) || (mod[0] == 5)){     //PolySeq or vaPolyseq
    digitalWriteFast(LED1,HIGH);
    digitalWriteFast(LED2,HIGH);
  }
  if (mod[0] == 4){                        //Clock
    digitalWriteFast(LED1,HIGH);
    digitalWriteFast(LED2,HIGH);
    digitalWriteFast(LED3,HIGH);
    digitalWriteFast(LED4,HIGH);
    digitalWriteFast(LED5,HIGH);
  }
  if (mod[0] != 4){
    if (mod[1] == 1){                      //ThruOn
      digitalWriteFast(LED3,HIGH);
      digitalWriteFast(LED4,LOW);
      digitalWriteFast(LED5,LOW);
    }
    if (mod[1] == 2){                      //ThruOff
      digitalWriteFast(LED4,HIGH);
      digitalWriteFast(LED3,LOW);
      digitalWriteFast(LED5,LOW);
    }
    if (mod[1] == 3){                      //Transpose
      digitalWriteFast(LED5,HIGH);
      digitalWriteFast(LED4,LOW);
      digitalWriteFast(LED3,LOW);
    }
  }
  do{
    if ((digitalReadFast(Button5) == LOW) && (x == 0)){
      x = 1;
      press[4] = 0;
    }
    unsigned long currentMillis = millis();
    if((currentMillis - previousMillis > 250)) {
      previousMillis = currentMillis;  
      count += 1;
      if (mod[0] == 5){
        if (ledState == HIGH){
          ledState = LOW;
        }
        else ledState = HIGH;
        digitalWriteFast(LED2, ledState);
      }
    }
    if (mod[0] != 4){
      if ((press[0] == 0) && (digitalReadFast(Button1) == HIGH) && (press[1] != 1) && (count > 2)){
        digitalWriteFast (LED1, HIGH);
        digitalWriteFast (LED2, LOW);
      
        previousMillis = currentMillis;
        count = 10;
    
        if (wait == 1){
          do{
            if (wait == 2){
              MidiOff();
            }  
          }
          while (wait != 0);
        }
        mod[0] = 1;
        press[0] = 1;
      }
     
      if ((press[1] == 0) && (digitalReadFast(Button2) == HIGH) && (press[0] != 1) && (count > 2)){
        digitalWriteFast (LED2, HIGH);
        digitalWriteFast (LED1, LOW);
      
        previousMillis = currentMillis;
        count = 10;
  
        if (wait == 1){
          do{
            if (wait == 2){
              MidiOff();
            }  
          }
          while (wait != 0);
        }
        
        mod[0] = 2;
        for (i = 0; i < 5; i += 1){
          arpnumber[i] = 0;
        }
        press[1] = 1;
      }
      
       if ((press[0] == 1) && (digitalReadFast(Button1) == LOW) && (count > 11)){ 
        press[0] = 0;
      }
      
      if ((press[1] == 1) && (digitalReadFast(Button2) == LOW) && (count > 11)){
        press[1] = 0;
      }
      if ((press[0] == 1) && (digitalReadFast(Button2) == HIGH) && (press[1] != 1)){
      
        digitalWriteFast (LED2, HIGH);
      
        if (wait == 1){
          do{
            if (wait == 2){
              MidiOff();
            }  
          }
          while (wait != 0);
        }
        if (mod[0] == 3){
          mod[0] = 5;                      // Voltage Addressed Polyseq
          count = 8;
        }
        else{
          mod[0] = 3;
          for (i = 0; i < 5; i += 1){
            arpnumber[i] = 0;
          }
        }
        press[1] = 1;
      }
      if ((press[1] == 1) && (digitalReadFast(Button1) == HIGH) && (press[0] != 1)){
        digitalWriteFast (LED1, HIGH);
      
        if (wait == 1){
          do{
            if (wait == 2){
              MidiOff();
            }  
          }
          while (wait != 0);
        }
        if (mod[0] ==3){
          mod[0] = 5;                      // Voltage Addressed Polyseq
          count = 8;
        }
        else{
          mod[0] = 3;
          for (i = 0; i < 5; i += 1){
            arpnumber[i] = 0;
          }
        }
        press[0] = 1;
      }
       
      if ((press[2] == 0) && (digitalReadFast(Button3) == HIGH) && (count > 2)){
        digitalWriteFast (LED3, HIGH);
        digitalWriteFast (LED4, LOW);
        digitalWriteFast (LED5, LOW);
      
        if(mod[1] == 3){                  //Clear transpose value if switching from Transpose mode
          ClearTrans();
        }
      
        mod[1] = 1;
        MIDI.turnThruOn(); 
        count = 10;
        press[2] = 1;
      }
      if ((press[3] == 0) && (digitalReadFast(Button4) == HIGH) && (count > 2)){
        digitalWriteFast (LED4, HIGH);
        digitalWriteFast (LED3, LOW);
        digitalWriteFast (LED5, LOW);
      
        if(mod[1] == 3){                  //Clear transpose value if switching from Transpose mode
          ClearTrans();
        }
      
        mod[1] = 2;
        MIDI.turnThruOff(); 
        count = 10;
        press[3] = 1;
      }
      if ((press[4] == 0) && (digitalReadFast(Button5) == HIGH) && (count > 2)){
        digitalWriteFast (LED5, HIGH);
        digitalWriteFast (LED4, LOW);
        digitalWriteFast (LED3, LOW);
        mod[1] = 3;
        MIDI.turnThruOff(); 
        count = 10;
        press[4] = 1;
      }
    }
    if ((press[2] == 1) && (digitalReadFast(Button3) == LOW) && (count > 11)){
      press[2] = 0;
    }
    if ((press[3] == 1) && (digitalReadFast(Button4) == LOW) && (count > 11)){
      press[3] = 0;
    }
    if ((press[4] > 0) && (digitalReadFast(Button5) == LOW) && (count > 11)){
      press[4] = 0;
    }
    if(wait == 2){
      MidiOff();
    }
    if (MIDI.read()) {
      MidiIn();
    }
   
    if ((press[4] == 2) && (count > 10)){
      press[4] = 0;
      //x = 2;
      if (mod[0] != 4){
        MidiOff();
        digitalWriteFast(LED1,HIGH);
        digitalWriteFast(LED2,HIGH);
        digitalWriteFast(LED3,HIGH);
        digitalWriteFast(LED4,HIGH);
        digitalWriteFast(LED5,HIGH);
        MIDI.turnThruOff();
        mod[0] = 4;
        EEPROM.write(1022,4);
//        locked = EEPROM.read(1021);
      }
      else {
        x = 2;
        mod[1] = 2;                       // thru Off after change from clock mode
        mod[0] = 1;
        digitalWriteFast(LED1, HIGH);
        digitalWriteFast(LED4, HIGH);
        EEPROM.write(1022,1);
      }
    }
   
    if (count > 16){
      x = 2;
    }
  }
  while (x != 2);
  
  if (mod[0] == 4){  
    digitalWriteFast(LED1, LOW);
    digitalWriteFast(LED2, LOW);
    digitalWriteFast(LED3, LOW);
    digitalWriteFast(LED4, LOW);
    digitalWriteFast(LED5, LOW);
  }
}
  

ISR(PCINT1_vect){
  if (mod[0] != 4){ 
    if (wait == 0){
      if (digitalReadFast (gateIn) == HIGH){
        delayMicroseconds(del);
        velocity = analogRead (gateIn);  
        if (MIDI.read()) {
          MidiIn();
        }     
  
        velocity = velocity - 495;
        velocity = velocity >> 2;
      
        if (velocity > 127){
          velocity = 127;
        }
        if (velocity <= 0){
          velocity = 1;
        }
        cvValue = analogRead (cvIn);
  
        if (cvValue <= 3){
          for (i = 0; i < 5; i += 1){
            Bank[i] = 0;
          }
          if (mod[0] > 1){
            for (i = 0; i < 5; i += 1){
              if (arpnumber[i] < length[i]){
                arpnumber[i] += 1;
              }
              else {
                arpnumber[i] = 0;
              }
            }
          }
        }
        else if (cvValue > 3 && cvValue <= 207){ 
          MidiOn(1);
        }
 
        else if (cvValue > 207 && cvValue <= 411){
          MidiOn(2);
        }
      
        else if (cvValue > 411 && cvValue <= 615){
          MidiOn(3);
        }
    
        else if (cvValue > 615 && cvValue <= 819){
          MidiOn(4);
        }
    
        else if (cvValue > 819 && cvValue <= 1023){
          MidiOn(5);
        }
      }
    }
    else if ((wait == 1) && (digitalReadFast(gateIn) == LOW)){ 
      wait = 2;
    }
  }

  else {
    if((PINC & (1 << PINC0)) == 1 ){
      clock_pulse();
    }   
    counter = 0; 
  }
}

void clock_pulse(){
    
  if (started == 0){
    syncSignal(0xFA);
    started = 1;
  }
    
  if (shift > 0){
    for (i=0; i<shift; i+=1){
      syncSignal(0xF8);
    }
    shift = 1;
  }
  else shift += 1;
} 

void loop() {

  if (mod[0] != 4){  
    if (Release > 1){  

      counter = 0;
      Switch[0] = 1;
      Switch[1] = 1;
      Switch[2] = 1;
      Switch[3] = 1;
      Switch[4] = 1;
      Release = 0;
    }

    unsigned long currentMillis = millis();
    if((currentMillis - prevMillis > 250)) {
      prevMillis = currentMillis;   
      if (counter < 3){                          // added to stop led held on longer than should
        counter += 1;
      }
    }
  
    if (MIDI.read()) {
      MidiIn();
    }

    if(Bank[0] == 1){
      digitalWriteFast(LED1, HIGH);
    }
    if((Bank[0] == 0) && (counter > 2)){ 
      digitalWriteFast(LED1, LOW);
    }
    if(Bank[1] == 1){
      digitalWriteFast(LED2, HIGH);
    }
    if((Bank[1] == 0) && (counter > 2)){
      digitalWriteFast(LED2, LOW);
    }
    if(Bank[2] == 1){
      digitalWriteFast(LED3, HIGH);
    }
    if((Bank[2] == 0) && (counter > 2)){
      digitalWriteFast(LED3, LOW);
    }
    if(Bank[3] == 1){
      digitalWriteFast(LED4, HIGH);
    }
    if((Bank[3] == 0) && (counter > 2)){
      digitalWriteFast(LED4, LOW);
    }
    if(Bank[4] == 1){
      digitalWriteFast(LED5, HIGH);
    }
    if((Bank[4] == 0) && (counter > 2)){
      digitalWriteFast(LED5, LOW);
    }
 
    if (Switch[0] != 0){
      if(digitalReadFast(Button1)==LOW){
        Switch[0] = 0;
      }
    }
 
    if (Switch[1] != 0){
      if(digitalReadFast(Button2)==LOW){
        Switch[1] = 0;
      }
    }
 
    if (Switch[2] != 0){
      if(digitalReadFast(Button3)==LOW){
        Switch[2] = 0;
      }
    }
 
    if (Switch[3] != 0){
      if(digitalReadFast(Button4)==LOW){
        Switch[3] = 0;
      }
    }
 
    if (Switch[4] != 0){
      if(digitalReadFast(Button5)==LOW){
        Switch[4] = 0;
      }
    } 
 
    if(wait == 2){
      MidiOff();
    }
 
    if((digitalReadFast(Button1)== HIGH) && (Switch[0] == 0) && (counter > 1)){
      Switch[0] = 1;
      readMIDI(1);
    }
    if((digitalReadFast(Button2)== HIGH) && (Switch[1] == 0) && (counter > 1)){
      Switch[1] = 1;
      readMIDI(2);
    }
    if((digitalReadFast(Button3)== HIGH) && (Switch[2] == 0) && (counter > 1)){
      Switch[2] = 1; 
      readMIDI(3);
    }
    if((digitalReadFast(Button4)== HIGH) && (Switch[3] == 0) && (counter > 1)){
      Switch[3] = 1;
      readMIDI(4);
    }
    if((digitalReadFast(Button5)== HIGH) && (Switch[4] == 0) && (counter > 1)){
      Switch[4] = 1;
      readMIDI(5);
    }
  }

  else{
    if (started != 1){
      Release = 0;
      ledState = HIGH;
      do{
        if (digitalReadFast(Button5) == HIGH){
          digitalWriteFast(LED5, HIGH);  
          unsigned long currentMillis = millis();
          if(currentMillis - prevMillis > interval) {
            prevMillis = currentMillis;
            hold += 1;
          }
          if (hold > 5){
            Release = 5;
            mode();
          }
        }
        else {
        digitalWriteFast(LED5, LOW);
        hold = 0;
      }
      
      if (Switch[2] > 0){
        unsigned long currentMillis = millis();
        if(currentMillis - prevMillis > interval) {
          prevMillis = currentMillis;
        
          if (locked == 0){
            digitalWriteFast(LED3, ledState);
          }
          if (Switch[2] == 1){
            digitalWriteFast(LED3, LOW);
          }
          if(ledState == HIGH){
            ledState = LOW;
          }
          else ledState = HIGH;
          Switch[2] -= 1;
        }
      }
    
      if (digitalReadFast(Button3) == HIGH){
        debounce (Button3, LED3);
        Switch[2] = 5;
        if (locked == 1){
          locked = 0;
          ledState = LOW;
          digitalWriteFast(LED3, HIGH);
        }
        else {
          locked = 1; 
          digitalWriteFast(LED3, HIGH);
        }
      }
 
      if (digitalReadFast(Button2) == HIGH){
        digitalWriteFast(LED2, HIGH);  
        do{
          unsigned long currentMillis = millis();
          if(currentMillis - prevMillis > interval) {
            prevMillis = currentMillis;
            hold += 1;
          }
          if (hold > 5){
          //Save locked paramater to EEPROM 1021
            digitalWriteFast(LED1, HIGH);
            digitalWriteFast(LED2, HIGH);
            digitalWriteFast(LED3, HIGH);
            digitalWriteFast(LED4, HIGH);
            digitalWriteFast(LED5, HIGH);
            if(EEPROM.read(1021) != locked){
              EEPROM.write(1021, locked);
            }
            hold = 0; 
          }
        }
        while (digitalReadFast(Button2) == HIGH);
        hold = 0;
        digitalWriteFast(LED1, LOW);
        digitalWriteFast(LED2, LOW);
        digitalWriteFast(LED3, LOW);
        digitalWriteFast(LED4, LOW);
        digitalWriteFast(LED5, LOW);
      }  
    }
    while ((started != 1) && (Release != 5));
  }

  unsigned long currentMillis = millis();
  if(currentMillis - prevMillis > interval) {
    // save the last time you blinked the LED
    if (started == 1){
      prevMillis = currentMillis;
    }
    if (hold > 0){
      hold += 1;
    }  
 
    if ((started == 1) && (locked == 0)){
      if (ledState == LOW)
        ledState = HIGH;
      else
        ledState = LOW;

      digitalWriteFast(LED3, ledState);
    }
  }
    
  counter += 1;
 
  if (counter > 25000){
    syncSignal(0xFC);
    started = 0;
    //locked = 0;
    nudge = 0;
    digitalWriteFast(nudgeLED, LOW);
    nudgeLED = 0;
    digitalWriteFast(LED3, LOW);
  }
  if ((started == 1) && (locked == 1)){
    digitalWriteFast(LED3,HIGH);
  }  
  
  if((digitalReadFast(Button3)==HIGH) && (Switch[2] == 0)){
    Switch[2] = 1;
    //nudge = 0;
    if (nudgeLED != 0){
      digitalWriteFast(nudgeLED, LOW);
      nudgeLED = 0;
    }
  
    if(started==0){    

      syncSignal(0xFA);
      
      debounce(Button3, LED3);
 
      started=1;
      counter = 0;
    }
     
    else if(started == 1 && nudge == 0){    // nudge must be 0 to switch locked state
      if (locked == 1){
        locked = 0;
      }
      else locked = 1;
      debounce(Button3, LED3);
    }
    nudge = 0; 
  }
  if((digitalReadFast(Button1) == HIGH) && (Switch[0] == 0)){
    Switch[0] = 1;
    if (locked == 1){
      nudge = -12;
      shift-=12;
      if (nudgeLED != LED1){
        digitalWriteFast(nudgeLED, LOW);
        nudgeLED = LED1;
      }
    }
    else shift-=2;
    debounce(Button1, LED1);
  }
  if((digitalReadFast(Button2) == HIGH) && (Switch[1] == 0)){
    Switch[1] = 1;
    if (locked == 1){
      nudge = -6;
      shift-=6;
      if (nudgeLED != LED2){
        digitalWriteFast(nudgeLED, LOW);
        nudgeLED = LED2;
      }
    }
    else shift-=1;
    debounce(Button2, LED2);  
  }
  if((digitalReadFast(Button4) == HIGH) && (Switch[3] == 0)){
    Switch[3] = 1;
    if (locked ==1){
      nudge = 6;
      shift+=6;
      if (nudgeLED != LED4){
        digitalWriteFast(nudgeLED, LOW);
        nudgeLED = LED4;
      }
    }
    else shift +=1;
    debounce(Button4, LED4);
  }
  if((digitalReadFast(Button5) == HIGH) && (Switch[4] == 0)){
    Switch[4] = 1;
    if (hold == 0){
      hold = 1;
    }
    if (locked == 1){
      nudge = 12;
      shift+=12;
      if (nudgeLED != LED5){
        digitalWriteFast(nudgeLED, LOW);
        nudgeLED = LED5;
      }
    }
    else shift +=2;
    debounce(Button5, LED5);
  }
  if(digitalReadFast(Button5)==LOW){
    hold = 0;
  }
  if(hold > 5){
    Release = 5;
    hold = 0;           
    mode();
    digitalWriteFast(LED1,LOW);
    digitalWriteFast(LED2,LOW);
    digitalWriteFast(LED4,LOW);
    digitalWriteFast(LED5,LOW);
  }
  
  
  if (nudged == 0){
    if(digitalReadFast(cvIn) == HIGH){
      digitalWriteFast(nudgeLED, HIGH);
      nudged = 1;
      shift += nudge;
    }
  }
  if (nudged == 1){
    if(digitalReadFast(cvIn) == LOW){
      digitalWriteFast(nudgeLED, LOW);
      nudged = 0;
    }
  }
  
  if ((Switch[0] == 1) && (digitalReadFast(Button1) == LOW)){
    Switch[0] = 0;
  }
  if ((Switch[1] == 1) && (digitalReadFast(Button2) == LOW)){
    Switch[1] = 0;
  }
  if ((Switch[2] == 1) && (digitalReadFast(Button3) == LOW)){
    Switch[2] = 0;
  }
  if ((Switch[3] == 1) && (digitalReadFast(Button4) == LOW)){
    Switch[3] = 0;
  }
  if ((Switch[4] == 1) && (digitalReadFast(Button5) == LOW)){
    Switch[4] = 0;
  }  
}
}



void MidiIn_Clock(){
  byte type = MIDI.getType();
  if (type == midi::Start){
    start_time = millis();
    syncSignal(0xFA);
    digitalWriteFast(LED3, HIGH);
    started = 10;
  }
}

void MidiOn(byte z){
  
  if (mod[0] >= 3){                        //polyseq (3) or vapolyseq (5)
    if ((cvValue > 900) && (mod[0] ==3)){
      for (j = 0; j < 5; j += 1){
        arpnumber[j] = 0;
      }
    }
    if (mod[0] == 5){                      //vapolyseq
      if (cvValue <= 50){
        for (i = 0; i < 5; i += 1){
          arpnumber[i] = 0;
        }
      }
      else if (cvValue >= 990){
        for (i = 0; i < 5; i += 1){
          arpnumber[i] = 15;
        }
      }
      else { 
        cvValue = (cvValue - 50) >> 6;
        for (i = 0; i < 5; i += 1){
          arpnumber[i] = cvValue;
        }
      }
    }
    if (trans[0] != 0){
      if (mod[0] == 3){
        if (MidiNotes1[0] < 128){
          transpose = (MidiNotes1[0] - trans[0]);
        }
        else if (MidiNotes2[0] < 128){
          transpose = (MidiNotes2[0] - trans[0]);
        }
        else if (MidiNotes3[0] < 128){
          transpose = (MidiNotes3[0] - trans[0]);
        }
        else if (MidiNotes4[0] < 128){
          transpose = (MidiNotes4[0] - trans[0]);
        }
        else if (MidiNotes5[0] < 128){
          transpose = (MidiNotes5[0] - trans[0]);
        }
      }
      else {
        byte notestack[] = {MidiNotes1[cvValue], MidiNotes2[cvValue], MidiNotes3[cvValue], MidiNotes4[cvValue], MidiNotes5[cvValue]};
       
        for (i = 0; i < 5; i += 1){
          if (notestack[i] < 128){
            transpose = notestack[i] - trans[0];
            i = 5;
          }
        }
      }
    }
    else {
      transpose = 0;
    }
    j = arpnumber[0];
    if(MidiNotes1[j] < 128){
      if(channels1[j] == transchan[0]){
        note[0] = (MidiNotes1[j] - transpose); 
      }
      else {
        note[0] = MidiNotes1[j];
      }
    }
    else {
      note[0] = MidiNotes1[j];
    }
    k = arpnumber[1];
    if(MidiNotes2[k] < 128){
      if(channels2[k] == transchan[0]){
        note[1] = (MidiNotes2[k] - transpose); 
      }
      else {
        note[1] = MidiNotes2[k];
      }
    }
    else {
      note[1] = MidiNotes2[k];
    }
    l = arpnumber[2];
    if(MidiNotes3[l] < 128){
      if(channels3[l] == transchan[0]){
        note[2] = (MidiNotes3[l] - transpose); 
      }
      else {
        note[2] = MidiNotes3[l]; 
      }
    }
    else {
      note[2] = MidiNotes3[l]; 
    }
    m = arpnumber[3];
    if(MidiNotes4[m] < 128){
      if(channels4[m] == transchan[0]){
        note[3] = (MidiNotes4[m] - transpose); 
      }
      else {
        note[3] = MidiNotes4[m];
      }
    }
    else {
      note[3] = MidiNotes4[m];
    }
    n = arpnumber[4];
    if(MidiNotes5[n] < 128){
      if(channels5[n] == transchan[0]){
        note[4] = (MidiNotes5[n] - transpose); 
      }
      else {
        note[4] = MidiNotes5[n];
      }
    }
    else {
      note[4] = MidiNotes5[n];
    }
    if((Mutes[0] != 0) && (note[0] < 128)){
      MIDI.sendNoteOn(note[0],velocity,channels1[j]);
      Bank[0] = 1;
    }
    if((Mutes[1] != 0) && (note[1] < 128)){
      MIDI.sendNoteOn(note[1],velocity,channels2[k]);
      Bank[1] = 1;
    }
    if((Mutes[2] != 0) && (note[2] < 128)){
      MIDI.sendNoteOn(note[2],velocity,channels3[l]);
      Bank[2] = 1;
    }
    if((Mutes[3] != 0) && (note[3] < 128)){
      MIDI.sendNoteOn(note[3],velocity,channels4[m]);
      Bank[3] = 1;
    }
    if((Mutes[4] != 0) && (note[4] < 128)){
      MIDI.sendNoteOn(note[4],velocity,channels5[n]);
      Bank[4] = 1;
    }
    wait = 1;
  }
  
  else if (z == 1){
    if (Mutes[0] != 0){
      Bank[0] = 1;
      if (pchange1[0] != 128){
        if ((pchange1[0] != program[0]) || (pchange1[1] != program[1])){ 
          MIDI.sendProgramChange(pchange1[0],pchange1[1]);
          program[0] = pchange1[0];
          program[1] = pchange1[1];
        }
      }
      if (trans[0] != 0){
        transpose = (MidiNotes1[0] - trans[0]);
      }
      else {
        transpose = 0;
      }
      if (mod[0] == 1){
        for (j = 0; j < 16; j += 1) {
          if (MidiNotes1[j] < 128){
            if(channels1[j] == transchan[0]){
              note[j] = (MidiNotes1[j] - transpose);
            }
            else {
              note[j] = MidiNotes1[j];
            }
          }
          else {
            note[j] = MidiNotes1[j];
          }
        }
        for (j = 0; j < 16; j += 1){
          if (note[j] < 128){  
            MIDI.sendNoteOn(note[j],velocity,channels1[j]);
          }
          if (note[j] == 128){
            j = 16;
          }
        }
        wait = 1;    
      }
      
      else {
        j = arpnumber[0];
        if(MidiNotes1[j] < 128){
          if(channels1[j] == transchan[0]){
            note[j] = (MidiNotes1[j] - transpose); 
          }
          else {
            note[j] = MidiNotes1[j];
          } 
          MIDI.sendNoteOn(note[j],velocity,channels1[j]);
        }
        else {
          note[j] = MidiNotes1[j];
        } 
        wait = 1;
     
      }
    }
  }
  
  else if (z == 2){
    if (Mutes[1] != 0){
      Bank[1] = 1;
      if (pchange2[0] != 128){
        if ((pchange2[0] != program[0]) || (pchange2[1] != program[1])){ 
          MIDI.sendProgramChange(pchange2[0],pchange2[1]);
          program[0] = pchange2[0];
          program[1] = pchange2[1];
        }
      }
      if (trans[0] != 0){
        transpose = (MidiNotes2[0] - trans[0]);
      }
      else {
        transpose = 0;
      }
      if (mod[0] == 1){
        for (j = 0; j < 16; j += 1) {
          if (MidiNotes2[j] < 128){
            if(channels2[j] == transchan[0]){
              note[j] = (MidiNotes2[j] - transpose);
            }
            else {
              note[j] = MidiNotes2[j];
            }
          }
          else {
            note[j] = MidiNotes2[j];
          }
        }
        for (j = 0; j < 16; j += 1) {
          if (note[j] < 128){
            MIDI.sendNoteOn(note[j],velocity,channels2[j]);
          }
          if (note[j] == 128){
            j = 16;
          }
        }
      
        wait = 1;
      
      }
      else {
        j = arpnumber[1];
        if(MidiNotes2[j] < 128){
          if(channels2[j] == transchan[0]){
            note[j] = (MidiNotes2[j] - transpose);
          }
          else {
            note[j] = MidiNotes2[j];
          }  
          MIDI.sendNoteOn(note[j],velocity,channels2[j]);
        }
        else {
          note[j] = MidiNotes2[j];
        } 
        wait = 1;
     
      } 
    }  
  }
  
  else if (z == 3){
    if (Mutes[2] != 0){
        Bank[2] = 1;
        if (pchange3[0] != 128){
          if ((pchange3[0] != program[0]) || (pchange3[1] != program[1])){ 
            MIDI.sendProgramChange(pchange3[0],pchange3[1]);
            program[0] = pchange3[0];
            program[1] = pchange3[1];
          }
        }
        if (trans[0] != 0){
          transpose = (MidiNotes3[0] - trans[0]);
        }
        else {
          transpose = 0;
        }
        if (mod[0] == 1){
          for (j = 0; j < 16; j += 1) {
            if (MidiNotes3[j] < 128){
              if(channels3[j] == transchan[0]){
                note[j] = (MidiNotes3[j] - transpose);
              }
              else {
                note[j] = MidiNotes3[j];
              }
            }
            else {
              note[j] = MidiNotes3[j];
            }
          }
          for (j = 0; j < 16; j += 1) {
            if (note[j] < 128){
              MIDI.sendNoteOn(note[j],velocity,channels3[j]);
            }
            if (note[j] == 128){
              j = 16;
            }
          }
          wait = 1; 
      
        }
        else {
          j = arpnumber[2];
          if(MidiNotes3[j] < 128){
            if(channels3[j] == transchan[0]){
              note[j] = (MidiNotes3[j] - transpose);
            }
            else {
              note[j] = MidiNotes3[j];
            }  
            MIDI.sendNoteOn(note[j],velocity,channels3[j]);
          }
          else {
            note[j] = MidiNotes3[j];
          }
          wait = 1;
     
        }
      }
    }
  
    else if (z == 4){
      if (Mutes[3] != 0){
        Bank[3] = 1;
        if (pchange4[0] != 128){
          if ((pchange4[0] != program[0]) || (pchange4[1] != program[1])){ 
            MIDI.sendProgramChange(pchange4[0],pchange4[1]);
            program[0] = pchange4[0];
            program[1] = pchange4[1];
          }
        }
        if (trans[0] != 0){
          transpose = (MidiNotes4[0] - trans[0]);
        }
        else {
          transpose = 0;
        }
        if (mod[0] == 1){
          for (j = 0; j < 16; j += 1) {
            if (MidiNotes4[j] < 128){
              if(channels4[j] == transchan[0]){
                note[j] = (MidiNotes4[j] - transpose);
              }
              else {
                note[j] = MidiNotes4[j];
              }
            }
            else {
              note[j] = MidiNotes4[j];
            }
          }
          for (j = 0; j < 16; j += 1) {
            if (note[j] < 128){
              MIDI.sendNoteOn(note[j],velocity,channels4[j]);
            }
            if (note[j] == 128){
              j = 16;
            }
          }
          wait = 1; 
        }
        else {
          j = arpnumber[3];
          if(MidiNotes4[j] < 128){
            if(channels4[j] == transchan[0]){
              note[j] = (MidiNotes4[j] - transpose);
            }
            else {
              note[j] = MidiNotes4[j];
            }  
            MIDI.sendNoteOn(note[j],velocity,channels4[j]);
          }
          else {
            note[j] = MidiNotes4[j];
          } 
          wait = 1; 
        }
      }
    }  
  
    else if (z == 5){
      if (Mutes[4] != 0){
        Bank[4] = 1;
        if (pchange5[0] != 128){
          if ((pchange5[0] != program[0]) || (pchange5[1] != program[1])){ 
            MIDI.sendProgramChange(pchange5[0],pchange5[1]);
            program[0] = pchange5[0];
            program[1] = pchange5[1];
          }
        }
        if (trans[0] != 0){
          transpose = (MidiNotes5[0] - trans[0]);
        }
        else {
          transpose = 0;
        }
        if (mod[0] == 1){
          for (j = 0; j < 16; j += 1) {
            if (MidiNotes5[j] < 128){
              if(channels5[j] == transchan[0]){
                note[j] = (MidiNotes5[j] - transpose);
              }
              else {
                note[j] = MidiNotes5[j];
              }
            }
            else {
              note[j] = MidiNotes5[j];
            }
          }
          for (j = 0; j < 16; j += 1) {
            if (note[j] < 128){
              MIDI.sendNoteOn(note[j],velocity,channels5[j]);
            }
            if (note[j] == 128){
              j = 16;
            }
          }
          wait = 1; 
        }
        else {
          j = arpnumber[4];
          if(MidiNotes5[j] < 128){
            if(channels5[j] == transchan[0]){
              note[j] = (MidiNotes5[j] - transpose);
            }
            else {
              note[j] = MidiNotes5[j];
            }  
            MIDI.sendNoteOn(note[j],velocity,channels5[j]);
          }
          else {
            note[j] = MidiNotes5[j];
          }
          wait = 1;
     
        }
      }
    }  
  }


void MidiOff (){
  if ((mod[0] == 2) || ((mod[0] == 3) && (cvValue < 512))){
    for(i = 0; i < 5; i += 1){
      if (arpnumber[i] < length[i]){
        arpnumber[i] += 1;
      }
      else {
        arpnumber[i] = 0;
      }
    } 
  }
    
  if (mod[0] >= 3){
    if(Bank[0] == 1){
      if (note[0] < 128){
        MIDI.sendNoteOff(note[0],velocity,channels1[j]);
      }
      Bank[0] = 0;
    }
    if(Bank[1] == 1){
      if (note[1] < 128){
        MIDI.sendNoteOff(note[1],velocity,channels2[k]);
      }
      Bank[1] = 0;
    }
    if(Bank[2] == 1){
      if (note[2] < 128){
        MIDI.sendNoteOff(note[2],velocity,channels3[l]);
      }
      Bank[2] = 0;
    }
    if(Bank[3] == 1){
      if (note[3] < 128){
        MIDI.sendNoteOff(note[3],velocity,channels4[m]);
      }
      Bank[3] = 0;
    }
    if(Bank[4] == 1){
      if (note[4] < 128){
        MIDI.sendNoteOff(note[4],velocity,channels5[n]);
      }
      Bank[4] = 0;
    }
    PCICR |= (0<<PCIE1);
    Serial.flush();
    wait = 0;
    PCICR |= (1<<PCIE1); 
  }
  else if (Bank[0] == 1){
    Bank[0] = 0;
    if (mod[0] == 1){
      for (i=0; i < 16; i += 1) {
        if (note[i] < 128){
          MIDI.sendNoteOff(note[i],127,channels1[i]);
        }
        if (note[i] == 128){
          i = 16;
        }
      }
    }
    else{
      if (arpnumber[0] != 0){
        i = (arpnumber[0] - 1);
      }
      else{
        i = length[0];
      }
      if (note[i] < 128){
        MIDI.sendNoteOff(note[i],127,channels1[i]);
      }
    }
    PCICR |= (0<<PCIE1);
    Serial.flush();
    wait = 0;
    PCICR |= (1<<PCIE1); 
  }
  else if (Bank[1] == 1){
    Bank[1] = 0;
    if (mod[0] == 1){
      for (i=0; i < 16; i += 1) {
        if (note[i] < 128){
          MIDI.sendNoteOff(note[i],127,channels2[i]);
        }
        if (note[i] == 128){
          i = 16;
        }
      }
    }
    else{
      if (arpnumber[1] != 0){
        i = (arpnumber[1] - 1);
      }
      else{
        i = length[1];
      }
      if (note[i] < 128){
        MIDI.sendNoteOff(note[i],127,channels2[i]);
      }
    }
    PCICR |= (0<<PCIE1);
    Serial.flush();
    wait = 0;
    PCICR |= (1<<PCIE1);
  }
  else if (Bank[2] == 1){
    Bank[2] = 0;
    if (mod[0] == 1){
      for (i=0; i < 16; i += 1) {
        if (note[i] < 128){
          MIDI.sendNoteOff(note[i],127,channels3[i]);
        }
        if (note[i] == 128){
          i = 16;
        }
      }
    }
    else{
      if (arpnumber[2] != 0){
        i = (arpnumber[2] - 1);
      }
      else{
        i = length[2];
      }
      if (note[i] < 128){
        MIDI.sendNoteOff(note[i],127,channels3[i]);
      }
    }
    PCICR |= (0<<PCIE1);
    Serial.flush();
    wait = 0;
    PCICR |= (1<<PCIE1);
  }
  else if (Bank[3] == 1){
    Bank[3] = 0;
    if (mod[0] == 1){
      for (i=0; i < 16; i += 1) {
        if (note[i] < 128){
          MIDI.sendNoteOff(note[i],127,channels4[i]);
        }
        if (note[i] == 128){
          i = 16;
        }
      }
    }
    else{
      if (arpnumber[3] != 0){
        i = (arpnumber[3] - 1);
      }
      else{
        i = length[3];
      }
      if (note[i] < 128){
        MIDI.sendNoteOff(note[i],127,channels4[i]);
      }
    }
    PCICR |= (0<<PCIE1);
    Serial.flush();
    wait = 0;
    PCICR |= (1<<PCIE1);
  }
  else if (Bank[4] == 1){
    Bank[4] = 0;
    if (mod[0] == 1){
      for (i=0; i < 16; i += 1) {
        if (note[i] < 128){
          MIDI.sendNoteOff(note[i],127,channels5[i]);
        }
        if (note[i] == 128){
          i = 16;
        }
      }
    }
    else{
      if (arpnumber[4] != 0){
        i = (arpnumber[4] - 1);
      }
      else{
        i = length[4];
      }
      if (note[i] < 128){
        MIDI.sendNoteOff(note[i],127,channels5[i]);
      }
    }
    PCICR |= (0<<PCIE1);
    Serial.flush();
    wait = 0;
    PCICR |= (1<<PCIE1); 
  }
   
  PCICR |= (0<<PCIE1);
  Serial.flush();
  wait = 0;
  PCICR |= (1<<PCIE1);  
}

void MidiIn(){
  byte p;
  byte q;
  byte channel;
  
  byte type = MIDI.getType();
    switch (type) {
      case midi::ControlChange:
      if (MIDI.getData1() == 123){ //  AllNotesOff
        PCICR |= (0<<PCIE1);
        Serial.flush();
        PCICR |= (1<<PCIE1);   
      }
      break;
       
      case midi::NoteOn:
      if (mod[1] == 3){
        newnote = MIDI.getData1();
        channel = MIDI.getChannel();
        if (MIDI.getData2() != 0){
          for (i = 0; i < 5; i += 1){
            if (newnote > trans[i]){
              p = i;
              i = 5;
            }
          }
          for (i = 4; i > p; i -= 1){
            trans[i] = trans[i - 1];
            transchan[i] = transchan[i - 1];
          }
          trans[p] = newnote;
          transchan[p] = channel;
        }
    
        else {
          for (i = 0; i < 5; i += 1){
            if (newnote == trans[i]){
              p = i;
              i = 5;
            }
          }
          for (i = p; i < 5; i += 1){
            q = (i + 1);
            trans[i] = trans[q];
            transchan[i] = transchan[q];
          }
        }
      }
      break;
      
      case midi::NoteOff:
      if (mod[1] == 3){
        newnote = MIDI.getData1();
        channel = MIDI.getChannel();
        for (i = 0; i < 5; i += 1){
          if (newnote == trans[i]){
            p = i;
            i = 5;
          }
        }
        for (i = p; i < 5; i += 1){
          q = (i + 1);
          trans[i] = trans[q];
          transchan[i] = transchan[q];
        }
      }
      break;
    }
  }

void ClearTrans(){
  
  for (i=0; i<5; i+=1){
    trans[i] = 0;
  }  
}

void syncSignal(char cmd){
  Serial.write(cmd);
}

void debounce(int Button, int LED){
  digitalWriteFast(LED, HIGH);
  delay(300);
  digitalWriteFast(LED,LOW);
}

void readpolymidi(byte l, byte m){
  long previousMillis = 0;
  int count = 0;
  byte counter = l-1;
  byte press[] = {1,1,1,1,1};
  byte keys = 0;
  byte MidiNotes[] = {128, 128, 128, 128, 128};
  byte channels[] = {0, 0, 0, 0, 0};
  byte LED[] = {LED1, LED2, LED3, LED4, LED5};

  for (i = 0; i < counter; i += 1){
    digitalWriteFast(LED[i], LOW);
  }
  for (i = m; i < 5; i += 1){
    digitalWriteFast(LED[i], LOW);
  }
  for (i = counter; i < m; i += 1){
    digitalWriteFast(LED[i], HIGH);
  }
  q = 0;
  do{
    if (press[0] != 0){
    if (digitalReadFast(Button1) == LOW){
      press[0] = 0;
    }
    }
    if (press[1] != 0){
    if (digitalReadFast(Button2) == LOW){
      press[1] = 0;
    }
    }
    if (press[2] != 0){
    if (digitalReadFast(Button3) == LOW){
      press[2] = 0;
    }
    }
    if (press[3] != 0){
    if (digitalReadFast(Button4) == LOW){
      press[3] = 0;
    }
    }
    if (press[4] != 0){
    if (digitalReadFast(Button5) == LOW){
      press[4] = 0;
    }
    }
    if(wait == 2){
      MidiOff();
    }
    if (press[0] == 0 && press[1] == 0 && press[2] == 0 && press[3] == 0 && press[4] == 0){
      count = 1;
    }
  }
  while (count != 1);
  count = 0;  
  do{
  unsigned long currentMillis = millis();
    if((currentMillis - previousMillis > interval) && Release < 3) {

      previousMillis = currentMillis;  
      count += 1;

      if (ledState == LOW)
        ledState = HIGH;
      else
        ledState = LOW;

      for (i = (l-1); i < m; i += 1){
        digitalWriteFast(LED[i], ledState);
      }
    }
    if (MIDI.read()) {
      MidiIn();
      byte type = MIDI.getType();
      switch (type) {
        case midi::NoteOn:
        noteByte = MIDI.getData1();
        velocityByte = MIDI.getData2();
        channel = MIDI.getChannel();
        
        if (velocityByte != 0){
          if (counter < m){
            MidiNotes[counter] = noteByte;
            channels[counter] = channel;
            counter += 1;
          }
          keys += 1;
        }
        
        break;
        
        case midi::NoteOff:
        keys -= 1;
        if (keys == 0){
          for (i = counter; i < m; i += 1){
            MidiNotes[i] = 129;
          }
          if(MidiNotes[0] != 128){
            MidiNotes1[q] = MidiNotes[0];
          }
          if(MidiNotes[1] != 128){
            MidiNotes2[q] = MidiNotes[1];
          }
          if(MidiNotes[2] != 128){
            MidiNotes3[q] = MidiNotes[2];
          }
          if(MidiNotes[3] != 128){
            MidiNotes4[q] = MidiNotes[3];
          }
          if(MidiNotes[4] != 128){
            MidiNotes5[q] = MidiNotes[4];
          }
          if(channels[0] != 0){
            channels1[q] = channels[0];
          }
          if(channels[1] != 0){
            channels2[q] = channels[1];
          }
          if(channels[2] != 0){
            channels3[q] = channels[2];
          }
          if(channels[3] != 0){
            channels4[q] = channels[3];
          }
          if(channels[4] != 0){
            channels5[q] = channels[4];
          }
          for ( i = (l-1); i < m; i += 1){
            length[i] = q;
          }
          for (i = 0; i < 5; i += 1){
            MidiNotes[i] = 128;
            channels[i] = 0;
          }
          counter = l-1;
          q += 1;
        }
        break;
      
      }
    } 
    noteByte = 0;
    velocityByte = 1;
    channel = 0;
      
    if(wait == 2){
      MidiOff();
    }
    
     if (digitalReadFast(Button1) == HIGH){
      if (press[0] == 0) {
        digitalWriteFast(LED1, HIGH);
        count = 0;
        press[0] = 1;
      }
      else {
        if (count > 4){
          exit();
        }
      }
    }
    else {
      if (press[0] == 1 && count > 1){
        press[0] = 0;
        digitalWriteFast(LED1, LOW);
        skip(counter, l, m);
      }
    }
    if (digitalReadFast(Button2) == HIGH){
      if (press[1] == 0) {
        digitalWriteFast(LED2, HIGH);
        count = 0;
        press[1] = 1;
      }
      else {
        if (count > 4){
          exit();
        }
      }
    }
    else {
      if (press[1] == 1 && count > 1){
        press[1] = 0;
        digitalWriteFast(LED2, LOW);
        skip(counter, l, m);
      }
    }
    if (digitalReadFast(Button3) == HIGH){
      if (press[2] == 0) {
        digitalWriteFast(LED3, HIGH);
        count = 0;
        press[2] = 1;
      }
      else {
        if (count > 4){
          exit();
        }
      }
    }
    else {
      if (press[2] == 1 && count > 1){
        press[2] = 0;
        digitalWriteFast(LED3, LOW);
        skip(counter, l, m);
      }
    }
    if (digitalReadFast(Button4) == HIGH){
      if (press[3] == 0) {
        digitalWriteFast(LED4, HIGH);
        count = 0;
        press[3] = 1;
      }
      else {
        if (count > 4){
          exit();
        }
      }
    }
    else {
      if (press[3] == 1 && count > 1){
        press[3] = 0;
        digitalWriteFast(LED4, LOW);
        skip(counter, l, m);
      }
    }
    if (digitalReadFast(Button5) == HIGH){
      if (press[4] == 0) {
        digitalWriteFast(LED5, HIGH);
        count = 0;
        press[4] = 1;
      }
      else {
        if (count > 4){
          exit();
        }
      }
    }
    else {
      if (press[4] == 1 && count > 1){
        press[4] = 0;
        digitalWriteFast(LED5, LOW);
        skip(counter, l, m);
      }
    }   
  }
  while (q < 16);
}
 
void skip(byte counter, byte l, byte m){
  for (i = counter; i < m; i += 1){
    MidiNotes[i] = 129;
  }
  for (i = 0; i < counter; i += 1){        // No skip for banks below bottom button press
    MidiNotes[i] = 128;
  }
  for (i = m; i < 5; i += 1){              // No skip for banks above top button press
    MidiNotes[i] = 128;
  }
  if(MidiNotes[0] != 128){
    MidiNotes1[q] = MidiNotes[0];
  }
  if(MidiNotes[1] != 128){
    MidiNotes2[q] = MidiNotes[1];
  }
  if(MidiNotes[2] != 128){
    MidiNotes3[q] = MidiNotes[2];
  }
  if(MidiNotes[3] != 128){
    MidiNotes4[q] = MidiNotes[3];
  }
  if(MidiNotes[4] != 128){
    MidiNotes5[q] = MidiNotes[4];
  }
  for (i = counter; i < m; i += 1){
    length[i] = q;
  }
  q += 1;
}

void exit(){

  q = 16;
  
} 
