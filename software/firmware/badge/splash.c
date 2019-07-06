#include "ch.h"
#include "hal.h"

#include "string.h"
#include "rand.h"

#include "gfx.h"
#include "images.h"
#include "buildtime.h"
#include "fontlist.h"

void splash_footer(void) {
  font_t font;

  font = gdispOpenFont (FONT_SYS);
  gwinSetDefaultFont (font);

  gdispDrawStringBox (0, 210, gdispGetWidth(),
		      gdispGetFontMetric(font, fontHeight),
		      "DA BOMB! DC 27 (2019)",
		      font, Red, justifyCenter);

  gdispDrawStringBox (0, 225, gdispGetWidth(),
		      gdispGetFontMetric(font, fontHeight),
		      BUILDTIME,
		      font, White, justifyCenter);

  gdispCloseFont (font);

}

void splash_SDFail(void) {
  font_t font;

  splash_footer();
  font = gdispOpenFont (FONT_LG_SANS);

  gdispDrawStringBox (0, 0, gdispGetWidth(),
		      gdispGetFontMetric(font, fontHeight),
		      "CHECK SD CARD!",
		      font, Red, justifyCenter);

}

void splash_welcome(void)
{
  gdispImage myImage;
  int curimg = 0;

  // cycle through these images once
  const char *splash_images[] = {
    IMG_SPLASH,
    IMG_SPONSOR,
    NULL
  };

  while (splash_images[curimg] != NULL) {
    if (gdispImageOpenFile (&myImage,
                            splash_images[curimg]) == GDISP_IMAGE_ERR_OK) {
      gdispImageDraw (&myImage,
                      0, 0,
                      myImage.width,
                      myImage.height, 0, 0);

      gdispImageClose (&myImage);
    }

    // footer only on 1st image.
    if (curimg == 0) {
      splash_footer();
    }

    chThdSleepMilliseconds (IMG_SPLASH_DISPLAY_TIME);
    curimg++;
  }
  return;
}
