#include <math.h>           //For event subscriber...

int led1 = 7;               //Visual feedback on LED on this pin, default to onboard LED.
int distance = 0;           //Distance (exposed)
int lastValue = 0;          //Last distance read
int timeout = 2000;         //Time between detecting the last wave and sending the event.
int last_wave = 0;          //Time last wave was detected
bool sent_event = true;     //Flag indicating we've sent the last lot of waves.
int number_of_waves = 0;    //Counts the number of waves as they are happening
int num_waves = 0;          //Holds the last number of REGISTERED waves (exposed)
bool can_detect = true;     //Flag to stop detecting the hand just moving in the beam.
int wave_limit = 400;       //How much change in signal we need to see before we register it.
int distance_cutoff = 1000; //Stop's the sensor detecting things moving too far away to be sensible.

//PAT MOD (Q3)
bool could_be_pat = false;  //Set this when we get a wave but the hand hasn't left the beam "enough"

char publishString[20];

//======================================================================
//SUBSCRIBE CODE -- Testing on the same Photon....
int feedbackLED = A5;
int i= 0;
int feedbackLEDValue = 0;
int lastEvent = 0;
int num_received_waves = 0;

void flashFeedbackLED(int numFlashes)
{
    for(int i = 0; i < numFlashes; i++)
    {
        digitalWrite(feedbackLED, HIGH);
        delay(100);
        digitalWrite(feedbackLED, LOW);
        delay(50);
    }
}

void eventHandler (const char* event, const char* data)
{
    //ok did we get a pat or a wave?
    if(data)
    {
        char code = data[0];
        switch(code)
        {
            case 'P':   //got a pat...
                lastEvent = 2;
                Serial.println("Pat");
                break;
            case 'W':   //got a wave...how many?
                lastEvent = 1;
                num_received_waves = atoi(data+1);
                Serial.print("Waves:");
                Serial.print(num_received_waves);
                Serial.println("");
                flashFeedbackLED(num_received_waves);
                break;
        }
    }
    
    //undertake some serial output for debugging...
    i++;
    Serial.print(i);
    Serial.print(event);
    Serial.print(", data: ");
    if (data)
        Serial.println(data);
    else
        Serial.println("NULL");   
}

void updateFeedbackLED()
{
    switch(lastEvent)
    {
        case 1: //waves...
            //Nothing to do, quick flashes occur straight away...
            break;
        case 2: //pat....
            float val = (exp(sin(millis()/2000.0* 3.1415)) - 0.36787944 ) * 108.0;
            analogWrite(feedbackLED, val);
            break;
    }
}

//END SUBSCRIBE CODE
//========================================================================

void flash(int numFlashes = 1) 
{
    for(int i = 0; i < numFlashes; i++)
    {
        digitalWrite(led1, HIGH);
        delay(50);
        digitalWrite(led1, LOW);
        delay(50);
    }
}

void setup() {
    pinMode(led1, OUTPUT);
    Particle.variable("distance", distance);
    Particle.variable("numWaves", num_waves);
    distance = analogRead(A0);
    lastValue = distance;
    
    //SUBSCRIBE CODE -- Testing on same Photon...
    Particle.subscribe("217015925_Wave", eventHandler);
    pinMode(feedbackLED, OUTPUT);
}

void loop() {
    distance = analogRead(A0);
    
    //SUBSCRIBE CODE -- Testing on same photon...
    updateFeedbackLED();
    
    //============================================
    //PAT MOD(Q3)
    if(distance < distance_cutoff && number_of_waves > 0)
    {
        //hand has left the sensor's "sensible" range, not a pat.
        could_be_pat = false;
    }
    //END PAT MOD
    //============================================
    
    if(abs(distance - lastValue) > wave_limit)
    {
        //Motion detected!
        if(distance - lastValue > wave_limit && can_detect && distance > distance_cutoff)
        {
            //...but a long to short distance is the only
            //time we want to class it as a "wave". This is only hit
            //as well if the can_detect flag is true. This stops us
            //getting multiple detections if you leave the hand in the "beam" and
            //move it back and forth, and has to be close enough to the sensor to be reasonable.
            flash();
            last_wave = millis();
            sent_event = false;
            can_detect = false;
            number_of_waves++;
        }
        else
        {
            can_detect = true;
        }
        lastValue = distance;
    }
    
    //============================================
    //PAT MOD (Q3)
    if(could_be_pat && number_of_waves >= 3)
    {
        could_be_pat = true;
        can_detect = false;
        number_of_waves = 0;
        sent_event = true;
        sprintf(publishString, "P");
        Particle.publish("217015925_Wave", publishString);
        flash(4);
    }
    //============================================
    
    //ok so send the waving gestures...
    if(millis() - last_wave > timeout && !sent_event || number_of_waves >= 9)
    {
        sent_event = true;
        could_be_pat = true;   //PAT MOD (Q3)
        can_detect = true;
        num_waves = number_of_waves;
        number_of_waves = 0;
        sprintf(publishString, "W%i", num_waves);
        Particle.publish("217015925_Wave", publishString);
        flash(2);
    }
    
    delay(100);
}