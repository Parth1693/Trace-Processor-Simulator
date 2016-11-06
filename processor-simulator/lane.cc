#include "processor.h"


PE_lane::PE_lane() {
   // The lane is initially empty of instructions.
   rr.valid = false;
   ex.valid = false;
   wb.valid = false;
}
