#include "simlib.h"

#define minutes(min) min * 60

Facility Reception("reception");
Queue ReceptionQueue("reception");
Facility ComplaintDesk();
Store Consultants(5);
Store CoffeeMachines(3);
Store ATMs(4);

class Visitor : public Process
{
public:
    Visitor();
    void Behavior() override;
    void setGoForCoffee();

private:
    bool goesForCoffee;
    void WentForCoffee();
};

class VisitorWantsCoffee : public Process
{
private:
    Visitor *visitor;
    int maxAwaitTime;

public:
    VisitorWantsCoffee(Visitor *visitor, int maxAwaitTime)
    {
        this->visitor = visitor;
        this->maxAwaitTime = maxAwaitTime;
    }

    void Behavior() override
    {
        visitor->Passivate();
        Wait(maxAwaitTime);
        visitor->Activate();
        visitor->setGoForCoffee();
    }
};

class ATM : public Process
{
};

class VisitorGenerator : public Event
{
public:
    VisitorGenerator(double expTime)
    {
        this->ExpTime = expTime;
    }

private:
    double ExpTime;
    void Behavior()
    {
        (new Visitor())->Activate();
        this->Activate(Time + Exponential(ExpTime));
    }
};
class CashGuysGenerator : public Event
{
};

void Visitor::Behavior()
{
    auto serviceType = Random();
    if (serviceType < 0.2)
    {
        // Reception processing
        while (Reception.Busy())
        {
            Into(ReceptionQueue);
            auto wantsCoffee = new VisitorWantsCoffee(this, minutes(8));
            wantsCoffee->Activate();

            if (goesForCoffee)
            {
                Out();
                WentForCoffee();
            }
        }

        Seize(Reception);
        Wait(Normal(minutes(5), minutes(2)));
        Release(Reception);
        if (!ReceptionQueue.Empty())
        {
            ReceptionQueue.GetFirst()->Activate();
        }
    }
    else if (serviceType >= 0.2 && serviceType < 0.7)
    {
        // ATM
    }
    else
    {
        // Consultants
    }
};

void Visitor::WentForCoffee()
{
    Enter(CoffeeMachines);
    Wait(minutes(1));
    Leave(CoffeeMachines);
}

int main()
{
    return 0;
}