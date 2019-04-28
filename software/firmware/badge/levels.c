typedef struct _game_levels {
  int minxp;
  char abbv[5];
  char desc[20];
} GAME_LEVELS;

const GAME_LEVELS levels[] = {
  {0,    "ENS",   "Ensign" },
  {100,  "LT JG", "Lieutenant Junior" },
  {200,  "LT",    "Lieutenant" },
  {300,  "LCDR",  "Lieutenant Commander" },
  {400,  "CDR",   "Commander" },
  {500,  "CAPT",  "Captain" },
  {600,  "RDML",  "Rear Admiral (LH)" },
  {700,  "RADM",  "Rear Admiral (UH)" },
  {800,  "VADM",  "Vice Admiral" },
  {900,  "ADM",   "Admiral Chief" },
  {1000, "FADM",  "Fleet Admiral" }
};
