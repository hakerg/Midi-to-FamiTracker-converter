#pragma once
#include "commons.h"
#include "Pattern.h"
#include "NoteTriggerData.h"
#include "MidiEvent.h"
#include "PitchCalculator.h"

class PlayingNesNote {
public:
    MidiEvent event;
    int keyAfterPitch;
    double frequencyAfterPitch;
    NoteTriggerData triggerData;
    double canInterruptSeconds;
    bool playing = true;

    PlayingNesNote(MidiEvent const& event, double startSeconds, NoteTriggerData const& triggerData, double canInterruptSeconds) :
        event(event), keyAfterPitch(event.key), frequencyAfterPitch(PitchCalculator::calculateFrequencyByKey(event.key)),
        triggerData(triggerData), canInterruptSeconds(canInterruptSeconds) {}
    
    void stop() {
        playing = false;
    }
};