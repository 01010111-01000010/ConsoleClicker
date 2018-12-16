#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <ncurses.h>
#include <string.h>

//Thread & Mutex
pthread_t async;
pthread_t passiveInput;
pthread_mutex_t pointLock;
pthread_mutex_t quit;

//Shared Resources
bool running = TRUE;

double score = 0.0;

double power = 1.0;
double passive = 0.0;

int powerCost = 100;
int powerLevel = 1;

double miceCost = 15;
double macroCost = 100;
double scriptCost = 1100;
double bufferCost = 12000;
double blockingCost = 130000;

int mice = 0;
int macro = 0;
int script = 0;
int buffer = 0;
int blocking = 0;

void *io(void*);
void *inc(void*);
void saveState(bool pipeFlag);

//Mouse event struct, used to determine what was pressed
MEVENT event;

int main(int argc, char **argv) {
	//Initialise curses environment
	initscr();
	noecho(); //Do not pipe input to stdscr
	nodelay(stdscr, TRUE); //Non blocking I/O allows the game to continue when idle
	curs_set(FALSE); //Stops cursor from ruining my aesthetic
	mousemask(ALL_MOUSE_EVENTS, NULL); 
	keypad(stdscr, TRUE); //Allows mouse events to trigger as a keypad
	mouseinterval(1); //Set mouse event queue timer to 1ms, stops slow I/O & quarantees all clicks work. One of the most important lines & I spent ages looking through curses documentation for this
	
	saveState(1);

	/*
		Multithread all events
		main: main() & Output (Screen) - Refreshes @ 240Hz
		async: asynchronously take input - Refreshes @ 1,000Hz 
		inc: passive score incrementation with own timer @ 100Hz bc people are slow but u can still use macros to cheat ;)
	*/
	pthread_create(&async, NULL, io, NULL);
	pthread_create(&passiveInput, NULL, inc, NULL);
	while(running) {
		//Access shared resources
		pthread_mutex_lock(&pointLock);

		power = powerLevel;

		passive = (mice * 0.1 * powerLevel) + macro + (script * 8) + (buffer * 47) + (blocking * 260);
		
		mvprintw(1, 1, "Clicker Score: %0.1f          ", score);
		mvprintw(2, 1, "Click Power: %0.1f          ", power);
		mvprintw(3, 1, "Passive Clicks: %0.1fcps          ", passive);
		mvprintw(4, 1, "1) Buy Extra Mouse Lvl.%d [0.1cps] (Cost %0.0f)", mice, miceCost);
		mvprintw(5, 1, "2) Buy Click Macro Lvl.%d [1cps] (Cost %0.0f)", macro, macroCost);
		mvprintw(6, 1, "3) Buy Hotkey Script Lvl.%d [8cps] (Cost %0.0f)", script, scriptCost);
		mvprintw(7, 1, "4) Buy I/O Buffer Overflower Lvl.%d [47cps] (Cost %0.0f)", buffer, bufferCost);
		mvprintw(8, 1, "5) Buy Non-blocking I/O Lvl.%d [260cps] (Cost %0.0f)", blocking, blockingCost);
		mvprintw(9, 1, "Z) Overclock Mouse Lvl.%d [2* Click & 1)] (Cost %d)", powerLevel, powerCost);
		
		mvprintw(10, 1, "Press Q to save & quit");

		pthread_mutex_unlock(&pointLock);

		//Refresh rate of 60Hz, its only the console
		usleep(16667);
		refresh();
	}
	pthread_join(async, NULL);
	pthread_join(passiveInput, NULL);
	clear();
	endwin();
	saveState(0);
	return 0;
}

void *io(void* arg) {
	int wait;
	while(running) {
		usleep(10000);
		int ch = getch();
		//Access shared resources
		pthread_mutex_lock(&pointLock);
		if (ch == KEY_MOUSE && getmouse(&event) == OK) { //If the input was a mouse event
			if (event.bstate & BUTTON1_PRESSED) { //If the mouse was pressed down & not released within 1ms (Humanly impossible)
				score += power;
			}
		} else {
			switch(ch) {
				case 122:
					if (score >= powerCost) {
						//Buy click power upgrade
						score -= powerCost;
						powerLevel++;
						powerCost = ((powerCost<<3)+(powerCost<<1));
					}
					break;
				case 49:
					if (score >= (int)miceCost) {
						//Buy passive click upgrade
						score -= (int)miceCost;
						mice++;
						miceCost = miceCost * 1.15;
					}
					break;
				case 50:
					if (score >= (int)macroCost) {
						//Buy passive click upgrade
						score -= (int)macroCost;
						macro++;
						macroCost = macroCost * 1.15;
					}
					break;
				case 51:
					if (score >= (int)scriptCost) {
						//Buy passive click upgrade
						score -= (int)scriptCost;
						script++;
						scriptCost = scriptCost * 1.15;
					}
					break;
				case 52:
					if (score >= (int)bufferCost) {
						//Buy passive click upgrade
						score -= (int)bufferCost;
						buffer++;
						bufferCost = bufferCost * 1.15;
					}
					break;
				case 53:
					if (score >= (int)blockingCost) {
						//Buy passive click upgrade
						score -= (int)blockingCost;
						blocking++;
						blockingCost = blockingCost * 1.15;
					}
					break;
				case 113:
					pthread_mutex_lock(&quit);
					running = FALSE;
					pthread_mutex_unlock(&quit);
					break;
			}
		}
		pthread_mutex_unlock(&pointLock);
	}
}

void *inc(void* arg) {
	while(running) {
		//Wait 1s to repeat
		usleep(1000000);
		//Access shared resources
		pthread_mutex_lock(&pointLock);
		score += passive;
		pthread_mutex_unlock(&pointLock);
	}
}

void saveState(bool pipeFlag) {
	FILE *fp;
	if(pipeFlag) {
		if(access("game.sav", F_OK) == -1) { //If save doesn't exist
			return;
		}
		fp = fopen("game.sav","r");
		/* Standardise File Structure */
		fscanf(fp, "%lf %d %d\n", &score, &powerLevel, &powerCost);
		fscanf(fp, "%d %lf\n", &mice, &miceCost);
		fscanf(fp, "%d %lf\n", &macro, &macroCost);
		fscanf(fp, "%d %lf\n", &script, &scriptCost);
		fscanf(fp, "%d %lf\n", &buffer, &bufferCost);
		fscanf(fp, "%d %lf\n", &blocking, &blockingCost);
	} else {
		fp = fopen("game.sav","w");
		//Prints everything to a file (ENCODE IN FUTURE) (MAYBE SEPERATE IF IT GETS TOO BIG?)
		fprintf(fp, "%0.1f %d %d\n", score, powerLevel, powerCost);
		fprintf(fp, "%d %0.1f\n", mice, miceCost);
		fprintf(fp, "%d %0.1f\n", macro, macroCost);
		fprintf(fp, "%d %0.1f\n", script, scriptCost);
		fprintf(fp, "%d %0.1f\n", buffer, bufferCost);
		fprintf(fp, "%d %0.1f\n", blocking, blockingCost);
	}
	fclose(fp);
	return;
}