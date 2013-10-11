//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "TTBuffer.h"
#include <TTEScheduler.h>
#include <TTBufferEmpty_m.h>
#include "ApplicationBase.h"

#include <ModuleAccess.h>

using namespace TTEthernetModel;

Define_Module( TTBuffer);

TTBuffer::TTBuffer()
{
    actionTimeEvent = new SchedulerActionTimeEvent("TTBuffer Scheduler Event", ACTION_TIME_EVENT);
}

TTBuffer::~TTBuffer()
{
}

int TTBuffer::numInitStages() const
{
    return 2;
}

void TTBuffer::initialize(int stage)
{
    CTBuffer::initialize(stage);
    if(stage==0){
        nextAction = par("sendWindowStart").longValue();
    }
    if(stage==1)
    {
        ev << "Initialize TTBuffer" << endl;

        //Register Event
        Scheduled::initialize();

        //Dirty FIX
        //TODO find out what is wrong here!
        if(actionTimeEvent->isScheduled())
            cancelEvent(actionTimeEvent);
        actionTimeEvent->setAction_time(par("sendWindowStart").longValue());
        actionTimeEvent->setDestinationGate(gate("schedulerIn"));
        nextAction = period->registerEvent(actionTimeEvent);

        setIsEmpty(true);
        return;
    }
}

void TTBuffer::handleMessage(cMessage *msg)
{
    bool arrivedOnSchedulerIn = msg->arrivedOn("schedulerIn");

    CTBuffer::handleMessage(msg);

    if (arrivedOnSchedulerIn && msg->getKind() == ACTION_TIME_EVENT && destinationGates.size() > 0)
    {
        cMessage *outgoingMessage = getFrame();
        //Send Message
        for (std::list<cGate*>::iterator destGate = destinationGates.begin(); destGate != destinationGates.end(); ++destGate)
        {
            if (outgoingMessage)
            {
                sendDirect(outgoingMessage->dup(), *destGate);
            }
            else
            {
                sendDirect(new TTBufferEmpty("TT Buffer Empty"), *destGate);
            }
        }
        if(gate("out")->isConnected()){
            if (outgoingMessage){
                send(outgoingMessage->dup(),"out");
            }
            else{
                send(new TTBufferEmpty("TT Buffer Empty"),"out");
            }
        }
        if (outgoingMessage)
        {
            recordPacketSent();
            delete outgoingMessage;

            // Now execute transmit callbacks if there are some
            for(std::map<ApplicationBase*,Callback*>::const_iterator iter = transmitCallbacks.begin();
                    iter != transmitCallbacks.end(); ++iter){
                iter->first->executeCallback(iter->second);
            }
        }
        //Reregister scheduler
        static_cast<SchedulerActionTimeEvent *>(msg)->setNext_cycle(true);
        nextAction = period->registerEvent(static_cast<SchedulerActionTimeEvent *>(msg));
    }
}

void TTBuffer::handleParameterChange(const char* parname){
    CTBuffer::handleParameterChange(parname);

    if(actionTimeEvent)
        actionTimeEvent->setAction_time(par("sendWindowStart").longValue());
}

uint64_t TTBuffer::nextSendWindowStart(){
    return nextAction;
}
