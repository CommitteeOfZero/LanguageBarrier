# LanguageBarrier

This is the core runtime component for Committee of Zero's MAGES engine game patches, hooking lots of game engine functions to enable asset access redirection at filesystem and higher levels, rendering changes and many more features.

As we try to support all games and any possible patches with a single binary, extensive external configuration is required (in particular, the hooks here make little sense the signatures pointing to their targets). See the root patch repositories (or patched game installations) for details.

LanguageBarrier source code is [MIT licensed](LanguageBarrier/LICENSE), but due to inclusion of xy-VSFilter, our binaries fall under GPLv2. If this is a problem for you, you must remove the xy-VSFilter dependency by hand.

## Currently supported games

- STEINS;GATE (Steam – 2016)
- STEINS;GATE 0 (Steam – 2018)
- CHAOS;CHILD (Steam – 2019, GOG – 2022)
- STEINS;GATE ELITE (Steam – 2019)
- STEINS;GATE: Linear Bounded Phenogram (Steam – 2019)
- STEINS;GATE: My Darling's Embrace (Steam – 2019)
- ROBOTICS;NOTES ELITE (Steam – 2020)
- ROBOTICS;NOTES DaSH (Steam – 2020)