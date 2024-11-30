#include "simlib.h"

Queue Reception();
Queue ComplaintDesk();
Store Consultants(5);
Store CoffeeMachines(3);
Store ATMs(4);


class Visitor : public Process{
    public:
        Visitor();
        void Behavior() override;
};
class VisitorWantsCoffee : public Process{};
class Consultant : public Process{};
class Reception : public Process{};
class ATM : public Process{};
class CoffeeMachine : public Process{};
class ATM : public Process{};
class ComplaintDesk : public Process{};


class UserGenerator : public Event{
    public:
        UserGenerator(double expTime){
            this->ExpTime = expTime;
        }
    private:
        double ExpTime;
    void Behavior(){
        (new Visitor())->Activate();
        this->Activate(Time + Exponential(ExpTime));
    }
};
class CashGuysGenerator : public Event{};


void Visitor::Behavior(){
    auto serviceType = Random();
    if(serviceType < 0.2){
        // Reception processing
    }
    else if(serviceType >= 0.2 && serviceType < 0.7){
        // ATM
    }
    else{
        // Consultants
    }
};


int main(){
    return 0;
}