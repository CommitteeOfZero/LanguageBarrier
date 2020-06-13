#ifndef __TEXTREPLACE_H__
#define __TEXTREPLACE_H__

namespace lb {
void globalTextReplacementsInit();
const char* processTextReplacements(const char* base, int fileId, int stringId);
}

#endif
