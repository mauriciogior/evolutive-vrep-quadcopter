#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define MAX_EXT_API_CONNECTIONS 255
#define NON_MATLAB_PARSING 

#include "extApi.c"
#include "extApiPlatform.c"

#define VREP_PORT 19997
#define VREP_MODEL "/Users/mauricio/Desktop/EVOLUTIVOS/scene.ttt"
#define VREP_PATH "/Users/mauricio/Desktop/V-REP_PRO_EDU_V3_2_2_Mac/vrep.app/Contents/MacOS/vrep -h -s0 -q /Users/mauricio/Desktop/EVOLUTIVOS/scene.ttt > logfile"
#define MAX_POPULATION 100
#define MAX_GENERATION 10
#define MAX_MOVIMENTS 200
#define MUTATION 3        // 30%

int clientID = -1;

typedef struct quadcopter {
  double **genes;
  double fitness;
} quadcopter;

void save_quadcopter(quadcopter *);
void run_scene(quadcopter *);
void load_fitness(quadcopter *);
void crossover(quadcopter **);
void save_best(quadcopter **, int);
quadcopter **duplicate_population(quadcopter **);
void free_population(quadcopter **);

int main(int argc, char **argv) {
  
  clientID = simxStart((char*) "127.0.0.1", VREP_PORT, 1, 1, 2000, 5);

  if (clientID == -1) {
    printf("Couldn't estabilish connection with v-rep!\n");
    return 1;
  }

  srand(time(NULL));

  quadcopter **population = (quadcopter **) malloc(MAX_POPULATION * sizeof(quadcopter *));
  
  for (int i=0; i<MAX_POPULATION; i++) {
    population[i] = (quadcopter *) malloc(MAX_POPULATION * sizeof(quadcopter));
    population[i]->fitness = 0;
    population[i]->genes = (double **) malloc(4 * sizeof(double *));

    for (int j=0; j<4; j++) {
      population[i]->genes[j] = (double *) malloc(MAX_MOVIMENTS * sizeof(double));

      for (int k=0; k<MAX_MOVIMENTS; k++) {
        population[i]->genes[j][k] = ((((double) rand()) / RAND_MAX) * 1.5) + 0.4;
      }
    }
  }

  for (int i=0; i<MAX_GENERATION; i++) {
    for (int j=0; j<MAX_POPULATION; j++) {
      printf(": Running scene for gen %d \t quad %d\n", i, j);
      run_scene(population[j]);
    }

    printf(": Saving best of gen %d\n", i);
    save_best(population, i);

    printf(": Crossover on gen %d to next\n", i);
    crossover(population);
  }

  printf(": Finished\n");
  free_population(population);

  simxFinish(clientID);

  return 0;
}

void save_quadcopter(quadcopter *q) {
  FILE *fp = fopen("quadcopter.dat", "w");

  if (fp == NULL) {
    printf("Erro");
    exit(0);
  }

  for (int i=0; i<MAX_MOVIMENTS; i++) {
    for (int j=0; j<4; j++) {
      fprintf(fp, "%lf\n", q->genes[j][i]);
    }
  }

  fclose(fp);
}

void run_scene(quadcopter *q) {
  save_quadcopter(q);

  simxLoadScene(clientID, VREP_MODEL, 0, simx_opmode_oneshot_wait);
  simxStartSimulation(clientID, simx_opmode_oneshot);

  sleep(2);

  simxStopSimulation(clientID, simx_opmode_oneshot);

  load_fitness(q); 
}

void load_fitness(quadcopter *q) {
  FILE *fp = fopen("fitness.dat", "r");

  if (fp == NULL) {
    printf("Erro");
    exit(0);
  }

  fscanf(fp, "%lf", &q->fitness);

  printf(": fitness %lf\n", q->fitness);

  fclose(fp);
}

void save_best(quadcopter **q, int generation) {

  for (int i=0; i<MAX_POPULATION; i++) {
    for (int j=i+1; j<MAX_POPULATION; j++) {
      if (q[i]->fitness < q[j]->fitness) {
        double fitness = q[i]->fitness;
        q[i]->fitness = q[j]->fitness;
        q[j]->fitness = fitness;

        double **genes = q[i]->genes;

        q[i]->genes = q[j]->genes;
        q[j]->genes = genes;
      }
    }
  }

  char filename[20];

  sprintf(filename, "best-gen-%d.dat", generation);

  FILE *fp = fopen(filename, "w");

  if (fp == NULL) {
    printf("Erro");
    exit(0);
  }

  for (int i=0; i<MAX_MOVIMENTS; i++) {
    for (int j=0; j<4; j++) {
      fprintf(fp, "%lf\n", q[0]->genes[j][i]);
    }
  }

  fclose(fp);

}

void crossover(quadcopter **q) {
  quadcopter **_q = duplicate_population(q);

  for (int i=0; i<MAX_POPULATION; i++) {
    
    q[i]->fitness = 0;
      
    int dad = rand() % (MAX_POPULATION / 10);
    int mom = rand() % (MAX_POPULATION / 10);

    while (mom == dad) {
      mom = rand() % (MAX_POPULATION / 10);
    }

    for (int j=0; j<4; j++) {
      for (int k=0; k<MAX_MOVIMENTS; k++) {
        if (rand() % 10 < MUTATION) {
          q[i]->genes[j][k] = ((((double) rand()) / RAND_MAX) * 1.5) + 0.5;
        } else {
          if (k < (rand() % (MAX_MOVIMENTS - 20)) + 20) {
            q[i]->genes[j][k] = _q[dad]->genes[j][k];
          } else {
            q[i]->genes[j][k] = _q[mom]->genes[j][k];
          }
        }
      }
    }

  }

  free_population(_q);
}

quadcopter **duplicate_population(quadcopter **population) {

  quadcopter **cp = (quadcopter **) malloc(MAX_POPULATION * sizeof(quadcopter *));

  for (int i=0; i<MAX_POPULATION; i++) {
    cp[i] = (quadcopter *) malloc(MAX_POPULATION * sizeof(quadcopter));
    cp[i]->fitness = population[i]->fitness;
    cp[i]->genes = (double **) malloc(4 * sizeof(double *));

    for (int j=0; j<4; j++) {
      cp[i]->genes[j] = (double *) malloc(MAX_MOVIMENTS * sizeof(double));

      for (int k=0; k<MAX_MOVIMENTS; k++) {
        cp[i]->genes[j][k] = population[i]->genes[j][k];
      }
    }
  }

  return cp;

}

void free_population(quadcopter **population) {

  for (int i=0; i<MAX_POPULATION; i++) {
    for (int j=0; j<4; j++) {
      free(population[i]->genes[j]);
    }

    free(population[i]->genes);
  }

  free(population);

}
