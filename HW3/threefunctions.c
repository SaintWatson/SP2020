#include "threadutils.h"

void BinarySearch(int thread_id, int init, int maxiter)
{
    /* Initilization */
    ThreadInit(thread_id, init, maxiter);
    int mid;
    Current->y = 0;
    Current->z = 100;

    /* Enter this function before schedule() executed. */
    if(setjmp(Current->Environment) == 0)
        return;

    for (Current->i = 0; Current->i < Current->N; ++Current->i)
    {
        sleep(1);
        mid = (Current->y + Current->z) / 2;
        printf("BinarySearch: %d\n", mid);

        if(mid > Current->x)
            Current->z = mid-1;
        else if (mid < Current->x)
            Current->y = mid+1;
        else
            break;

        ThreadYield();
    }
    ThreadExit()
}

void BlackholeNumber(int thread_id, int init, int maxiter)
{
    /* Initilization */
    ThreadInit(thread_id, init, maxiter);
    int a,b,c,t;

    /* Enter this function before schedule() executed. */
    if(setjmp(Current->Environment) == 0)
        return;

    for (Current->i = 0; Current->i < Current->N; ++Current->i)
    {
        sleep(1);
        a = Current->x / 100;
        b = (Current->x / 10) % 10;
        c = Current->x % 10;
        
        t = (a>b) ? a : b;
        Current->y = (c>t) ? c : t; // max digit
        
        t = (a<b) ? a : b;
        Current->z = (c<t) ? c : t; // min digit

        Current->x = 99 * (Current->y - Current->z);

        printf("BlackholeNumber: %d\n", Current->x);
        if(Current->x == 495)
            break;

        ThreadYield();
    }
    ThreadExit()
}

void FibonacciSequence(int thread_id, int init, int maxiter)
{
    /* Initilization */
    ThreadInit(thread_id, init, maxiter);
    Current->x = 0;
    Current->y = 1;

    /* Enter this function before schedule() executed. */
    if(setjmp(Current->Environment) == 0)
        return;

    for (Current->i = 0; Current->i < Current->N; ++Current->i)
    {
        sleep(1);
        Current->z = Current->x + Current->y;
        printf("FibonacciSequence: %d\n", Current->z);
        Current->x = Current->y;
        Current->y = Current->z;
        ThreadYield();
    }
    ThreadExit()
}
