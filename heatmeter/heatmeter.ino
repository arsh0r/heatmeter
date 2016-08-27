// heat meter
 
// Copyright (c) 2015 arsh0r
// See the file LICENSE for copying permission.

#include <OneWire.h>

//#define DEBUG_OW
//DEBUG_OW Output:
//S01_SEARCH: 15 ms (only once)
//S02_START_CONVERSION: 7 ms
//S03_WAIT: 743 ms
//S04_CMD_SCRATCHPAD: 7 ms
//S05_READ_SCRATCHPAD: 7 ms

//#define DEBUG


// DS18S20 Temperature chip i/o
OneWire ds(10);  // on pin 10

#define S01_SEARCH            1
#define S02_START_CONVERSION  2
#define S03_WAIT              3
#define S04_CMD_SCRATCHPAD    4
#define S05_READ_SCRATCHPAD   5

#define DENSITY 0.998203 //[kg/l]
#define PULSE_WEIGHT 0.5 //[l]
#define HEAT_CAPACITY 4.182 //[kJ/(kgK)]

//global cycle time
unsigned long time_last; //last lifetime in ms

//global flow rate
unsigned int time_flow_pulse; //duration of flow pulse in [ms]
unsigned long time_flow_pause; //duration between two flow pulses in [ms]
bool is_flow_pulse; //pulse identified

//global temperature difference
unsigned int wait_conversion; //wait for conversion to complete
#ifdef DEBUG_OW
unsigned int time_since_step;
#endif

byte addr[2][8]; //addresses of 2 sensors
byte isensor; //current sensor idex
byte data[12]; //raw data from sensor
double temperature[2]; //forward and backward temperature
int step_loop; //step in sequencer
int last_step_loop; //last step in sequencer
double tdiff_sum; //sum of temperature difference
unsigned int tdiff_count; //count of temperature difference
double tdiff; //current temperature difference


void setup(void) {
  // initialize inputs/outputs
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  pinMode(8, INPUT);
  //digitalWrite(8, HIGH);      // turn on pullup resistor
  // start serial port
  Serial.begin(9600);
  //init OneWire
  ds.reset_search();
  //init cycle time measurement
  time_last = millis();
}

void loop(void) {
  byte i;
  double current_temp; //converted from raw data
  double time_pulse; //time between pulses in seconds
  double flow_rate; //flow rate in [l/s]
  double heat_power; //current heat power in [kW]
  unsigned int cycle; //cycle time in [ms]
  unsigned long time_cur; //current lifetime in [ms]
#ifdef DEBUG
  unsigned int tdiff_cnt_old;
#endif

  //generate cycle time  
  time_cur = millis();
  if (time_cur >= time_last) {
    cycle = time_cur - time_last;
  }
  else { //millis() overflows approximately every 50 days
    cycle = 0xFFFFFF - time_last + time_cur;
  }
  time_last = time_cur;

  //get flow rate
  if (time_flow_pause >= 0xFFFFFF - cycle) {
    time_flow_pause = 0xFFFFFF; //no pulse for 50 days -> freeze
  }
  else {
    time_flow_pause = time_flow_pause + cycle;
  }
  if (time_flow_pause > 900000) {
        if (tdiff_count > 0) { //prevent div/0
          tdiff = tdiff_sum / tdiff_count;
          tdiff_sum = 0.0;
          tdiff_count = 0;
        }
        time_pulse = 0.0;
        flow_rate = 0.0;
        heat_power = 0.0;
        Serial.print(heat_power,3);
        Serial.print(",");
        Serial.print(flow_rate,3);
        Serial.print(",");
        Serial.print(tdiff,2);
        Serial.print(",");
        Serial.print(heat_power*time_pulse/3600,6);
        Serial.print(",");
        Serial.print(temperature[0],1);
        Serial.print(",");
        Serial.print(temperature[1],1);
        Serial.println(); //finish print values
        time_flow_pause = 0;
        time_flow_pulse = 0;
  }
  else if (!digitalRead(8)) {
    digitalWrite(13, HIGH); //LED
    time_flow_pulse = time_flow_pulse + cycle;
    if (time_flow_pulse >= 100) { //wait 100 ms before counting as pulse
      if (!is_flow_pulse) {
#ifdef DEBUG
        tdiff_cnt_old = tdiff_count;
#endif
        if (tdiff_count > 0) { //prevent div/0
          tdiff = tdiff_sum / tdiff_count;
          tdiff_sum = 0.0;
          tdiff_count = 0;
        }
        time_pulse = time_flow_pause / 1000.0;
        flow_rate = PULSE_WEIGHT / time_pulse;
        heat_power = flow_rate * DENSITY * HEAT_CAPACITY * tdiff;
        Serial.print(heat_power,3);
        Serial.print(",");
        Serial.print(flow_rate,3);
        Serial.print(",");
        Serial.print(tdiff,2);
        Serial.print(",");
        Serial.print(heat_power*time_pulse/3600,6);
        Serial.print(",");
        Serial.print(temperature[0],1);
        Serial.print(",");
        Serial.print(temperature[1],1);
#ifdef DEBUG
        Serial.print(",");
        Serial.print(tdiff_cnt_old);
        Serial.print(",");
        Serial.print(time_flow_pause);
        Serial.print(",");
#endif
        Serial.println(); //finish print values   
        time_flow_pause = 0;
        is_flow_pulse = true;
      }
    }
  }
  else {
    digitalWrite(13, LOW); //LED
    if (is_flow_pulse) {
#ifdef DEBUG
      Serial.println(time_flow_pulse);
#endif
    }
    time_flow_pulse = 0;
    is_flow_pulse = false;
  }

  
#ifdef DEBUG_OW
  //DEBUG temperature difference sequence
  time_since_step = time_since_step + cycle;

  if (last_step_loop != step_loop) {
    switch(last_step_loop) {
      case S01_SEARCH:
        Serial.print("S01_SEARCH: ");
        break;
      case S02_START_CONVERSION:
        Serial.print("S02_START_CONVERSION: ");
        break;
      case S03_WAIT:
        Serial.print("S03_WAIT: ");
        break;
      case S04_CMD_SCRATCHPAD:
        Serial.print("S04_CMD_SCRATCHPAD: ");
        break;
      case S05_READ_SCRATCHPAD:
        Serial.print("S05_READ_SCRATCHPAD: ");
        break;
    }
    Serial.print(time_since_step);
    Serial.println(" ms");
    time_since_step = 0;
  }
#endif

  //temperature difference sequence
  last_step_loop = step_loop;
  switch (step_loop) {
    case S01_SEARCH: //search for two sensors
      if (addr[isensor][0] != 0x28) //but only once
        if ( !ds.search(addr[isensor])) {
          ds.reset_search();
        }
        else {
          if ( OneWire::crc8( addr[isensor], 7) == addr[isensor][7]) {
            step_loop++;
          }
        }
      else {
        step_loop++;
      }
      break;
    case S02_START_CONVERSION:
      ds.reset();
      ds.select(addr[isensor]);
      ds.write(0x44,1);         // start conversion, with parasite power on at the end
      step_loop++;
      break;
    case S03_WAIT:
      // maybe 750ms is enough, maybe not
      wait_conversion = wait_conversion + cycle;
      if (wait_conversion >= 750) {
        wait_conversion = 0;
        step_loop++;
      }
      break;
    case S04_CMD_SCRATCHPAD:
      // we might do a ds.depower() here, but the reset will take care of it.
      ds.reset();
      ds.select(addr[isensor]);    
      ds.write(0xBE);         // Read Scratchpad
      step_loop++;
      break;
    case S05_READ_SCRATCHPAD:
      for ( i = 0; i < 9; i++) {           // we need 9 bytes
        data[i] = ds.read();
      }
      current_temp = ((data[1] << 8) + data[0] )*0.0625;
      temperature[isensor] = current_temp;
      tdiff = abs(temperature[0]-temperature[1]);
#ifdef DEBUG_OW
      Serial.print("Tdiff = "); 
      Serial.print(tdiff); 
      Serial.print(", T1 = "); 
      Serial.print(temperature[0]); 
      Serial.print(", T2 = "); 
      Serial.println(temperature[1]); 
#endif
      tdiff_sum = tdiff_sum + tdiff;
      tdiff_count = tdiff_count + 1;
      isensor = (isensor + 1) % 2; //only do two sensors
      step_loop = S01_SEARCH;
      break;
    default:
      step_loop = S01_SEARCH;
      break;
  } 
}


