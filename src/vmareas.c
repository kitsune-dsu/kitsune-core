#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#include <interval.h>

#include "kitsune_internal.h"
#include "vmareas_internal.h"

enum { READ_ATTR = 1, WRITE_ATTR = 2, EXEC_ATTR = 4 };
typedef unsigned char attr_type;

typedef struct mem_area {
  void *start;
  void *end;
  attr_type attr;
  vmarea_type type;
  char *label;
} mem_area;

char *vmareas_to_str(vmarea *a) {
  mem_area *ma = (mem_area *)a;
  char *r = ma->attr & READ_ATTR ? "r" : "";
  char *w = ma->attr & WRITE_ATTR ? "w" : "";
  char *x = ma->attr & EXEC_ATTR ? "x" : "";
  char *result;
  asprintf(&result, "%s [%s%s%s]", ma->label, r, w, x);
  return result;
}
int vmareas_get_readable(vmarea *a) {
  return ((mem_area *)a)->attr & READ_ATTR;
}
int vmareas_get_writable(vmarea *a) {
  return ((mem_area *)a)->attr & WRITE_ATTR;
}
int vmareas_get_executable(vmarea *a) {
  return ((mem_area *)a)->attr & EXEC_ATTR;
}
char *vmareas_get_label(vmarea *a) {
  return ((mem_area *)a)->label;
}
vmarea_type vmareas_get_type(vmarea *a) {
  return ((mem_area *)a)->type;
}

static void *area_start(void *r) {
  return ((mem_area *)r)->start;
}
static void *area_end(void *r) {
  return ((mem_area *)r)->end;
}
static int addr_compare(void *p0, void *p1)
{
  /* Strictly speaking, we shouldn't be comparing pointers that might point to
     arbitary addresses (e.g., not in the same array). :) */
  ptrdiff_t diff = p0 - p1;
  if (diff == 0)
    return 0;
  else if (diff > 0)
    return 1;
  else
    return -1;
}

interval_tree memory_areas;

static void vmareas_add(void *start_addr, void *end_addr, unsigned char attr, char *label)
{
  mem_area *new_area = malloc(sizeof(mem_area));

  if (strcmp("[heap]", label) == 0) {
    new_area->type = HEAP;  
  } else if (strcmp("[stack]", label) == 0) {
    new_area->type = STACK;
  } else if (strstr(".so", label) != NULL) {
    new_area->type = LIBRARY;
  } else {
    new_area->type = OTHER;
  }

  new_area->attr = attr;
  new_area->start = start_addr;
  new_area->end = end_addr;
  new_area->label = label;
  interval_tree_insert(&memory_areas, new_area);
}


vmarea *vmareas_lookup(void *addr) 
{  
  mem_area query;
  query.start = query.end = addr;
  return (vmarea *)interval_tree_lookup(&memory_areas, &query);
}

static size_t parse_addr(char *buf, size_t pos, size_t max, unsigned long *addrp) {
  unsigned long result = 0;
  size_t p = pos;
  while (p < max) {
    char c = buf[p];
    if (c >= '0' && c <= '9')
      result = (result << 4) + (c - '0');
    else if (c >= 'a' && c <= 'f')
      result = (result << 4) + 10 + (c - 'a');
    else
      break;
    p++;
  }
  kitsune_assert(pos != p, "Failed to parse vm area line address.");
  *addrp = result;
  return p;
}

static size_t skip_chars(char *buf, size_t pos, size_t max, int skip_spaces) {

  while (pos < max && 
         ((skip_spaces && buf[pos] == ' ') ||
          (!skip_spaces && buf[pos] != ' ')))
    pos++;
  return pos;
}

void vmareas_parse(void) 
{
  FILE *fp = fopen("/proc/self/maps", "r");
  kitsune_assert(fp != NULL, "Unable to open: /proc/self/maps");

  char line[LINE_MAX];
  while (fgets(line, LINE_MAX, fp) != NULL) {
    unsigned long start_addr, end_addr;
    attr_type attr = 0;
    size_t pos = 0;
    size_t len = strlen(line);
    line[len-1] = 0;

    char *label;

    pos = parse_addr(line, pos, len, &start_addr);
    assert(line[pos] == '-');
    pos++;
    pos = parse_addr(line, pos, len, &end_addr);
    assert(line[pos] == ' ');
    pos++;
    assert(pos+4 <= len);
    if (line[pos++] == 'r') attr |= READ_ATTR; 
    if (line[pos++] == 'w') attr |= WRITE_ATTR;
    if (line[pos++] == 'x') attr |= EXEC_ATTR;
    pos += 2;
    pos = skip_chars(line, pos, len, 0);
    pos++;
    pos = skip_chars(line, pos, len, 0);
    pos++;
    pos = skip_chars(line, pos, len, 0);
    pos = skip_chars(line, pos, len, 1);
    label = &line[pos];
    vmareas_add((void *)start_addr, (void *)end_addr, attr, strdup(label)); 
  }
  
  fclose(fp);
}

/* clear should be called once we reach the target update point */
void vmareas_free(void)
{
  interval_tree_free(&memory_areas);
}

/* init should be called during startup */
void vmareas_init(void)
{
  interval_tree_init(&memory_areas, area_start, area_end, addr_compare);
  vmareas_parse();
}
