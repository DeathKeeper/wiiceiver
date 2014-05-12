#ifndef THROTTLE_H
#define THROTTLE_H

// not strictly necessary, but a nice reminder
#include "Chuck.h"
#include "Smoother.h"

/*
 * Manages the throttle input; presents a smoothed output, [ -1 .. 1 ]
 */

class Throttle {
#define EEPROM_AUTOCRUISE_ADDY 1
  private:
    float autoCruise, throttle, smoothed;
    int xCounter;
    Smoother smoother;
    
    
    void readAutoCruise(void) {
      byte storedValue = EEPROM.read(EEPROM_AUTOCRUISE_ADDY);
#ifdef DEBUGGING_THROTTLE
      Serial.print("Read autoCruise from EEPROM: ");
      Serial.print(storedValue);
#endif
      if (storedValue > 0 && storedValue < 100) {
        autoCruise = 0.01 * storedValue;
#ifdef DEBUGGING_THROTTLE
        Serial.print("; setting autoCruise = ");
        Serial.println(autoCruise);
#endif  
      } else {
#ifdef DEBUGGING_THROTTLE
        Serial.print("; ignoring, setting autoCruise = ");
        Serial.println(autoCruise);
#endif        
      }
    } // readAutoCruise(void) 
    
    
    void writeAutoCruise(void) {
      int storedValue = autoCruise * 100;
      EEPROM.write(EEPROM_AUTOCRUISE_ADDY, storedValue);
#ifdef DEBUGGING_THROTTLE
      Serial.print("Storing autoCruise as ");
      Serial.println(storedValue);
#endif         
    }
    
    
  public:    
    
    Throttle() {
      smoother = Smoother();
      throttle = 0;
      autoCruise = THROTTLE_MIN_CC;
      xCounter = 0;
    } // Throttle()
    

    void init(void) {
      readAutoCruise();
    }
    
    /*
     * returns a smoothed float [-1 .. 1]
     *
     * Theory of Operation: identify the throttle position (joystick angle), 
     *   then return a smoothed representation
     *
     *   if C is pressed, "cruise control":
     *      set "cruise" to last joystick position
     *      if joystick == up, increment throttle position (Z button: 3x increment)
     *      if joystick == down, decrement throttle position  (Z button: 3x decrement)
     *   else throttle position == chuck.Y joystick position
     *   return a smoothed value from the throttle position (Z button: 4x less smoothed)
     */
    float update(Chuck chuck) {
#ifdef DEBUGGING_THROTTLE
      Serial.print("Throttle: ");
      Serial.print("y=");
      Serial.print(chuck.Y, 4);
      Serial.print(", ");
      Serial.print("c=");
      Serial.print(chuck.C);
      Serial.print("; ");
#endif

      if (checkAutoCruise(chuck)) {
        return throttle;  // we're looking for autoCruise, so do that -- don't change the throttle
      }
      
      if (chuck.C) { // cruise control!
/*
#ifdef DEBUGGING_THROTTLE
        Serial.print("CC: last = ");
        Serial.print(throttle, 4);
        Serial.print(", ");
#endif
        
        if (throttle < autoCruise) {
          throttle += THROTTLE_CC_BUMP * 1.5;
        } else {
          if (chuck.Y > 0.5 && throttle < 1.0) {
            throttle += THROTTLE_CC_BUMP * (chuck.Z ? 2 : 1);
          } else if (chuck.Y < -0.5 && throttle > -1.0) {
            throttle -= THROTTLE_CC_BUMP * (chuck.Z ? 2 : 1);
          } // if (chuck.Y > 0.5 && throttle < 1.0) - else
        } // if (throttle < THROTTLE_MIN_CC) - else
*/
       if (chuck.Y > 0.5 && throttle < 1.0) {
         throttle += THROTTLE_CC_BUMP * (chuck.Z ? 2 : 1);
       } else {
         if (chuck.Y < -0.5 && throttle > -1.0) {
           throttle -= THROTTLE_CC_BUMP * (chuck.Z ? 2 : 1);
         } else {
           if (throttle < autoCruise) {
             throttle += THROTTLE_CC_BUMP * 1.5;
           }
         }
       } // if (chuck.Y > 0.5 && throttle < 1.0) - else

      } else { // if (checkAutoCruise() && chuck.C)
        throttle = chuck.Y;
        // "center" the joystick by enforcing some very small minimum
        if (abs(throttle) < THROTTLE_MIN) {
          throttle = 0;
        }
      } // if (chuck.C) -- else

      smoothed = smoother.compute(throttle, THROTTLE_SMOOTHNESS * (chuck.Z ? 4 : 1));

#ifdef DEBUGGING_THROTTLE
      Serial.print(F("position: "));
      Serial.print(throttle, 4);
      Serial.print(F(" smoothed: "));
      Serial.println(smoothed, 4);
#endif

      return smoothed;
    } // float update(void)
    
    
    /*
     * returns 'true' if we're in a "set autocruise level" state:
     *   while in cruise (chuck.C), with no throttle input (chuck.Y =~ 0), 
     *   nonzero throttle, full X deflection (chuck.X > 0.75) ... for N cycles
     * set "autoCruise" to the current throttle level.
     *   
     */
    bool checkAutoCruise(Chuck chuck) {
      if (! chuck.C) {
#ifdef DEBUGGING_THROTTLE
        Serial.println("checkAutoCruise: no C");
#endif
        xCounter = 0;
        return false;
      }
      if (abs(chuck.Y) < 0.5 && 
          abs(chuck.X) > 0.75) {
        ++xCounter;
#ifdef DEBUGGING_THROTTLE
        Serial.print("checkAutoCruise: xCounter = ");
        Serial.println(xCounter);
#endif
        if (xCounter == 150) { // ~3s holding X
          setAutoCruise();  
        }
        return true;
      } else {
        xCounter = 0;
#ifdef DEBUGGING_THROTTLE
        Serial.println("checkAutoCruise: no X or Y");
        Serial.print("x = ");
        Serial.print(abs(chuck.X));
        Serial.print(", y = ");
        Serial.println(abs(chuck.Y));
#endif        
        return false;
      }
    } // bool checkAutoCruise(void)
    
    
    void setAutoCruise(void) {
#ifdef DEBUGGING_THROTTLE
      Serial.print("Setting autoCruise = ");
      Serial.println(throttle);
#endif
      autoCruise = throttle;
      writeAutoCruise();
    } // setAutoCruise(void)
    
    
    float getThrottle(void) {
      return smoothed;
    } // float getThrottle()
    
    
    void zero(void) {
      throttle = 0;
      smoother.zero();
    } // void zero(void)
    
}; // class Throttle

#endif
