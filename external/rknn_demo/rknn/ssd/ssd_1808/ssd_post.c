#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ssd.h"
#include "ssd_post.h"
#include "rknn_msg.h"
#include "tracker/track_c_link_c++.h"
#define BOX_PRIORS_TXT_PATH "/usr/share/rknn_demo/box_priors.txt"
#define LABEL_NALE_TXT_PATH "/usr/share/rknn_demo/coco_labels_list.txt"

float MIN_SCORE = 0.4f;
float NMS_THRESHOLD = 0.45f;

char *labels[NUM_CLASS];
float box_priors[4][NUM_RESULTS];

#define MAX_OUTPUT_NUM 100
object_T object_input[MAX_OUTPUT_NUM];
object_T object_output[MAX_OUTPUT_NUM];

char * readLine(FILE *fp, char *buffer, int *len)
{
    int ch;
    int i = 0;
    size_t buff_len = 0;

    buffer = malloc(buff_len + 1);
    if (!buffer) return NULL;  // Out of memory

    while ((ch = fgetc(fp)) != '\n' && ch != EOF)
    {
        buff_len++;
        void *tmp = realloc(buffer, buff_len + 1);
        if (tmp == NULL)
        {
            free(buffer);
            return NULL; // Out of memory
        }
        buffer = tmp;

        buffer[i] = (char) ch;
        i++;
    }
    buffer[i] = '\0';

	*len = buff_len;

    // Detect end
    if (ch == EOF && (i == 0 || ferror(fp)))
    {
        free(buffer);
        return NULL;
    }
    return buffer;
}

int readLines(const char *fileName, char *lines[]){
	FILE* file = fopen(fileName, "r");
	char *s;
	int i = 0;
	int n = 0;
	while ((s = readLine(file, s, &n)) != NULL) {
		lines[i++] = s;
	}
	return i;
}

int loadLabelName(const char* locationFilename, char* labels[]) {
	readLines(locationFilename, labels);
    return 0;
}

int loadBoxPriors(const char* locationFilename, float (*boxPriors)[NUM_RESULTS])
{
    const char *d = " ";
	char *lines[4];
	int count = readLines(locationFilename, lines);
	// printf("line count %d\n", count);
 	// for (int i = 0; i < count; i++) {
		// printf("%s\n", lines[i]);
	// }
    for (int i = 0; i < 4; i++)
    {
        char *line_str = lines[i];
        char *p;
        p = strtok(line_str, d);
        int priorIndex = 0;
        while (p) {
            float number = (float)(atof(p));
            boxPriors[i][priorIndex++] = number;
            p=strtok(NULL, d);
        }
        if (priorIndex != NUM_RESULTS) {
			printf("error\n");
            return -1;
        }
    }
    return 0;
}

float CalculateOverlap(float xmin0, float ymin0, float xmax0, float ymax0, float xmin1, float ymin1, float xmax1, float ymax1) {
    float w = fmax(0.f, fmin(xmax0, xmax1) - fmax(xmin0, xmin1));
    float h = fmax(0.f, fmin(ymax0, ymax1) - fmax(ymin0, ymin1));
    float i = w * h;
    float u = (xmax0 - xmin0) * (ymax0 - ymin0) + (xmax1 - xmin1) * (ymax1 - ymin1) - i;
    return u <= 0.f ? 0.f : (i / u);
}

float expit(float x) {
    return (float) (1.0 / (1.0 + expf(-x)));
}

void decodeCenterSizeBoxes(float* predictions, float (*boxPriors)[NUM_RESULTS]) {

    for (int i = 0; i < NUM_RESULTS; ++i) {
        float ycenter = predictions[i*4+0] / Y_SCALE * boxPriors[2][i] + boxPriors[0][i];
        float xcenter = predictions[i*4+1] / X_SCALE * boxPriors[3][i] + boxPriors[1][i];
        float h = (float) expf(predictions[i*4 + 2] / H_SCALE) * boxPriors[2][i];
        float w = (float) expf(predictions[i*4 + 3] / W_SCALE) * boxPriors[3][i];

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

int filterValidResult(float * outputClasses, int (*output)[NUM_RESULTS], int numClasses)
{
    int validCount = 0;
    // Scale them back to the input size.
    for (int i = 0; i < NUM_RESULTS; ++i) {
        float topClassScore = (float)(-1000.0);
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

int postProcessSSD(float * predictions, float *output_classes, int width,
                   int heigh, struct ssd_group *group)
{
	static int init = -1;
	if (init == -1) {
		int ret = 0;
		printf("loadLabelName\n");
		ret = loadLabelName(LABEL_NALE_TXT_PATH, labels);
		if (ret < 0) {
			return -1;
		}
		// for (int i = 0; i < 91; i++) {
			// printf("%s\n", labels[i]);
		// }

		printf("loadBoxPriors\n");
		ret = loadBoxPriors(BOX_PRIORS_TXT_PATH, box_priors);
		if (ret < 0) {
			return -1;
		}
		// for (int i = 0; i < 4; i++) {
			// for (int j = 0; j < 1917; j++) {
				// printf("%f ", box_priors[i][j]);
			// }
			// printf("\n");
		// }
		init = 0;
	}
	int output[2][NUM_RESULTS];
    decodeCenterSizeBoxes(predictions, box_priors);

    int validCount = filterValidResult(output_classes, output, NUM_CLASS);

    if (validCount > 100) {
        printf("validCount too much !!\n");
        return -1;
    }

    /* detect nest box */
    nms(validCount, predictions, output);

    int last_count = 0;
    group->count = 0;
	int track = 1;
	int maxTrackLifetime = 3;
	if(!track) {
        /* box valid detect target */
	    for (int i = 0; i < validCount; ++i) {
	        if (output[0][i] == -1) {
	            continue;
	        }
	        int n = output[0][i];
	        int topClassScoreIndex = output[1][i];

	        int x1 = (int)(predictions[n * 4 + 1] * width);
	        int y1 = (int)(predictions[n * 4 + 0] * heigh);
	        int x2 = (int)(predictions[n * 4 + 3] * width);
	        int y2 = (int)(predictions[n * 4 + 2] * heigh);
	        // There are a bug show toothbrush always
	        if (x1 == 0 && x2 == 0 && y1 == 0 && y2 == 0)
	            continue;
	        char *label = labels[topClassScoreIndex];

	        group->objects[last_count].select.left   = x1;
	        group->objects[last_count].select.top    = y1;
	        group->objects[last_count].select.right  = x2;
	        group->objects[last_count].select.bottom = y2;
	        memcpy(group->objects[last_count].name, label, 10);

			//printf("%s\t@ (%d, %d, %d, %d)\n", label, x1, y1, x2, y2);
	        last_count++;
	    }
	}
	else
	{
		int track_num_input = 0;
		for (int i = 0; i < validCount; ++i) {
			if (output[0][i] == -1) {
		        continue;
		    }
		    int n = output[0][i];
		    int topClassScoreIndex = output[1][i];

		    int x1 = (int)(predictions[n * 4 + 1] * width);
		    int y1 = (int)(predictions[n * 4 + 0] * heigh);
		    int x2 = (int)(predictions[n * 4 + 3] * width);
		    int y2 = (int)(predictions[n * 4 + 2] * heigh);
		    // There are a bug show toothbrush always
		    if (x1 == 0 && x2 == 0 && y1 == 0 && y2 == 0)
		        continue;
			object_input[track_num_input].r.x = x1;
			object_input[track_num_input].r.y = y1;
			object_input[track_num_input].r.width = x2 -x1;
			object_input[track_num_input].r.height = y2 -y1;
			object_input[track_num_input].obj_class= topClassScoreIndex;
			track_num_input++;
		}

		int track_num_output = 0;

		object_track(maxTrackLifetime, track_num_input, object_input, &track_num_output, object_output, width, heigh);
		for (int i = 0; i < track_num_output; ++i) {
	        int topClassScoreIndex = object_output[i].obj_class;

	        int x1 = (int)(object_output[i].r.x);
	        int y1 = (int)(object_output[i].r.y);
	        int x2 = (int)(object_output[i].r.x +object_output[i].r.width);
	        int y2 = (int)(object_output[i].r.y +object_output[i].r.height);
	        // There are a bug show toothbrush always
	        if (x1 == 0 && x2 == 0 && y1 == 0 && y2 == 0)
	            continue;
	        char *label = labels[topClassScoreIndex];

	        group->objects[last_count].select.left   = x1;
	        group->objects[last_count].select.top    = y1;
	        group->objects[last_count].select.right  = x2;
	        group->objects[last_count].select.bottom = y2;
	        memcpy(group->objects[last_count].name, label, 10);
	        last_count++;
			//printf("%s\t@ (%d, %d, %d, %d)\n", group->faces[i].name, x1, y1, x2 -x1, y2 -y1);
	    }
	}

    group->count = last_count;
}

