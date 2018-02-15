#ifndef __ARPC_H_
#define __ARPC_H_

void* arpc_emit_call(int dest, int code, const void* input);
void* arpc_recv_call(int dest, int code, const void* input);

#endif /* ifndef __ARPC_H_ */
