#ifndef THREAD_H
#define THREAD_H

void *master_thread_func(void *cls); // master thread
void *net_thread_func(void *cls); // net thread
void *unit_thread_func(void *cls); // thread pool unit

#endif
