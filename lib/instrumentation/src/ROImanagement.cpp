#ifdef PIN_H

// #ifndef ROI_FUNCTION_START
#define ROI_FUNCTION_START "counting_start"
// #end

// #ifndef ROI_FUNCTION_START
#define ROI_FUNCTION_STOP "counting_stop"
// #endif

extern CommTrace comm;

KNOB<INT32> KnobROI(KNOB_MODE_WRITEONCE, "pintool", "r", "0",
                    "region-of-interest activation");

static VOID BeginRoi(string name) {
  std::cout << "[PIN] beginning of ROI " << name << " detected" << endl;
  CommTrace::enabled = true;
}

static VOID EndRoi(string name) {
  std::cout << "[PIN] end of ROI " << name << " detected" << endl;
  CommTrace::enabled = false;
}

void InstrumentROI(RTN rtn, const string& roi_begin, const string& roi_end) {
  if (KnobROI) {
    string name = RTN_Name(rtn);

    if (!strcmp(name.c_str(), roi_begin.c_str())) {
      cout << "Counting for region of interest only" << endl;
      RTN_Open(rtn);
      RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)BeginRoi, IARG_PTR,
                     RTN_Name(rtn), IARG_END);
      RTN_Close(rtn);
    }

    // if(name.find(roi_end) != std::string::npos){
    if (!strcmp(name.c_str(), roi_end.c_str())) {
      RTN_Open(rtn);
      RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)EndRoi, IARG_PTR,
                     RTN_Name(rtn), IARG_END);
      RTN_Close(rtn);
    }
  } else {
    CommTrace::enabled = true;
  }
}

void InstrumentROI(RTN rtn) {
  InstrumentROI(rtn, string(ROI_FUNCTION_START), string(ROI_FUNCTION_STOP));
}
#endif
