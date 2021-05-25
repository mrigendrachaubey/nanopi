# Copyright 2018 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================
"""Converts a frozen graph into a TFLite FlatBuffer."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import os as _os
import subprocess as _subprocess
import tempfile as _tempfile

from tensorflow.contrib.lite.python import lite_constants
from tensorflow.contrib.lite.toco import model_flags_pb2 as _model_flags_pb2
from tensorflow.contrib.lite.toco import toco_flags_pb2 as _toco_flags_pb2
from tensorflow.python.framework import dtypes as _dtypes
from tensorflow.python.platform import resource_loader as _resource_loader
from tensorflow.python.util.lazy_loader import LazyLoader


# Lazy load since some of the performance benchmark skylark rules
# break dependencies.
_toco_python = LazyLoader(
    "tensorflow_wrap_toco", globals(),
    "tensorflow.contrib.lite.toco.python."
    "tensorflow_wrap_toco")
del LazyLoader

# Find the toco_from_protos binary using the resource loader if using from
# bazel, otherwise we are in a pip where console_scripts already has
# the toco_from_protos tool.
if lite_constants.EXPERIMENTAL_USE_TOCO_API_DIRECTLY:
  _toco_from_proto_bin = ""
else:
  _toco_from_proto_bin = _resource_loader.get_path_to_datafile(
      "../toco/python/toco_from_protos")

if _toco_from_proto_bin and not _os.path.exists(_toco_from_proto_bin):
  _toco_from_proto_bin = "toco_from_protos"


def toco_convert_protos(model_flags_str, toco_flags_str, input_data_str):
  """Convert `input_data_str` according to model and toco parameters.

  Unless you know what you are doing consider using
  the more friendly @{tf.contrib.lite.toco_convert}}.

  Args:
    model_flags_str: Serialized proto describing model properties, see
      `toco/model_flags.proto`.
    toco_flags_str: Serialized proto describing conversion properties, see
      `toco/toco_flags.proto`.
    input_data_str: Input data in serialized form (e.g. a graphdef is common)
  Returns:
    Converted model in serialized form (e.g. a TFLITE model is common).
  Raises:
    RuntimeError: When conversion fails, an exception is raised with the error
      message embedded.
  """
  # TODO(aselle): When toco does not use fatal errors for failure, we can
  # switch this on.
  if not _toco_from_proto_bin:
    return _toco_python.TocoConvert(
        model_flags_str, toco_flags_str, input_data_str)

  with _tempfile.NamedTemporaryFile() as fp_toco, \
           _tempfile.NamedTemporaryFile() as fp_model, \
           _tempfile.NamedTemporaryFile() as fp_input, \
           _tempfile.NamedTemporaryFile() as fp_output:
    fp_model.write(model_flags_str)
    fp_toco.write(toco_flags_str)
    fp_input.write(input_data_str)
    fp_model.flush()
    fp_toco.flush()
    fp_input.flush()

    cmd = [
        _toco_from_proto_bin, fp_model.name, fp_toco.name, fp_input.name,
        fp_output.name
    ]
    cmdline = " ".join(cmd)
    proc = _subprocess.Popen(
        cmdline,
        shell=True,
        stdout=_subprocess.PIPE,
        stderr=_subprocess.STDOUT,
        close_fds=True)
    stdout, stderr = proc.communicate()
    exitcode = proc.returncode
    if exitcode == 0:
      stuff = fp_output.read()
      return stuff
    else:
      raise RuntimeError("TOCO failed see console for info.\n%s\n%s\n" %
                         (stdout, stderr))


def tensor_name(x):
  return x.name.split(":")[0]


def toco_convert(input_data,
                 input_tensors,
                 output_tensors,
                 inference_type=lite_constants.FLOAT,
                 inference_input_type=None,
                 input_format=lite_constants.TENSORFLOW_GRAPHDEF,
                 output_format=lite_constants.TFLITE,
                 quantized_input_stats=None,
                 default_ranges_stats=None,
                 drop_control_dependency=True,
                 reorder_across_fake_quant=False,
                 allow_custom_ops=False,
                 change_concat_input_ranges=False):
  """Convert a model using TOCO from `input_format` to `output_format`.

  Typically this is to convert from TensorFlow GraphDef to TFLite, in which
  case the default `input_format` and `output_format` are sufficient.

  Args:
    input_data: Input data (i.e. often `sess.graph_def`).
    input_tensors: List of input tensors. Type and shape are computed using
      `foo.get_shape()` and `foo.dtype`.
    output_tensors: List of output tensors (only .name is used from this).
    inference_type: Target data type of arrays in the output file. Currently
      must be `{FLOAT, QUANTIZED_UINT8}`.  (default FLOAT)
    inference_input_type: Target data type of input arrays. Allows for a
      different type for input arrays in the case of quantization. Currently
      must be `{FLOAT, QUANTIZED_UINT8}`. (default `inference_type`)
    input_format: Type of data to read Currently must be
      `{TENSORFLOW_GRAPHDEF}`. (default TENSORFLOW_GRAPHDEF)
    output_format: Output file format. Currently must be `{TFLITE,
      GRAPHVIZ_DOT}`. (default TFLITE)
    quantized_input_stats: Dict of strings representing input tensor names
      mapped to tuple of integers representing the mean and standard deviation
      of the training data (e.g., {"foo" : (0., 1.)}). Only need if
      `inference_type` is `QUANTIZED_UINT8`. (default None)
    default_ranges_stats: Tuple of integers representing (min, max) range values
      for all arrays without a specified range. Intended for experimenting with
      quantization via "dummy quantization". (default None)
    drop_control_dependency: Boolean indicating whether to drop control
      dependencies silently. This is due to TFLite not supporting control
      dependencies. (default True)
    reorder_across_fake_quant: Boolean indicating whether to reorder FakeQuant
      nodes in unexpected locations. Used when the location of the FakeQuant
      nodes is preventing graph transformations necessary to convert the graph.
      Results in a graph that differs from the quantized training graph,
      potentially causing differing arithmetic behavior. (default False)
    change_concat_input_ranges: Boolean to change behavior of min/max ranges for
      inputs and outputs of the concat operator for quantized models. Changes
      the ranges of concat operator overlap when true. (default False)
    allow_custom_ops: Boolean indicating whether to allow custom operations.
      When false any unknown operation is an error. When true, custom ops are
      created for any op that is unknown. The developer will need to provide
      these to the TensorFlow Lite runtime with a custom resolver.
      (default False)

  Returns:
    The converted data. For example if TFLite was the destination, then
    this will be a tflite flatbuffer in a bytes array.

  Raises:
    ValueError: If the input tensor type is unknown
    RuntimeError: If TOCO fails to convert (in which case the runtime error's
      error text will contain the TOCO error log)
  """
  toco = _toco_flags_pb2.TocoFlags()
  toco.input_format = input_format
  toco.output_format = output_format
  toco.inference_type = inference_type
  if inference_input_type:
    toco.inference_input_type = inference_input_type
  toco.drop_control_dependency = drop_control_dependency
  toco.reorder_across_fake_quant = reorder_across_fake_quant
  toco.allow_custom_ops = allow_custom_ops
  if default_ranges_stats:
    toco.default_ranges_min = default_ranges_stats[0]
    toco.default_ranges_max = default_ranges_stats[1]

  model = _model_flags_pb2.ModelFlags()
  model.change_concat_input_ranges = change_concat_input_ranges
  for idx, input_tensor in enumerate(input_tensors):
    if input_tensor.dtype == _dtypes.float32:
      tflite_input_type = lite_constants.FLOAT
    elif input_tensor.dtype == _dtypes.int32:
      tflite_input_type = lite_constants.INT32
    elif input_tensor.dtype == _dtypes.int64:
      tflite_input_type = lite_constants.INT64
    elif input_tensor.dtype == _dtypes.uint8:
      tflite_input_type = lite_constants.QUANTIZED_UINT8
    # TODO(aselle): Insert strings when they are available
    else:
      raise ValueError("Tensors %s not known type %r" % (input_tensor.name,
                                                         input_tensor.dtype))

    input_array = model.input_arrays.add()

    if inference_type == lite_constants.QUANTIZED_UINT8:
      if tflite_input_type == lite_constants.FLOAT:
        tflite_input_type = lite_constants.QUANTIZED_UINT8
      input_array.mean_value, input_array.std_value = quantized_input_stats[idx]

    input_array.name = tensor_name(input_tensor)
    input_array.shape.dims.extend(map(int, input_tensor.get_shape()))

  for output_tensor in output_tensors:
    model.output_arrays.append(tensor_name(output_tensor))

  # TODO(aselle): Consider handling the case of allowing quantized
  # inputs to be converted to float (via the toco.inference_input_type field).
  data = toco_convert_protos(model.SerializeToString(),
                             toco.SerializeToString(),
                             input_data.SerializeToString())
  return data
