
void int_to_long(void *vin, void *vout, int _ignored, void **ignored) {
  int *in = vin;
  long *out = vout;
  *out = *in;  
}
