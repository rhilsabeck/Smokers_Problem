/* Author: R. Hilsabeck */
/* CSC 420/520 */
/* Parallel Cigarette-Smoker Problem */
// include headers

#include <iostream>
#include <Windows.h>
#include <conio.h>
#include <stdio.h>
#include <time.h>
#include <string>
#include <stdlib.h>


using namespace std;

void agent();
void smoker(int);


// shared vars for the smoke
enum iState {EMPTY =0, TOBACCO, PAPER, MATCHES};
enum sState {GONE =0 , PRESENT, CRAVING, SMOKING, DONE};
enum combo {PAPER_MATCH =0, MATCH_TOBACCO, TOBACCO_PAPER}; //different item combinations
string states[] = {"GONE","PRESENT","CRAVING","SMOKING","DONE"}; // get string of enum of state of smoker
string onTable[] = {"Paper & Match", "Match & Tobacco", "Tobacco & Paper"}; //get string of enum of items on table

int item[3] = {EMPTY}; /* holds iState */
int smokerCount = 0;
int smokerState[3] = {GONE};
int numTimesSmoked[3] = {0,0,0};
int tableEmpty = 1; //is table empty or not


// locks to create mutual exclusion
HANDLE mutex=CreateMutex(NULL, FALSE, NULL);
HANDLE tableStatus=CreateMutex(NULL, FALSE, NULL); //lock to change table from empty to full and vice versa
HANDLE writersLock=CreateMutex(NULL,FALSE,NULL); //lock for putting down items for agent and picking up from smokers

// ********** Create 3 smokers and an agent & set them smoking ****************
void main()
{ // Set up 4 threads
	HANDLE sThread[4];                  // 4 smokers
	DWORD  sThreadID[4];                // PID of thread
	DWORD WINAPI proc(LPVOID);          // code for the 4 smokers

	// seed rand # generator
  //seed rand # generator
  srand (static_cast<unsigned int>(time(NULL)));   // uncomment this when you have it running

	// start 4 threads
	for(int smokerNbr = 0; smokerNbr < 4; smokerNbr++)
	{  
		sThread[smokerNbr]=CreateThread(NULL,0,proc,NULL,0,&sThreadID[smokerNbr]);
	}

	WaitForMultipleObjects(4, sThread, true, INFINITE); // wait for threads to finish
	cout << "press CR to end."; while (_getch()!='\r');
	return;
}

DWORD WINAPI proc(LPVOID)
{ int myID;

WaitForSingleObject(mutex, INFINITE);   // lock the lock
myID = smokerCount++;
ReleaseMutex(mutex);                    // unlock the lock
smokerState[myID] = PRESENT;

if (myID == 3)
{    cout << "\n Calling Agent \n"; 

agent();
}
else
{   cout <<"\n Calling Smoker \n ";
smokerState[myID]=PRESENT;
smoker(myID);
}
return 0;
}

void agent()
{ 
	/* wait for all three smokers to report */
	while(smokerCount < 3)Sleep(0);
	int nextSmokerCombo = 0+rand()%3; //select random integer from 0 to 2 to decide what items to place on table
	while(true){
		//Check to see if the item combo that belongs to a smoker, still has smoker at the table
		if(smokerState[nextSmokerCombo] == CRAVING && numTimesSmoked[nextSmokerCombo]<100)
		{
		//Making sure that smoker is not in the middle of it's last smoke so you don't select that combo, if you do select that item
	    //combo of someone who is about to be done, then this could cause deadlock so you wait a bit and keep picking a random integer
	    //until you know for sure that the combo items that you are picking are actually needed by a smoker at the table still
		if(numTimesSmoked[nextSmokerCombo]>98){
				Sleep(rand()%10);
				if(numTimesSmoked[nextSmokerCombo] == 100){
					nextSmokerCombo = 0+rand()%3;
					continue;
				}
			}
		    //While there are items still on table, sleep
			while(tableEmpty == 0 )Sleep(0);
			//Create a writers lock so the smokers can't decrement items while agent in incrementing them
			WaitForSingleObject(writersLock,INFINITE);
			//Switch statement that will take the random integer selected and figure out what items need to be put out
			switch(nextSmokerCombo){
			case PAPER_MATCH:	item[PAPER-1]++;item[MATCHES-1]++;break;
			case MATCH_TOBACCO: item[MATCHES-1]++;item[TOBACCO-1]++;break;
			case TOBACCO_PAPER: item[TOBACCO-1]++;item[PAPER-1]++;break;
			}
			//Debug code to see items on table and smokers statuses
			//WaitForSingleObject(mutex,INFINITE);
			//cout << "Items on Table: " << onTable[nextSmokerCombo] << endl;
			//cout << "Smoker 0 state: " << states[smokerState[0]] << endl;
			//cout << "Smoker 1 state: "  << states[smokerState[1]] << endl;
			//cout << " Smoker 2 state: " << states[smokerState[2]] << endl;
			//ReleaseMutex(mutex);
			//this lock will change the table from empty to full and will wake up all smokers to let them know items have been placed onto tabl
			WaitForSingleObject(tableStatus,INFINITE);
			tableEmpty = 0;
			ReleaseMutex(tableStatus);

			//exit out of critical section so the smoker with chosen ingredients can enter it
			ReleaseMutex(writersLock);
		}
		//select next combo of smoker items
		nextSmokerCombo = 0+rand()%3;
		//check to see if all smokers have smoked 100 times, once this takes place, the program will end
		if(numTimesSmoked[0]==100 && numTimesSmoked[1]==100 && numTimesSmoked[2]==100){
			return;
		}
	}
}

void smoker(int myID)
{
	while(numTimesSmoked[myID]<100){
	
    smokerState[myID]=CRAVING;
	//Smokers will sleep until the agent tells them that the table is full
    while(tableEmpty == 1)Sleep(0);
	//Smoker will check to see if items are available
	if(item[(myID+5)%3] && item[(myID+1)%3]){
	    //Use lock to create mutual exclusion so smoker can pick up(decrement) there items to be able to smoke
		WaitForSingleObject(writersLock,INFINITE);
		switch(myID){
			case PAPER_MATCH:	item[PAPER-1]--;item[MATCHES-1]--;break;
			case MATCH_TOBACCO: item[MATCHES-1]--;item[TOBACCO-1]--;break;
			case TOBACCO_PAPER: item[TOBACCO-1]--;item[PAPER-1]--;break;
		}
		//This will let the agent know that items have been picked up items from table and that the agent can put on new items
		WaitForSingleObject(tableStatus,INFINITE);
		tableEmpty = 1;
		ReleaseMutex(tableStatus);
		//exit out of critical section
		ReleaseMutex(writersLock);
		numTimesSmoked[myID]++; //increment number of times smoked
		smokerState[myID]=SMOKING;
		Sleep(rand()%10); //smoke for random amount of time
	    //debug code to see what smoke is smoking
		//WaitForSingleObject(mutex,INFINITE);
		//cout << "Smoker " << myID << " is smoking." << endl;
		//ReleaseMutex(mutex);
	}
	else
		Sleep(rand()%10);
	}
	
	smokerState[myID]=DONE;
	
	WaitForSingleObject(mutex,INFINITE);
	cout << "Smoker " << myID <<  " died of lung cancer!" << endl;
	ReleaseMutex(mutex);
}