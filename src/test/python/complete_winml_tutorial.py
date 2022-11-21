import asyncio
import csv
import os
import time
import winrt
import winrt.windows.ai.machinelearning as winml 

from pathlib import Path
from winrt.windows.graphics.imaging import BitmapDecoder
from winrt.windows.media import VideoFrame
from winrt.windows.storage import StorageFile, FileAccessMode

def timed_op(fun):

    def sync_wrapper(*args, **kwds):
        print("Starting", fun.__name__)
        start = time.perf_counter()

        ret = fun(*args, **kwds)

        end = time.perf_counter()
        print(f"{fun.__name__} took {end - start:.3f} seconds")
        return ret

    async def async_wrapper(*args, **kwds):
        print("Starting", fun.__name__)
        start = time.perf_counter()

        ret = await fun(*args, **kwds)

        end = time.perf_counter()
        print(f"{fun.__name__} took {end - start:.3f} seconds")
        return ret

    return async_wrapper if asyncio.iscoroutinefunction(fun) else sync_wrapper

@timed_op
def load_model(model_path):
    return winml.LearningModel.load_from_file_path(os.fspath(model_path))

@timed_op
async def load_image_file(file_path):
    file = await StorageFile.get_file_from_path_async(os.fspath(file_path))
    stream = await file.open_async(FileAccessMode.READ)
    decoder = await BitmapDecoder.create_async(stream)
    software_bitmap = await decoder.get_software_bitmap_async()
    return VideoFrame.create_with_software_bitmap(software_bitmap)

@timed_op
def bind_model(model, image_frame):
    device = winml.LearningModelDevice(winml.LearningModelDeviceKind.DEFAULT)
    session = winml.LearningModelSession(model, device)
    binding = winml.LearningModelBinding(session)
    image_feature_value = winml.ImageFeatureValue.create_from_video_frame(image_frame)
    binding.bind("data_0", image_feature_value)
    shape = winml.TensorFloat.create([1, 1000, 1, 1])
    binding.bind("softmaxout_1", shape)
    return (session, binding)

@timed_op
def evaluate_model(session, binding):
    results = session.evaluate(binding, "RunId")
    o = results.outputs["softmaxout_1"]
    result_tensor = winml.TensorFloat._from(o)
    return result_tensor.get_as_vector_view()

def load_labels(labels_path):
    with open(labels_path) as labels_file:
        return {int(index): ', '.join(labels)
                for index, *labels
                in csv.reader(labels_file)}

def print_results(results, labels):
    for confidence, label in sorted(zip(results, labels), reverse=True)[:3]:
        print(f"{labels[label]} ({confidence * 100:.1f}%)")

async def async_main():
    cur_path = Path.cwd()

    winml_content_path = next((
        path for path in
        (p / "samples/python/winml_tutorial/winml_content" for p in (cur_path, *cur_path.parents))
        if path.is_dir()
    ), cur_path)

    model_path = winml_content_path / "SqueezeNet.onnx"
    model = load_model(model_path)

    image_file =  winml_content_path / "kitten_224.png"
    image_frame = await load_image_file(image_file)

    session, binding = bind_model(model, image_frame)
    results = evaluate_model(session, binding)

    labels_path = winml_content_path / "Labels.txt"
    labels = load_labels(labels_path)

    print_results(results, labels)

asyncio.run(async_main())
