#include "orchard-ui.h"
#include <string.h>

orchard_ui_start();
orchard_ui_end();

// mutex to lock the graphics subsystem for safe multi-threaded drawing
mutex_t orchard_gfxMutex;

const OrchardUi *getUiByName(const char *name) {
  const OrchardUi *current;

  current = orchard_uis();
  while(current->name) {
    if( !strncmp(name, current->name, 16) ) {
      return current;
    }
    current++;
  }
  return NULL;
}

void uiStart(void) {
  osalMutexObjectInit(&orchard_gfxMutex);
}

void orchardGfxStart(void) {

  osalMutexLock(&orchard_gfxMutex);
  
}

void orchardGfxEnd(void) {
  
  osalMutexUnlock(&orchard_gfxMutex);
  
}

void noRender(GWidgetObject* gw, void* param)
{
        (void)gw;
        (void)param;

        return;
}

