#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "pthread_sleep.h"


/* Procédure qui bloque le thread appelant pour delai exprimé en secondes.
 * Cette procédure est nécessaire car on ne peut utiliser l'appel système
 * sleep() avec des threads.  Pour suspendre momentanément un thread, on
 * utilise pthread_cond_timedwait() qui permet de bloquer un thread sur une
 * condition pour un temps limité
 */

#define MILLION 1000000

void pthread_sleep (double delai)
{
  assert(delai<10000.0);
  pthread_cond_t cw;          // condition "privée" utilisée par le thread qui veut dormir
  pthread_mutex_t mx;         // mutex nécessaire pour pouvoir utiliser pthread_cond_timedwait
  struct timeval tps_deb;     // heure de debut du blocage
  struct timespec tps_exp;    // heure d'expiration du blocage

  pthread_cond_init (&cw, NULL);
  pthread_mutex_init (&mx, NULL);

  if (pthread_mutex_lock(&mx)!=0)
    { perror("Erreur mutex_lock "); exit(errno); }
  if(gettimeofday (&tps_deb, 0)==-1)	               // on récupère l'heure courante
    { perror("Erreur gettimeofday "); exit(errno); }
  int usec = (int)tps_deb.tv_usec + (int)(delai*MILLION);
  int sec = usec/MILLION;
  usec = usec%MILLION;
  tps_exp.tv_sec = tps_deb.tv_sec+sec ;        // on rajoute delai secondes
  tps_exp.tv_nsec = 1000*usec;                 // on convertit les µs en ns
  int res = pthread_cond_timedwait (&cw, &mx, &tps_exp); // et on se bloque jusqu'au temps calculé
  if (res!=0 && res!=ETIMEDOUT)
    { perror("Erreur cond_timedwait "); exit(errno); }
  if (pthread_mutex_unlock(&mx)!=0)
    { perror("Erreur mutex_unlock "); exit(errno); }
  if (pthread_cond_destroy (&cw)!=0)
    { perror("Erreur cond_destroy "); exit(errno); }
  if (pthread_mutex_destroy (&mx)!=0)
    { perror("Erreur mutex_destroy "); exit(errno); }
}
