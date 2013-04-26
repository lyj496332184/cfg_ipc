#ifndef _CFG_COMMON_H_
#define _CFG_COMMON_H_

#define SERVER_IPC_DOMAIN_NAME	"main.socket"

#ifdef CFG_DEBUG
#define DEBUG_NOR(f, a...)	printf("%s %s %d:" f,  __FILE__, __FUNCTION__, __LINE__ , ## a);
#else
#define DEBUG_NOR(f, a...)
#endif

#define DEBUG_ERR(f, a...)	fprintf(stderr, "%s %s %d:" f, __FILE__, __FUNCTION__, __LINE__, ##a);

#endif