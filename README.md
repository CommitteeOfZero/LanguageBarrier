# LanguageBarrier

**You are looking at the OUTDATED master branch.** The S;G Steam patch depends on this branch, and we keep it default in order to avoid confusion for third-party patches using LanguageBarrier. **New development is done on the multi-game branch, which we use since S;G0.**

This is the core runtime component for Committee of Zero's MAGES engine game patches, hooking lots of game engine functions to enable asset access redirection at filesystem and higher levels, rendering changes and many more features.

As we try to support all games and any possible patches with a single binary, extensive external configuration is required (in particular, the hooks here make little sense the signatures pointing to their targets). See the root patch repositories (or patched game installations) for details.

LanguageBarrier source code is [MIT licensed](LanguageBarrier/LICENSE), but due to inclusion of xy-VSFilter, our binaries fall under GPLv2. If this is a problem for you, you must remove the xy-VSFilter dependency by hand.