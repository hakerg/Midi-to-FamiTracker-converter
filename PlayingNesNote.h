#pragma once
#include "commons.h"
#include "Pattern.h"
#include "NoteTriggerData.h"

class PlayingNesNote {
public:
    int midiChannel = -1;
    int midiKey = -1;
    int midiVelocity = -1;
    int detuneIndex = 0; // used to slightly detune notes with the same frequency as on different channels to avoid audio wave overlapping/cancelling
    double startSeconds = -1;
    NoteTriggerData triggerData;
    double canInterruptSeconds = -1;
    bool playing = false;

    PlayingNesNote() = default;

    PlayingNesNote(int midiChannel, int midiKey, int midiVelocity, int detuneIndex, double startSeconds, NoteTriggerData const& triggerData, double canInterruptSeconds) :
        midiChannel(midiChannel), midiKey(midiKey), midiVelocity(midiVelocity), detuneIndex(detuneIndex), startSeconds(startSeconds), triggerData(triggerData),
        canInterruptSeconds(canInterruptSeconds), playing(true) {}
    
    void stop() {
        playing = false;
    }
};