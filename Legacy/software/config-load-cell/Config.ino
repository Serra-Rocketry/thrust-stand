#include "HX711.h"

HX711 escala;    

void setup() {
  escala.begin(26, 27);
  Serial.begin(115200);
  escala.set_scale();            
  escala.tare();
}

void loop() {
  Serial.println(escala.get_value(1), 0); 
  delay(100);
}
