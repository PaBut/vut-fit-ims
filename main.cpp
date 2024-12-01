#include "simlib.h"

#define seconds(sec) sec
#define minutes(min) min * 60
#define hours(h) minutes(h * 60)

Facility VoucherMachine("voucher machine");
Facility Reception("reception");
Queue ReceptionQueue("reception");
Facility ComplaintDesk("complaint desk");
Store Consultants(4);
Queue ConsultantsQueue("consultants");
Store CoffeeMachines(3);

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

ATMs ATMsQueue(3);

class Visitor : public Process
{
public:
    Visitor() : goesForCoffee(false){}
    void Behavior() override;
    void setGoForCoffee() { goesForCoffee = true; }

private:
    bool goesForCoffee;
    void WentForCoffee();
    void MakeAComplaint(double percent);
    void CallNextCustomer()
    {
        if (!ConsultantsQueue.Empty())
        {
            ConsultantsQueue.GetFirst()->Activate();
        }
    }
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
            Priority = 1;
            while (takenATMs < atmCount)
            {
                if(ATMsQueue._Store.Full()){
                    ATMsQueue._Queue.InsFirst(this);
                    Passivate();
                }
                Enter(ATMsQueue._Store);
                takenATMs++;
            }

            ATMsQueue.AreProcessed = true;
            Wait(Normal(minutes(30), minutes(5)));
            Leave(ATMsQueue._Store, atmCount);
            ATMsQueue.AreProcessed = false;
        }
    }
};

void Visitor::Behavior()
{
    bool needsAnotherService = true;
    while (needsAnotherService)
    {
        needsAnotherService = false;
        auto serviceType = Random();
        if (serviceType < 0.2)
        {
            // Reception processing
            while (Reception.Busy() || !ReceptionQueue.Empty())
            {
                Into(ReceptionQueue);
                auto wantsCoffee = new VisitorWantsCoffee(this, minutes(4));
                wantsCoffee->Activate();
                Passivate();

                if (goesForCoffee)
                {
                    Out();
                    WentForCoffee();
                    goesForCoffee = false;
                }
                else
                {
                    wantsCoffee->Cancel();
                    break;
                }
            }

            if (isInQueue())
            {
                Out();
            }

            Seize(Reception);
            Wait(Normal(minutes(5), minutes(2)));
            Release(Reception);
            if (!ReceptionQueue.Empty())
            {
                ReceptionQueue.GetFirst()->Activate();
            }

            if (Random() < 0.8)
            {
                needsAnotherService = true;
            }
        }
        else if (serviceType >= 0.2 && serviceType < 0.6)
        {
            // ATMs
            if (ATMsQueue.AreProcessed)
            {
                return;
            }

            if (!ATMsQueue._Store.Empty())
            {
                Into(ATMsQueue._Queue);
                Passivate();
                // Out();
            }

            if (ATMsQueue.AreProcessed)
            {
                return;
            }

            Enter(ATMsQueue._Store);
            Wait(Exponential(minutes(3)));
            Leave(ATMsQueue._Store);

            if (!ATMsQueue._Queue.Empty())
            {
                ATMsQueue._Queue.GetFirst()->Activate();
            }
        }
        else if (serviceType >= 0.6 && serviceType < 0.9)
        {
            // Consultants
            auto isPremium = Random() < 0.15;
            if (!isPremium)
            {
                Seize(VoucherMachine);
                Wait(Normal(seconds(20), seconds(5)));
                Release(VoucherMachine);
            }

            while (!Consultants.Free() || !ConsultantsQueue.Empty())
            {
                // function to check if the visitor goes for coffee
                if (isPremium)
                {
                    Priority = 1;
                }
                Into(ConsultantsQueue);
                auto wantsCoffee = new VisitorWantsCoffee(this, minutes(8));
                wantsCoffee->Activate();
                Passivate();

                if (goesForCoffee)
                {
                    Out();
                    WentForCoffee();
                    goesForCoffee = false;
                }
                else
                {
                    wantsCoffee->Cancel();
                    break;
                }
            }
            
            /// ???
            // if (isInQueue())
            // {
            //     Out();
            // }

            Enter(Consultants);

            Priority = 0;

            // documents check
            auto isDocumentsValid = Random();
            if (isDocumentsValid < 0.25)
            {
                // No
                Leave(Consultants);
                CallNextCustomer();
                // Make a complaint
                MakeAComplaint(0.75);
            }
            else
            {
                // Yes
                auto consulatationProccess = Random();
                if (consulatationProccess < 0.5)
                {
                    // Registration
                    Wait(Normal(minutes(20), minutes(5)));
                    Leave(Consultants);
                    CallNextCustomer();
                    // Could continue ?????
                }
                else if (consulatationProccess >= 0.5 && consulatationProccess < 0.7)
                {
                    // Investment plan
                    Wait(Normal(minutes(40), minutes(10)));
                    Leave(Consultants);
                    CallNextCustomer();
                    // Could continue ?????
                }
                else if (consulatationProccess >= 0.7 && consulatationProccess < 0.8)
                {
                    // Fraud investigation
                    Wait(Normal(minutes(10), minutes(2)));
                    auto isSolutionFound = Random();
                    Leave(Consultants);
                    CallNextCustomer();
                    if (isSolutionFound < 0.8)
                    {
                        // No
                        MakeAComplaint(0.8);
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
                        CallNextCustomer();
                    }
                    else
                    {
                        // No
                        Leave(Consultants);
                        CallNextCustomer();
                        MakeAComplaint(0.4);
                    }
                }
            }
        }
        else
        {
            Priority = -1;
            MakeAComplaint(1);
        }
    }

    Terminate();
};

void Visitor::WentForCoffee()
{
    Enter(CoffeeMachines);
    Wait(minutes(1));
    Leave(CoffeeMachines);
}

void Visitor::MakeAComplaint(double percent)
{
    auto wantsMakeComplaint = Random();
    if (wantsMakeComplaint < percent)
    {
        Seize(ComplaintDesk);
        Wait(minutes(1));
        Release(ComplaintDesk);
    }
}

int main()
{
    RandomSeed(time(NULL));
    SetOutput("bank.out");
    Init(0, hours(9));
    (new VisitorGenerator(minutes(5)))->Activate();
    (new CashGuys(hours(6)))->Activate();
    Run();

    CoffeeMachines.Output();
    ConsultantsQueue.Output();
    ReceptionQueue.Output();
    ComplaintDesk.Output();
    VoucherMachine.Output();
    Reception.Output();
    Consultants.Output();
    ATMsQueue._Store.Output();
    ATMsQueue._Queue.Output();

    return 0;
}

// Priority for premium users towards consultans - Done
// Change the probability for complaints from the beginning - Done
// After consultant priority for complaint is higher - Done

// User returns back with some probabily !!!!!!!