/* $Id: job.c,v 1.12 2001/07/31 13:10:12 jorge Exp $ */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "job.h"
#include "database.h"
#include "constants.h"
#include "slave.h"
#include "logger.h"
#include "semaphores.h"

int job_index_free (void *pwdb)
{
  /* Return the index to a free job record OR -1 if there */
  /* are no more free records */
  int index = -1;
  int i;
  struct database *wdb = (struct database *)pwdb;

  for (i=0; i<MAXJOBS; i++) {
    if (wdb->job[i].used == 0) {
      index = i;
      break;
    }
  }

  return index;
}

void job_report (struct job *job)
{
  printf ("JOB Report\n");
  printf ("Name:\t\t\t%s\n",job->name);
  printf ("Status:\t\t\t%s\n",job_status_string(job->status));
  printf ("Owner:\t\t\t%s\n",job->owner);
  printf ("Command:\t\t%s\n",job->cmd);
  printf ("Frame start,end:\t%i,%i\n",job->frame_start,job->frame_end);
}

void job_init_registered (struct database *wdb,uint32_t ijob,struct job *job)
{
  /* Called we we have just received a job to be registered */
  int i;
  int nframes;
  char msg[BUFFERLEN];
  
  semaphore_lock(wdb->semid);

  memcpy (&wdb->job[ijob], job, sizeof(struct job));
  wdb->job[ijob].used = 1;
  wdb->job[ijob].id = ijob;	/* Check ranges !!! */
  wdb->job[ijob].status = JOBSTATUS_WAITING;
  /* We allocate the memory for the frame_info */
  nframes = job_nframes (&wdb->job[ijob]);
  fprintf (stderr,"nframes: %i\n",nframes);

  wdb->job[ijob].fishmid = get_frame_shared_memory (nframes);
  wdb->job[ijob].frame_info = attach_frame_shared_memory (wdb->job[ijob].fishmid);

  /* Set done frames to NONE */
  for (i=0;i<nframes;i++) {
    wdb->job[ijob].frame_info[i].status = FS_WAITING;
  }

  wdb->job[ijob].fleft = nframes;
  wdb->job[ijob].fdone = 0;
  wdb->job[ijob].ffailed = 0;

  wdb->job[ijob].nprocs = 0;

  wdb->job[ijob].avg_frame_time = DFLTAVGFTIME;
  wdb->job[ijob].est_finish_time = time (NULL) + (DFLTAVGFTIME * nframes);

  detach_frame_shared_memory(wdb->job[ijob].frame_info);

  semaphore_release(wdb->semid);

  snprintf(msg,BUFFERLEN,"Registered on position %i",ijob);
  log_master_job (&wdb->job[ijob],L_INFO,msg);
}

void job_init (struct job *job)
{
  job->used = 0;
  job->frame_info = NULL;
  job->fishmid = -1;		/* -1 when not reserved */
}

void job_delete (struct job *job)
{
  job->used = 0;

  if (job->fishmid != -1) {
    if (shmctl (job->fishmid,IPC_RMID,NULL) == -1) {
      log_master_job(job,L_ERROR,"shmctl (job->fishmid,IPC_RMID,NULL)");
    }
    job->fishmid = -1;
  }

  job->frame_info = NULL;
}

char *job_status_string (char status)
{
  static char sstring[BUFFERLEN];

  sstring[MAXCMDLEN-1] = 0;
  switch (status) {
  case JOBSTATUS_WAITING:
    strncpy (sstring,"Waiting",BUFFERLEN-1);
    break;
  case JOBSTATUS_ACTIVE:
    strncpy (sstring,"Active",BUFFERLEN-1);
    break;
  case JOBSTATUS_STOPPED:
    strncpy (sstring,"Stopped",BUFFERLEN-1);
    break;
  case JOBSTATUS_HSTOPPED:
    strncpy (sstring,"Hard Stopped",BUFFERLEN-1);
    break;
  case JOBSTATUS_DELETING:
    strncpy (sstring,"Deleting",BUFFERLEN-1);
    break;
  case JOBSTATUS_FINISHED:
    strncpy (sstring,"Finished",BUFFERLEN-1);
    break;
  default:
    strncpy (sstring,"DEFAULT (?!)",BUFFERLEN-1);
    fprintf (stderr,"job_status == DEFAULT\n");
    exit (1);
  }

  return sstring;
}

int job_nframes (struct job *job)
{
  int n;

  n = job->frame_start - job->frame_end;
  n = (n<0) ? -n : n;
  n++;

  return n;
}


int job_available (struct database *wdb,uint32_t ijob, int *iframe)
{
  if (wdb->job[ijob].used == 0)
    return 0;

  if ((*iframe = job_first_frame_available (wdb,ijob)) == -1)
    return 0;

  if ((wdb->job[ijob].status == JOBSTATUS_WAITING) || (wdb->job[ijob].status == JOBSTATUS_ACTIVE))
    return 1;

  return 0;
}

int job_first_frame_available (struct database *wdb,uint32_t ijob)
{
  /* This function not only returns the first frame */
  /* This function is called non blocked */
  /* available but also updates the job structure when found */
  /* so the frame status goes to assigned (we still have to */
  /* set the info about the icomp,start,itask */
  int i;
  int r = -1;
  int nframes = job_nframes (&wdb->job[ijob]);
  struct frame_info *fi;

  semaphore_lock(wdb->semid);
  fi = attach_frame_shared_memory(wdb->job[ijob].fishmid);
  for (i=0;i<nframes;i++) {
    if (fi[i].status == FS_WAITING) {
      r = i;			/* return = current */
      fi[i].status = FS_ASSIGNED; /* Change the status to assigned */
      break;
    }
  }
  detach_frame_shared_memory(fi);
  semaphore_release(wdb->semid);

  return r;
}

void job_update_assigned (struct database *wdb, uint32_t ijob, int iframe, int icomp, int itask)
{
  /* LOCK BEFORE CALLING THIS FUNCTION */
  /* Here we should set all the information inside the task structure (slave) */
  /* about the assigned job (master) into the remote computer */
  /* This function is called by the master, locked */
  wdb->job[ijob].frame_info = attach_frame_shared_memory (wdb->job[ijob].fishmid);

  /* The status should already be FS_ASSIGNED */
  if (wdb->job[ijob].frame_info[iframe].status != FS_ASSIGNED) {
    fprintf (stderr,"(wdb->job[ijob].frame_info[iframe].status != FS_ASSIGNED)\n");
    fprintf (stderr,"%s:%i\n",__FILE__,__LINE__);
    wdb->job[ijob].frame_info[iframe].status = FS_ASSIGNED;
  }

  wdb->job[ijob].frame_info[iframe].icomp = icomp;
  wdb->job[ijob].frame_info[iframe].itask = itask;

  /* Time stuff */
  time (&wdb->job[ijob].frame_info[iframe].start_time);
  wdb->job[ijob].frame_info[iframe].end_time = wdb->job[ijob].frame_info[iframe].start_time
    + wdb->job[ijob].avg_frame_time;

  /* Exit code */
  wdb->job[ijob].frame_info[iframe].exitcode = 0;

  detach_frame_shared_memory(wdb->job[ijob].frame_info);
}

int get_frame_shared_memory (int nframes)
{
  int shmid;

  if ((shmid = shmget (IPC_PRIVATE,sizeof(struct frame_info)*nframes, IPC_EXCL|IPC_CREAT|0600)) == -1) {
    log_master (L_ERROR,"get_frame_shared_memory: shmget");
    exit (1);
  }

  return shmid;
}

void *attach_frame_shared_memory (int shmid)
{
  void *rv;			/* return value */

  if ((rv = shmat (shmid,0,0)) == (void *)-1) {
    log_master (L_ERROR,"attach_frame_shared_memory: shmat");
    perror ("shmat");
    exit (1);
  }

  return rv;
}

void detach_frame_shared_memory (struct frame_info *fishp)
{
  if (shmdt((char*)fishp) == -1) {
    log_master (L_WARNING,"Call to shmdt failed");
  }
}

int job_njobs_masterdb (struct database *wdb)
{
  int i,c=0;

  for (i=0;i<MAXJOBS;i++) {
    if (wdb->job[i].used) {
      c++;
    }
  }

  return c;
}

void job_update_info (struct database *wdb,uint32_t ijob)
{
  /* This function is called by the master */
  /* It updates the number of process running */
  /* This function is called unlocked */
  int i,nprocs=0;
  struct frame_info *fi;
  int fleft=0,fdone=0,ffailed=0;
  int total;
  if (ijob > MAXJOBS)
    return;

  total = job_nframes(&wdb->job[ijob]);

  fi = attach_frame_shared_memory (wdb->job[ijob].fishmid);
  wdb->job[ijob].frame_info = fi;
  for (i=0;i<total;i++) {
    job_check_frame_status (wdb,ijob,i);
    switch (fi[i].status) {
    case FS_ASSIGNED:
      nprocs++;
      break;
    case FS_WAITING:
      fleft++;
      break;
    case FS_FINISHED:
      fdone++;
      break;
    case FS_ERROR:
      ffailed++;
      break;
    }
  }
  detach_frame_shared_memory(fi);

  semaphore_lock(wdb->semid);
  wdb->job[ijob].nprocs = nprocs;
  wdb->job[ijob].fleft = fleft;
  wdb->job[ijob].fdone = fdone;
  wdb->job[ijob].ffailed = ffailed;
  switch (wdb->job[ijob].status) {
  case JOBSTATUS_WAITING:
  case JOBSTATUS_ACTIVE:
    if (nprocs > 0) {
      wdb->job[ijob].status = JOBSTATUS_ACTIVE;
    } else {
      if (fleft == 0) {
	wdb->job[ijob].status = JOBSTATUS_FINISHED;
      } else {
	wdb->job[ijob].status = JOBSTATUS_WAITING;
      }
    }
    break;
  case JOBSTATUS_DELETING:
  case JOBSTATUS_HSTOPPED:
  case JOBSTATUS_STOPPED:
  }
  semaphore_release(wdb->semid);
}

void job_check_frame_status (struct database *wdb,uint32_t ijob, uint32_t iframe)
{
  /* This function is called by the master, unlocked */
  /* This function check if the running or loading (in frame_info at job) process is actually */
  /* runnning or not (in task at computer) */
  /* This function is called with the frame info memory ATTACHED <------- */
  t_framestatus fistatus;
  int running = 1;
  uint16_t icomp,itask;
  t_taskstatus tstatus;

  semaphore_lock(wdb->semid);
  
  fistatus = wdb->job[ijob].frame_info[iframe].status;
  icomp = wdb->job[ijob].frame_info[iframe].icomp;
  itask = wdb->job[ijob].frame_info[iframe].itask;

  tstatus = wdb->computer[icomp].status.task[itask].status;

  if (fistatus == FS_ASSIGNED) {
    /* check if the task status is running */
    if ((tstatus != TASKSTATUS_RUNNING) && (tstatus != TASKSTATUS_LOADING))
      running = 0;
    /* check if the job is the same in index */
    if (wdb->computer[icomp].status.task[itask].jobindex != ijob)
      running = 0;
    /* check if the job is the same in name */
    if (strcmp (wdb->computer[icomp].status.task[itask].jobname,wdb->job[ijob].name) != 0)
      running = 0;
    if (!running) {
      log_master_job (&wdb->job[ijob],L_WARNING,"Task registered as running not running. Requeued");
      wdb->job[ijob].frame_info[iframe].status = FS_WAITING;
    }
  }

  semaphore_release(wdb->semid);

}

int priority_job_compare (const void *a,const void *b)
{
  struct tpol *apt,*bpt;
  
  apt = (struct tpol *)a;
  bpt = (struct tpol *)b;

  if (apt->pri > bpt->pri)
    return 1;
  else if (apt->pri < bpt->pri)
    return -1;
  else if (apt->index > bpt->index)
    return 1;
  else if (apt->index < bpt->index)
    return -1;

  return 0;
}
