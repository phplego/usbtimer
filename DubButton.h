#include <Arduino.h>


class DubButton
{
    private:
        int pin;
        std::function<void()> onPressedCallback;
        std::function<void()> onPressedForCallback;
        std::function<void()> onClickCallback;
        std::function<void()> onDoubleClickCallback;

    public:
        ulong doubleClickDelay = 300;

        DubButton(int pin);
        void init();
        void onPressed(std::function<void()> callback);
        void onPressedFor(int forDuration, std::function<void()> callback);
        void onClick(std::function<void()> callback);
        void onDoubleClick(std::function<void()> callback);
        void loop();
};