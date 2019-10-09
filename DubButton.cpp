#include "DubButton.h"

int     lastState              = HIGH;
ulong   lastPressTime          = 0;

bool    maybeClick             = false;

int     pressedForDuration     = 99999;


DubButton::DubButton(int btnPin)
{
    this->pin = btnPin;
    this->onPressedCallback     = [](){};
    this->onPressedForCallback  = [](){};
    this->onClickCallback       = [](){};
    this->onDoubleClickCallback = [](){};
}

void DubButton::init()
{
    lastState = digitalRead(this->pin);
}


void DubButton::onPressed(std::function<void()> callback)
{
    this->onPressedCallback = callback;
}

void DubButton::onPressedFor(int duration, std::function<void()> callback)
{
    pressedForDuration = duration;
    this->onPressedForCallback = callback;
}

void DubButton::onClick(std::function<void()> callback)
{
    this->onClickCallback = callback;
}

void DubButton::onDoubleClick(std::function<void()> callback)
{
    this->onDoubleClickCallback = callback;
}

void DubButton::loop()
{
    int oldLastState = lastState;
    lastState = digitalRead(this->pin);


    // pressed moment
    if(lastState == LOW && oldLastState == HIGH)
    {
        // if double click
        if(millis() - lastPressTime <= this->doubleClickDelay){
            maybeClick = false;
            this->onDoubleClickCallback();
        }
        else{
            // not double click
            this->onPressedCallback();
            maybeClick = true;
        }

        lastPressTime = millis();
    }
    else{
        // if still pressed
        if(digitalRead(this->pin) == LOW){
            if(millis() - lastPressTime > pressedForDuration){
                maybeClick = false;
                this->onPressedForCallback();
            }
        }
        else{
            if(maybeClick && millis() - lastPressTime > this->doubleClickDelay){
                maybeClick = false;
                this->onClickCallback();
            }
        }
    }

}
