// Liam Howes: 5880331
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <cstdlib>
#include <pthread.h>
#include <cmath>

using namespace std;
volatile bool keep_running;
volatile int occupied;
pthread_mutex_t lock; // mutual exclusion lock

//Formula: f(x,y)= −( y+47)sin(√|x /2+ y+47|)−x sin(√|x−y−47|)
//Domain: -512 ≤ x, y ≤ +512

volatile double best_so_far = 10000; //best minimum found so far (initialized to very high value)
volatile double best_x, best_y; //coordiantes that yielded best minimum so far

void sig_function(int sig){ // ctrl+c to stop finding nonce/hash pairs and return to menu
    cout<<"\nReturning to menu...\n\n";
    keep_running = false;
}

void peek(int sig){ // peekl at current best
    cout<<"\nglobal best(minimum height): "<<best_so_far<<endl;;
    cout<<"x = "<<best_x<<", y = "<<best_y<<endl<<endl;;
}

void* hill_climber(void* blahblah){
    double current_height;
    double x, y;
    double mod_x, mod_y;
    x = rand() % 1025+(-512); // random x coordinate (ints cast to doubles)
    y = rand() % 1025+(-512); // random y coordinate 
    while(keep_running){
        bool updated = false; // check if any of the 4 steps actually yielded a better result
        bool find_valid_mods = false;
        double lower_bound = -5, upper_bound = 5;
        for(int i=0; i<4; i++){ // get 4 differnt possible steps
            while(!find_valid_mods){ // find valid x and y modifiers
                // random modifiers //
                mod_x = (upper_bound-(lower_bound))*((double)rand()/(double)RAND_MAX) + (lower_bound); // random step between -5 and 5
                mod_y = (upper_bound-(lower_bound))*((double)rand()/(double)RAND_MAX) + (lower_bound);
                if(x+mod_x >= -512 && x+mod_x<= 512){ //if in bounds
                    find_valid_mods = true; //found them
                    double temp_x = x + mod_x;
                    double temp_y = y + mod_y;
                    // calculate what the step would yield
                    double next_step = -(temp_y+47)*sin(sqrt(abs((temp_x/2)+temp_y+47))) - temp_x*sin(sqrt(abs(temp_x-temp_y-47)));
                    if(next_step<current_height){
                        // update climber's current_height and corresponding x and y coordinates
                        current_height = next_step;
                        x = temp_x;
                        y = temp_y;
                        updated = true;
                        if(current_height < best_so_far){ // check if this local minimum is the new global minimum
                            pthread_mutex_lock(&lock); // lock so threads don't write at the same time
                            best_so_far = current_height;
                            best_x = x;
                            best_y = y;
                            occupied--;
                            pthread_mutex_unlock(&lock); // release lock
                        }
                    }
                }
                else{ // NOT in bounds
                    if(x+mod_x < -512){ // x is less than lower bound
                        double difference =  (x+mod_x + 512)*(-1); //find the amount that it was out of range
                        lower_bound = lower_bound + difference; // clamp lower bound
                    }
                    if(y+mod_y < -512){ // y is less than lower bound
                        double difference =  (y+mod_y + 512)*(-1); //find the amount that it was out of range
                        lower_bound = lower_bound + difference; // clamp lower bound)
                    }
                    // and do the same, but for the upper bounds of x and y mods
                    if(x+mod_x > 512){
                        double difference = (x+mod_x - 512);
                        upper_bound = upper_bound - difference;
                    }
                    if(y+mod_y > 512){
                        double difference = (y+mod_y - 512);
                        upper_bound = upper_bound - difference;
                    }
                }
            } // end of while loop
        }
        // if none of the generated steps improved the climber's position, get new random position
        if(!updated){
            x = rand() % 1025+(-512);
            y = rand() % 1025+(-512);
        }
    }
}

int main(){
    bool main_running = true;
    while(main_running){
        srand(time(NULL)); // random seed
        int num_climbers;
        cout<<"'ctrl+c' to stop and return to menu, 'ctrl+z' to peek at current best\n";
        cout<<"How many climbers (threads) to run simultaneously (MAX of 8, 0=exit)? ";
        cin>>num_climbers;
        if(num_climbers==0 || num_climbers > 8){
            main_running = false; // exit
        }
        else{
            pthread_t ct[num_climbers]; // threads
            if(signal(SIGINT, sig_function)==SIG_ERR){ //keep running until SIGINT (ctrl+c)
                cout<<"Unable to change signal handler."<<endl;
                return 1;
            }
            if(signal(SIGTSTP, peek)==SIG_ERR){
                cout<<"Unable to change signal handler."<<endl;
                return 1;
            }
            keep_running = true;
            for(int i=0; i<num_climbers; i++){
                pthread_mutex_lock(&lock); // reserve lock
                pthread_create(&ct[i], NULL, &hill_climber, NULL); // create threads
                occupied++;
                pthread_mutex_unlock(&lock); //realease lock
            }
            while(keep_running){ // keep er going as long as the threads ar running
                // sleep to improve responsiveness by forfeiting time slice
                while(occupied>0){
                    sleep(1);
                }  
            }
            // once they're cancelled with ctrl+c, print the best results
            cout<<"global best(minimum height): "<<best_so_far<<endl;;
            cout<<"x = "<<best_x<<", y = "<<best_y<<endl<<endl;;
        }
    }
    return 0;
}