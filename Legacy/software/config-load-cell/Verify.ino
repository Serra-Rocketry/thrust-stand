#include "HX711.h"

HX711 escala;    

const float fator_calib = 260443; 

void setup() {
  escala.begin(26, 27);
  Serial.begin(115200);
  escala.set_scale(fator_calib);
  escala.tare();
}

void loop() {
  Serial.println(escala.get_units()); 
  delay(100);
}
