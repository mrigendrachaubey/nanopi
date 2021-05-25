Python Env:
python2.7
numpy
tensorflow = 1.8

Step 1:
bazel run -c opt tensorflow/python/tools/optimize_for_inference -- \
--input=/home/vincent/5c.pb  --output=/home/vincent/5c_opt.pb --frozen_graph=True \
--input_names=Preprocessor/sub --output_names=concat,concat_1 \
--alsologtostderr

Step 2:
bazel run tensorflow/contrib/lite/toco:toco -- \
--input_file=/home/vincent/5c_opt.pb --output_file=/home/vincent/5c.tflite \
--input_format=TENSORFLOW_GRAPHDEF --output_format=TFLITE \
--input_shapes=1,300,300,3 --input_arrays=Preprocessor/sub \
--output_arrays=concat,concat_1 --inference_type=FLOAT --logtostderr

Step 3:
./TFLiteMobileNetSSD300Convertor 5c.tflite 5c

