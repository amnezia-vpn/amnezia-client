#ifndef WINHELP_H
#define WINHELP_H

#include <QDebug>

#ifdef __cplusplus
extern "C" {
#endif

extern int winhelpLaunchStartup(const char *, int, const char *);
extern int winhelpOneProcess();
extern int winhelpSystemBits();
extern bool winhelpIsSystem_x64();
extern int winhelpProcessQuery(void **);
extern int winhelpProcessNext(void *, char *);
extern int winhelpRecvEvent(const char *);
extern int winhelpSendEvent(const char *);
extern int winhelpSystemVersion();
extern int winhelperSetMTUSize(const char *, int);
extern int winhelpRecvSendBytes(const char *, int *, int *);

#ifdef __cplusplus
}
#endif

#endif /* WINHELP_H */
