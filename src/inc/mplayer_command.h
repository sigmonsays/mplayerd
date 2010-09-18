#ifndef HAVE_MPLAYER_COMMAND_H
#define HAVE_MPLAYER_COMMAND_H
#include "instance.h"

/* prototypes */

struct mp *mplayer_init(struct mp *mpx);
char *mplayer_status_str(struct mp *mpx);
int mplayer_message(struct mp_instance_t *mp_inst);
int time_to_seconds(char *buf);
void get_mplayer_av_info(char *buf, struct mp *mpx);
#endif
