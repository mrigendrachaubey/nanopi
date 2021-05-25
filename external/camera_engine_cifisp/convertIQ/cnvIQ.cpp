#include <iostream>
#include <time.h>
#include <fcntl.h>

#include <cam_ia_api/cam_ia10_engine_api.h>
#include <calib_xml/calibdb.h>

using namespace std;

void printHelp(void) {
  printf("cnvIQ IQ_file db_file\n");
}

int cnvIQ(const char* iq_file) {
  CalibDb calidb;

  if (calidb.CreateCalibDb(iq_file, IQDATA_DUMP_BINDATA))
      return 0;
  else
      return -1;
}

int main(int argc, const char* argv[]) {
  // to print version of libcam_ia.so
  std::shared_ptr<CamIA10EngineItf> iqEngine = getCamIA10EngineItf();

  if (argc != 3) {
    printf("input parameter error\n");
    printHelp();
    return -1;
  }

  remove("/tmp/CamIqData.db");
  if (cnvIQ(argv[1]) == 0) {
    rename("/tmp/CamIqData.db", argv[2]);
    printf("cnvIQ success\n");
  } else {
    printf("cnvIQ failed\n");
  }

  return 0;
}

