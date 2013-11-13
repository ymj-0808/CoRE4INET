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

#include "RCBuffer.h"
#include <CTFrame_m.h>
#include <TTEScheduler.h>
#include "ApplicationBase.h"

namespace CoRE4INET {

Define_Module( RCBuffer);

RCBuffer::RCBuffer()
{
    bagExpired = true;
}

RCBuffer::~RCBuffer()
{
}

int RCBuffer::numInitStages() const
{
    if(CTBuffer::numInitStages()>1)
        return CTBuffer::numInitStages();
    else
        return 1;
}

void RCBuffer::initialize(int stage)
{
    CTBuffer::initialize(stage);
    if(stage==0){
        Timed::initialize();

        //Update displaystring
        setIsEmpty(true);
    }
}

void RCBuffer::handleMessage(cMessage *msg)
{
    CTBuffer::handleMessage(msg);

    if(destinationGates.size() > 0)
    {
        if (msg->arrivedOn("in"))
        {
            if (bagExpired)
            {
                EtherFrame *outgoingMessage = getFrame();
                if(outgoingMessage){
                    bagExpired = false;
                    numReset = 0;
                    //Send Message
                    for (std::list<cGate*>::iterator dgate = destinationGates.begin(); dgate != destinationGates.end(); ++dgate)
                    {
                        sendDirect(outgoingMessage->dup(),0,0, *dgate);
                    }
                    if(gate("out")->isConnected()){
                        send(outgoingMessage->dup(),"out");
                    }
                    recordPacketSent(outgoingMessage);
                    delete outgoingMessage;
                }
            }
        }
        else if (msg->arrivedOn("schedulerIn") && msg->getKind() == TIMER_EVENT)
        {
            EtherFrame *outgoingMessage = getFrame();
            if (outgoingMessage)
            {
                bagExpired = false;
                numReset = 0;
                //Send Message
                for (std::list<cGate*>::iterator gate = destinationGates.begin(); gate != destinationGates.end(); ++gate)
                {
                    sendDirect(outgoingMessage->dup(), *gate);
                }
                recordPacketSent(outgoingMessage);
                delete outgoingMessage;
            }
            else
            {
                bagExpired = true;
                if(ev.isGUI()){
                    getDisplayString().setTagArg("i2", 0, "");
                }
            }
            delete msg;
        }
    }

}

void RCBuffer::handleParameterChange(const char* parname){
    CTBuffer::handleParameterChange(parname);
}

void RCBuffer::resetBag()
{
    Enter_Method("resetBag()");
    //This is the moment when the message was transmitted execute transmit callbacks if there are some
    for(std::map<ApplicationBase*,Callback*>::const_iterator iter = transmitCallbacks.begin();
            iter != transmitCallbacks.end(); ++iter){
        iter->first->executeCallback(iter->second);
    }

    //Set icon:
    if(ev.isGUI()){
        getDisplayString().setTagArg("i2", 0, "status/hourglass");
    }

    numReset++;
    if (numReset == destinationGates.size())
    {
        //Register scheduler
        SchedulerTimerEvent *timerMessage = new SchedulerTimerEvent("RCBuffer Scheduler Event", TIMER_EVENT);
        timerMessage->setTimer(par("bag").doubleValue());
        timerMessage->setDestinationGate(gate("schedulerIn"));
        timer->registerEvent(timerMessage);
    }
}

} //namespace
