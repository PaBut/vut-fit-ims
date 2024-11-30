#include "simlib.h"

#define minutes(min) min * 60
#define hours(hours) hours * 60 * 60
#define seconds(sec) sec * 1

Facility VoucherMachine("voucher machine");
Facility Reception("reception");
Queue ReceptionQueue("reception");
Facility ComplaintDesk("complaint desk");
Store Consultants(5);
Queue ConsultantsQueue("consultants");
Store CoffeeMachines(3);
// Store ATMs(4);
// Queue ATMsQueue("atms");

class ATMs
{
public:
    Store _Store;
    Queue _Queue;
    bool AreProcessed;
    ATMs(int atmCount) : _Store(atmCount), _Queue("atms")
    {
        AreProcessed = false;
    }
};

ATMs ATMsQueue(4);

class PriorityStore : public Store
{
public:
    PriorityStore(int capacity) : Store(capacity) {}
};

class Visitor : public Process
{
public:
    Visitor();
    void Behavior() override;
    void setGoForCoffee() { goesForCoffee = true; }
    void setSkipATM() { skipATM = true; }

private:
    bool goesForCoffee;
    bool skipATM;
    void WentForCoffee();
    void MakeAComplaint(double percent);
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

// Some fun logic incoming, there it was left

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
class CashGuys : public Process
{
private:
    int frequencyArrival;

public:
    bool atmsAreProcessing = false;
    CashGuys(int frequencyArrival)
    {
        this->frequencyArrival = frequencyArrival;
    }
    void Behavior() override
    {
        while (true)
        {
            Wait(Exponential(frequencyArrival));
            auto atmCount = ATMsQueue._Store.Capacity();
            auto takenATMs = 0;
            while (takenATMs < atmCount)
            {
                Priority = 1;
                Into(ATMsQueue._Queue);
                Passivate();
                Enter(ATMsQueue._Store);
            }

            while (!ATMsQueue._Queue.Empty())
            {
                Visitor *visitor = dynamic_cast<Visitor *>(ATMsQueue._Queue.GetFirst());
                if (visitor)
                {
                    visitor->setSkipATM();
                }
            }
            ATMsQueue.AreProcessed = true;

            Wait(Normal(minutes(30), minutes(5)));
            Leave(ATMsQueue._Store, atmCount);
            ATMsQueue.AreProcessed = true;
        }
    }
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
        if (ATMsQueue.AreProcessed)
        {
            return;
        }

        Into(ATMsQueue._Queue);
        Passivate();

        if (ATMsQueue.AreProcessed)
        {
            return;
        }

        Enter(ATMsQueue._Store);
        Wait(Exponential(minutes(3)));
        Leave(ATMsQueue._Store);
        ATMsQueue._Queue.GetFirst()->Activate();
    }
    else
    {
        // Consultants
        auto isPremium = Random();
        if (isPremium < 0.85)
        {
            Seize(VoucherMachine);
            Wait(Normal(seconds(20), seconds(5)));
            Release(VoucherMachine);
        }
        while (Consultants.Free() == 0)
        {
            // function to check if the visitor goes for coffee
            Into(ConsultantsQueue);
            auto wantsCoffee = new VisitorWantsCoffee(this, minutes(8));
            wantsCoffee->Activate();

            if (goesForCoffee)
            {
                // TODO pasha lox
                Out();
                WentForCoffee();
            }
        }
        Enter(Consultants);

        // documents check
        auto isDocumentsValid = Random();
        if (isDocumentsValid < 0.25)
        {
            // No
            Leave(Consultants);
            // Make a complaint
            MakeAComplaint(0.75);
            return;
        }
        else
        {
            // Yes
            Wait(Normal(minutes(10), minutes(2)));
            Leave(Consultants);
            auto consulatationProccess = Random();
            if (consulatationProccess < 0.5)
            {
                // Registration
                Wait(Normal(minutes(20), minutes(5)));
                Leave(Consultants);
                return;
            }
            else if (consulatationProccess >= 0.5 && consulatationProccess < 0.7)
            {
                // Investment plan
                Wait(Normal(minutes(40), minutes(10)));
                Leave(Consultants);
                return;
            }
            else if (consulatationProccess >= 0.5 && consulatationProccess < 0.7)
            {
                // Fraud investigation
                Wait(Normal(minutes(10), minutes(2)));
                auto isSolutionFound = Random();
                if (isSolutionFound < 0.2)
                {
                    // Yes
                    Leave(Consultants);
                    return;
                }
                else
                {
                    // No
                    Leave(Consultants);
                    MakeAComplaint(0.8);
                    return;
                }
            }
            else
            {
                // Taking a loan Background check
                auto isBackgroundCheckValid = Random();
                if (isBackgroundCheckValid < 0.8)
                {
                    // Yes
                    Wait(Normal(minutes(30), minutes(5)));
                    Leave(Consultants);
                    return;
                }
                else
                {
                    // No
                    Leave(Consultants);
                    MakeAComplaint(0.4);
                    return;
                }
            }
        }
    }
};

void Visitor::WentForCoffee()
{
    Enter(CoffeeMachines);
    Wait(minutes(1));
    Leave(CoffeeMachines);
}

void Visitor::MakeAComplaint(double percent)
{
    auto wantsMakeComplaint = random();
    if (wantsMakeComplaint < percent)
    {
        Seize(ComplaintDesk);
        Wait(minutes(1));
        Release(ComplaintDesk);
    }
}

int main()
{
    return 0;
}