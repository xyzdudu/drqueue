/* $Id: communications.h,v 1.17 2002/06/26 13:33:55 jorge Exp $ */

#ifndef _COMMUNICATIONS_H_
#define _COMMUNICATIONS_H_

#include "request.h"
#include "job.h"
#include "database.h"

#ifdef COMM_REPORT
extern long int bsent;		/* Bytes sent */
extern long int brecv;		/* Bytes received */
#endif


int get_socket (short port);
int accept_socket (int sfd,struct database *wdb,struct sockaddr_in *addr);
int accept_socket_slave (int sfd);
int connect_to_master (void);
int connect_to_slave (char *slave);

int recv_request (int sfd, struct request *request);
int send_request (int sfd, struct request *request,int who);
int send_computer (int sfd, struct computer *computer);
int recv_computer (int sfd, struct computer *computer);
int recv_computer_hwinfo (int sfd, struct computer_hwinfo *hwinfo);
int send_computer_hwinfo (int sfd, struct computer_hwinfo *hwinfo);
int recv_computer_status (int sfd, struct computer_status *status);
int send_computer_status (int sfd, struct computer_status *status);
int recv_computer_limits (int sfd, struct computer_limits *cl);
int send_computer_limits (int sfd, struct computer_limits *cl);
int recv_job (int sfd, struct job *job);
int send_job (int sfd, struct job *job);
int recv_task (int sfd, struct task *task);
int send_task (int sfd, struct task *task);
int recv_frame_info (int sfd, struct frame_info *fi);
int send_frame_info (int sfd, struct frame_info *fi);
int recv_autoenable (int sfd, struct autoenable *ae);
int send_autoenable (int sfd, struct autoenable *ae);

int write_32b (int sfd, void *data);
int write_16b (int sfd, void *data);
int read_32b (int sfd, void *data);
int read_16b (int sfd, void *data);

int dr_read (int fd, void *buf, uint32_t len);
int dr_write (int fd, void *buf, uint32_t len);


#endif /* _COMMUNICATIONS_H_ */