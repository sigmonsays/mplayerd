#ifndef HAVE_DBG_H
#define HAVE_DBG_H

#ifdef DEBUG
#define DBG(fmt...) do { \
 printf("DEBUG: %s:%d ", __FILE__, __LINE__); \
 printf(fmt); \
} while (0);

#else
#define DBG(fmt...) do { \
} while (0);
#endif

#endif
