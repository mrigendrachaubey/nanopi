/*
 *
 * Rockchip SSD Demo on Linux Platform
 *
 */

#include <sys/time.h>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <getopt.h>

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#include "run_ssd.h"

using namespace cv;
using namespace std;

#define IMG_CHANNEL         3
#define MODEL_INPUT_SIZE    300
#define NUM_RESULTS         1917

#define Y_SCALE  10.0f
#define X_SCALE  10.0f
#define H_SCALE  5.0f
#define W_SCALE  5.0f

typedef float DTYPE;

Scalar colorArray[10] = {
        Scalar(139,   0,   0, 255),
        Scalar(139,   0, 139, 255),
        Scalar(  0,   0, 139, 255),
        Scalar(  0, 100,   0, 255),
        Scalar(139, 139,   0, 255),
        Scalar(209, 206,   0, 255),
        Scalar(  0, 127, 255, 255),
        Scalar(139,  61,  72, 255),
        Scalar(  0, 255,   0, 255),
        Scalar(255,   0,   0, 255),
};

float MIN_SCORE = 0.4f;

float NMS_THRESHOLD = 0.45f;

int loadLabelName(string locationFilename, string* labels) {
    ifstream fin(locationFilename);
    string line;
    int lineNum = 0;
    while(getline(fin, line))
    {
        labels[lineNum] = line;
        lineNum++;
    }
    return 0;
}


int loadCoderOptions(string locationFilename, float (*boxPriors)[NUM_RESULTS])
{
    const char *d = ", ";
    ifstream fin(locationFilename);
    string line;
    int lineNum = 0;
    while(getline(fin, line))
    {
        char *line_str = const_cast<char *>(line.c_str());
        char *p;
        p = strtok(line_str, d);
        int priorIndex = 0;
        while (p) {
            float number = static_cast<float>(atof(p));
            boxPriors[lineNum][priorIndex++] = number;
            p=strtok(nullptr, d);
        }
        if (priorIndex != NUM_RESULTS) {
            return -1;
        }
        lineNum++;
    }
    return 0;

}

float CalculateOverlap(float xmin0, float ymin0, float xmax0, float ymax0, float xmin1, float ymin1, float xmax1, float ymax1) {
    float w = max(0.f, min(xmax0, xmax1) - max(xmin0, xmin1));
    float h = max(0.f, min(ymax0, ymax1) - max(ymin0, ymin1));
    float i = w * h;
    float u = (xmax0 - xmin0) * (ymax0 - ymin0) + (xmax1 - xmin1) * (ymax1 - ymin1) - i;
    return u <= 0.f ? 0.f : (i / u);
}

float expit(float x) {
    return (float) (1.0 / (1.0 + exp(-x)));
}

void decodeCenterSizeBoxes(float* predictions, float (*boxPriors)[NUM_RESULTS]) {

    for (int i = 0; i < NUM_RESULTS; ++i) {
        float ycenter = predictions[i*4+0] / Y_SCALE * boxPriors[2][i] + boxPriors[0][i];
        float xcenter = predictions[i*4+1] / X_SCALE * boxPriors[3][i] + boxPriors[1][i];
        float h = (float) exp(predictions[i*4 + 2] / H_SCALE) * boxPriors[2][i];
        float w = (float) exp(predictions[i*4 + 3] / W_SCALE) * boxPriors[3][i];

        float ymin = ycenter - h / 2.0f;
        float xmin = xcenter - w / 2.0f;
        float ymax = ycenter + h / 2.0f;
        float xmax = xcenter + w / 2.0f;

        predictions[i*4 + 0] = ymin;
        predictions[i*4 + 1] = xmin;
        predictions[i*4 + 2] = ymax;
        predictions[i*4 + 3] = xmax;
    }
}

int scaleToInputSize(float * outputClasses, int (*output)[NUM_RESULTS], int numClasses)
{
    int validCount = 0;
    // Scale them back to the input size.
    for (int i = 0; i < NUM_RESULTS; ++i) {
        float topClassScore = static_cast<float>(-1000.0);
        int topClassScoreIndex = -1;

        // Skip the first catch-all class.
        for (int j = 1; j < numClasses; ++j) {
            float score = expit(outputClasses[i*numClasses+j]);
            if (score > topClassScore) {
                topClassScoreIndex = j;
                topClassScore = score;
            }
        }

        if (topClassScore >= MIN_SCORE) {
            output[0][validCount] = i;
            output[1][validCount] = topClassScoreIndex;
            ++validCount;
        }
    }

    return validCount;
}

int nms(int validCount, float* outputLocations, int (*output)[NUM_RESULTS])
{
    for (int i=0; i < validCount; ++i) {
        if (output[0][i] == -1) {
            continue;
        }
        int n = output[0][i];
        for (int j=i + 1; j<validCount; ++j) {
            int m = output[0][j];
            if (m == -1) {
                continue;
            }
            float xmin0 = outputLocations[n*4 + 1];
            float ymin0 = outputLocations[n*4 + 0];
            float xmax0 = outputLocations[n*4 + 3];
            float ymax0 = outputLocations[n*4 + 2];

            float xmin1 = outputLocations[m*4 + 1];
            float ymin1 = outputLocations[m*4 + 0];
            float xmax1 = outputLocations[m*4 + 3];
            float ymax1 = outputLocations[m*4 + 2];

            float iou = CalculateOverlap(xmin0, ymin0, xmax0, ymax0, xmin1, ymin1, xmax1, ymax1);

            if (iou >= NMS_THRESHOLD) {
                output[0][j] = -1;
            }
        }
    }
}

struct globalArgs_t {
    int verbosity;               /* -v option */
    char *labelListFile;         /* -l option */
    char *boxPriors;            /* -b option */
    char *inputImage;           /* -i option */
    char *outputImage;          /* -o option */
    char *graphRuntimePath;     /* -g option */
    char *paramPath;            /* -p option */
    int numClasses;             /* -n option */

} globalArgs;

void display_usage( void ) {
    cout << "rkssddemo for linux platform" << endl;
    cout << "usage:" << endl;
    cout << "\t-i:\tinput image file" << endl;
    cout << "\t-o:\toutput image file" << endl;
    cout << "\t-l:\tlabel list file" << endl;
    cout << "\t-b:\tbox priors file" << endl;
    cout << "\t-g:\tgraph runtime file" << endl;
    cout << "\t-p:\tparam file" << endl;
    cout << "\t-n:\ttarget classes number" << endl;
    cout << endl;
    cout << "example: ./rkssddemo -i ssd/test.jpg -o out.jpg -l ssd/coco_labels_list.txt -b ssd/box_priors.txt -g librkssd.so -p ssd/ssd300_91c_param.params -n 91"<< endl;
    cout << endl;
    exit(EXIT_FAILURE);
}

void getopts(int argc, char **argv)
{
    int res;
    globalArgs.inputImage = nullptr;
    globalArgs.outputImage = const_cast<char *>("out.jpg");
    globalArgs.labelListFile = nullptr;
    globalArgs.boxPriors = nullptr;
    globalArgs.graphRuntimePath = nullptr;
    globalArgs.paramPath = nullptr;
    globalArgs.verbosity = 0;
    globalArgs.numClasses = 91;
    while ((res = getopt(argc, argv, "i:o:l:b:g:p:n:vh")) >= 0)
    {
        switch (res)
        {
            case 'i':
                globalArgs.inputImage = optarg;
                break;
            case 'o':
                globalArgs.outputImage = optarg;
                break;
            case 'l':
                globalArgs.labelListFile = optarg;
                break;
            case 'b':
                globalArgs.boxPriors = optarg;
                break;
            case 'g':
                globalArgs.graphRuntimePath = optarg;
                break;
            case 'p':
                globalArgs.paramPath = optarg;
                break;
            case 'v':
                globalArgs.verbosity = 1;
                break;
            case 'h':
                display_usage();
                break;
            case 'n':
                globalArgs.numClasses = atoi(optarg);
                break;
            default:
                display_usage();
                break;
        }
    }
    if (globalArgs.inputImage == nullptr || globalArgs.graphRuntimePath == nullptr
        || globalArgs.labelListFile == nullptr || globalArgs.boxPriors == nullptr
        || globalArgs.paramPath == nullptr) {
        display_usage();
    }
}

int main(int argc, char **argv)
{
    getopts(argc, argv);

    size_t inNum = IMG_CHANNEL * MODEL_INPUT_SIZE * MODEL_INPUT_SIZE;
    size_t predictionsNum = NUM_RESULTS * 4;
    size_t outputClassesNum = NUM_RESULTS * globalArgs.numClasses;
    size_t inSize = inNum;

    float boxPriors[4][NUM_RESULTS];
    string labels[globalArgs.numClasses];

    char *inData = new char[inSize];
    DTYPE *predictions = new DTYPE[predictionsNum];
    DTYPE *outputClasses = new DTYPE[outputClassesNum];
    int output[2][NUM_RESULTS];

    memset(inData, 0x00, inSize);
    memset(predictions, 0x00, predictionsNum * sizeof(float));
    memset(outputClasses, 0x00, outputClassesNum * sizeof(float));

    /* load label and boxPriors */
    loadLabelName(globalArgs.labelListFile, labels);
    loadCoderOptions(globalArgs.boxPriors, boxPriors);

    /* read image and transform to BGR array */
    cerr << "read image " << globalArgs.inputImage << endl;
    Mat rgba = imread(globalArgs.inputImage, CV_LOAD_IMAGE_UNCHANGED);
    Mat rgb;
    Mat rgbIn;
    cvtColor(rgba, rgb, COLOR_RGBA2BGR);
    cv::resize(rgb, rgbIn, Size(MODEL_INPUT_SIZE, MODEL_INPUT_SIZE));

    memcpy(inData, rgbIn.data, inSize);

//    ofstream fout("out.RGB");
//    fout.write(inData, inSize);

    // init CRunSSD
    cout << "create CRunSSD" << endl;
    CRunSSD *rkssd = CRunSSD::create(MODEL_INPUT_SIZE, IMG_CHANNEL, NUM_RESULTS, globalArgs.numClasses,
                                     globalArgs.graphRuntimePath, globalArgs.paramPath);

    /* run ssd */
    cout << "run ssd" << endl;
    rkssd->run_ssd((char *)inData, (float *)predictions, (float *)outputClasses);

    /* transform */
    decodeCenterSizeBoxes(predictions, boxPriors);
    int validCount = scaleToInputSize(outputClasses, output, globalArgs.numClasses);
    cout << "validCount: " << validCount << endl;

    if (validCount > 100) {
        cerr << "validCount too much !!" << endl;
        return -1;
    }

    /* detect nest box */
    nms(validCount, predictions, output);

    /* box valid detect target */
    for (int i = 0; i < validCount; ++i) {
        if (output[0][i] == -1) {
            continue;
        }
        int n = output[0][i];
        int topClassScoreIndex = output[1][i];

        int x1 = static_cast<int>(predictions[n * 4 + 1] * rgba.cols);
        int y1 = static_cast<int>(predictions[n * 4 + 0] * rgba.rows);
        int x2 = static_cast<int>(predictions[n * 4 + 3] * rgba.cols);
        int y2 = static_cast<int>(predictions[n * 4 + 2] * rgba.rows);

        string label = labels[topClassScoreIndex];

        cout << label << "\t@ (" << x1 << ", " << y1 << ") (" << x2 << ", " << y2 << ")" << endl;

        rectangle(rgba, Point(x1, y1), Point(x2, y2), colorArray[topClassScoreIndex%10], 5);
        putText(rgba, label, Point(x1, y1 - 12), 1, 2, Scalar(0, 255, 0, 255));
    }

    /* eval average time */
    struct timeval tv1, tv2;
    gettimeofday(&tv1, NULL);
    for (int i = 0; i < 10; i++) {
        rkssd->run_ssd((char *)inData, (float *)predictions, (float *)outputClasses);
    }
    gettimeofday(&tv2, NULL);
    double timeToken = (tv2.tv_sec-tv1.tv_sec)*1000.0+(tv2.tv_usec-tv1.tv_usec)/1000.0;
    printf("ssd average duration: %.2fms\n", timeToken / 10);

    delete inData;
    delete predictions;
    delete outputClasses;

    /* write to image file */
    imwrite("out.jpg", rgba);

    return 0;
}
