static void * nop_benchmark(void * args) {
  asm volatile("top:    ; \
                jmp top ");
  return NULL;
}
...
for (thread = 0; thread < cores; ++thread) {
  pthread_create(&thread_handles[thread], NULL,
                 &nop_benchmark,
                 (void *) NULL);
}
