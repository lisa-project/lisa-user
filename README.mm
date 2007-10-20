FIXME: move this somewhere else

Good mm practices:
* use mm_create() durig the init phase of your code and make it globally
  accessible (i.e. global variable) so you can easily call mm_
  primitives;
* always lock the mm handle before using (even reading) mm shared data;
  otherwise things can change while you look at them;
* mm_lock() also ensures that the whole shm area is mapped into your
  process virtual memory (the shm area can be enlarged by other
  processes between the time you open it with mm_create() and access it);
* if mm_alloc is involved in your critical region, never rely on process
  addresses (pointers) and always use mm pointers; although you access
  the shm data from within a critical region (with the mm handle locked),
  mm_alloc can extend the shm area and thus remap the shm area in your
  process virtual memory; old pointer values will then point to
  unallocated pages
