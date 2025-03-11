#include "emscripten.h"
#include "greshunkel.h"

EMSCRIPTEN_KEEPALIVE
int init() {
  greshunkel_ctext *test = gshkl_init_context();
  gshkl_free_context(test);

  return 1;
}

