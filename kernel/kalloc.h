

struct ref_count {
  struct spinlock lock;
  int number[PGROUNDUP(PHYSTOP) / PGSIZE];
};