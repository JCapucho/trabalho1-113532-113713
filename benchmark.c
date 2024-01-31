#include <stdio.h>

#include "image8bit.h"
#include "instrumentation.h"

int main(int ac, char *av[]) {
  ImageInit();

  Image img = ImageCreate(300, 300, 255);

  InstrReset();

  ImageBlur(img, 7, 7);

  InstrPrint();

  ImageDestroy(&img);

  return 0;
}
