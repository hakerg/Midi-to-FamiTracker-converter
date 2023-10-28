#pragma once
#include "commons.h"
#include "Pattern.h"
#include "NoteTriggerData.h"

class PlayingNesNote {
public:
    int midiChannel;
    int midiKey;
    int keyAfterPitch;
    int midiVelocity;
    int detuneIndex; // used to slightly detune notes with the same frequency as on different channels to avoid audio wave overlapping/cancelling
    double startSeconds;
    NoteTriggerData triggerData;
    double canInterruptSeconds;
    bool playing = true;

    PlayingNesNote(int midiChannel, int midiKey, int midiVelocity, int detuneIndex, double startSeconds, NoteTriggerData const& triggerData, double canInterruptSeconds) :
        midiChannel(midiChannel), midiKey(midiKey), keyAfterPitch(midiKey), midiVelocity(midiVelocity), detuneIndex(detuneIndex), startSeconds(startSeconds), triggerData(triggerData),
        canInterruptSeconds(canInterruptSeconds) {}
    
    void stop() {
        playing = false;
    }
};