/* $Id: request_codes.h,v 1.20 2001/10/24 14:52:52 jorge Exp $ */

#ifndef _REQUEST_CODES_H_
#define _REQUEST_CODES_H_

#define R_R_REGISTER   1	/* Request register computer */
#define R_R_UCSTATUS   2	/* Request to update computer status */
#define R_R_REGISJOB   3	/* Register a new job */
#define R_R_AVAILJOB   4	/* Request available job */
#define R_R_TASKFINI   5	/* Request task finished, the slave send this when finishes a task */
#define R_R_LISTJOBS   6	/* Request a list of current jobs in the queue */
#define R_R_LISTCOMP   7	/* Request a list of current registered computers */
#define R_R_DELETJOB   8	/* Request a job to be deleted */
#define R_R_STOPJOB    9	/* Stop a job */
#define R_R_CONTJOB    10	/* Continue a stopped job */
#define R_R_HSTOPJOB   11	/* Hard stop a job */
#define R_R_JOBXFER    12	/* Job info transfer by index (identification) */
#define R_R_JOBXFERFI  13	/* Job frame info transfer by index (identification) */
#define R_R_COMPXFER   14	/* Computer info transfer by index (identification) */
#define R_R_JOBFWAIT   15	/* Job frame waiting (only non-running frames) */
#define R_R_JOBFKILL   16	/* Job frame kill (kills only [it's later set to waiting because of the usual handling]) */
#define R_R_JOBFFINI   17	/* Job frame finished (only waiting frames) */
#define R_R_JOBFKFIN   18	/* Job frame kill and finished (only running frames) */
#define R_R_UCLIMITS   19	/* Update computer limits */
#define R_R_SLAVEXIT   20	/* Slave exit, one slave must be removed from the queue */
#define R_R_JOBSESUP   21	/* SES (Start,End,Step frames) update */
#define R_R_JOBLNMCS   22	/* Job limit nmaxcpus set */
#define R_R_JOBLNMCCS  23	/* Job limit nmaxcpuscomputer set */
#define R_R_JOBPRIUP   24	/* Job priority update */


/* Requests to the slave */
#define RS_R_KILLTASK           1 /* Request to slave to kill a task */
#define RS_R_SETNMAXCPUS        2 /* Request to set limits nmaxcpus */
#define RS_R_SETMAXFREELOADCPU  3 /* Request to set limits maxfreeloadcpu */

#endif /* _REQUEST_CODES_H_ */