#ifndef HAVE_MPLAYER_SLAVE_H
#define HAVE_MPLAYER_SLAVE_H


int mplayer_osd(struct mp *mpx, int level);
int mplayer_load(struct mp *mpx, char *file, int instance);
int mplayer_command(struct mp *mpx, char *cmd);
int mplayer_seek_percent(struct mp *mpx, char *p);
int mplayer_seek_absolute(struct mp *mpx, char *s);
int mplayer_seek_relative(struct mp *mpx, char *s);
int mplayer_play(struct mp *mpx);
int mplayer_pause(struct mp *mpx);
int mplayer_quit(struct mp *mpx);
int mplayer_volume(struct mp *mpx, int direction);
int mplayer_fullscreen(struct mp *mpx);
int mplayer_mute(struct mp *mpx);
int mplayer_unload(struct mp *mpx);
int mplayer_free(struct mp *mpx);

#endif
