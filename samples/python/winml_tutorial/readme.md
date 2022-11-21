# Tutorial: Create a Windows Machine Learning application with Python/WinRT

This tutorial is a port of the [WinML C++/WinRT tutorial](https://docs.microsoft.com/windows/ai/windows-ml/get-started-desktop)
to Python. Please review the original documentation for background on WinML. This document will only
focus on the differences related to using Python/WinRT instead of C++/WinRT.

> Note, xlang in general and Python/WinRT in particular is still early in its development.
> You are encouraged to experiment with using Python/WinRT outside the bounds of the core scenario
> described in this document. Just remember that Python/WinRT is in beta as you experiment.
> Please [open issues](https://github.com/Microsoft/xlang/issues) when (not if!) you find bugs or
> missing functionality.

## Prerequisites

* [Windows 10](https://www.microsoft.com/windows), October 2018 Update or later.
* [Python for Windows](https://docs.python.org/3.7/using/windows.html), version 3.7 or later
* [pip](https://pypi.org/project/pip/), version 19 or later

## Install the Python/WinRT package

Python/WinRT is available from the Python Package Index. You can install it from the command line
using [pip](https://pypi.org/project/pip/):

``` shell
> pip install winrt
```

## Create the Python file

Create a new python file in the sample root folder (i.e. in the same folder that contains this
readme file) and open it with your favorite text editor. The name of the python file doesn't matter,
but this tutorial will assume the file is named winml_tutorial.py

> Note, [complete_winml_tutorial.py](complete_winml_tutorial.py) contains the complete code for this
tutorial, if you'd rather not type the code in yourself.

## Load the WinML Model

Now that the Python/WinRT  module has been installed, it can be imported for use in Python. We will
use the [LearningModel.LoadFromFilePath](https://docs.microsoft.com/uwp/api/windows.ai.machinelearning.learningmodel.loadfromfilepath)
API to load the ONNX model from disk, as per the
[load the model](https://docs.microsoft.com/windows/ai/windows-ml/get-started-desktop#load-the-model)
step from the original tutorial.

First, instead of manually adding timing code to every function like the C++/WinRT version does,
let's add a [Python decorator](https://docs.python.org/3.7/glossary.html#term-decorator) so we only
have to write the timing code once.

``` python
def timed_op(fun):
    import time

    def wrapper(*args, **kwds):
        print("Starting", fun.__name__)
        start = time.perf_counter()

        ret = fun(*args, **kwds)

        end = time.perf_counter()
        print(fun.__name__, "took", end - start, "seconds")
        return ret

    return wrapper
```

With the `timed_op` decorator, LoadModel in Python is a trivial, one-line function. We simply need to
import the Windows.AI.MachineLearning namespace via the winrt module. Note, even though WinRT
namespaces are PascalCased, they are projected in Python in lowercase as per Python naming conventions.

Once the namespace is loaded, we can access the LoadFromFilePath static function directly from the
LearningModel type. Like namespaces, WinRT class methods are projected in Python as per Python naming
conventions - in the case of methods, lower_snake_case. Also, note the use of
[os.fspath](https://docs.python.org/3/library/os.html?highlight=fspath#os.fspath).
This enables load_model to work with Python
[PathLike objects](https://docs.python.org/3/library/os.html?highlight=fspath#os.PathLike).

``` python
import winrt.windows.ai.machinelearning as winml
import os

@timed_op
def load_model(model_path):
    return winml.LearningModel.load_from_file_path(os.fspath(model_path))
```

We then call the `load_model` function from the main part of the python script, passing the path to
the provided ONNX model.

``` python
from pathlib import Path
winml_content_path = Path.cwd() / "winml_content"

model_path = winml_content_path / "SqueezeNet.onnx"
model = load_model(model_path)
```

Running this code from Python now should result in something similar to the following (the time it
takes to load the model will vary).

``` shell
<path to your clone of the xlang repo>\samples\python\winml_tutorial>py winml_tutorial.py

Starting load_model
load_model took 0.7060564 seconds
```

## Load the Image to Evaluate

Next, we'll [load an image](https://docs.microsoft.com/windows/ai/windows-ml/get-started-desktop#load-the-image)
that we are going to evaluate with the loaded model.

This step requires the use of async WinRT methods. Python/WinRT projects async methods as
[Python awaitables](https://docs.python.org/3/library/asyncio-task.html#awaitables). This enables us
to directly `await` WinRT async methods. However, we need to do a few things to make our python script
work with awaitables.

First, we need a version of the timed_op decorator that supports awaitables. Replace the current `timed_op`
function with the following.

``` python
import asyncio

def timed_op(fun):
    import time

    def sync_wrapper(*args, **kwds):
        print("Starting", fun.__name__)
        start = time.perf_counter()

        ret = fun(*args, **kwds)

        end = time.perf_counter()
        print(fun.__name__, "took", end - start, "seconds")
        return ret

    async def async_wrapper(*args, **kwds):
        print("Starting", fun.__name__)
        start = time.perf_counter()

        ret = await fun(*args, **kwds)

        end = time.perf_counter()
        print(fun.__name__, "took", end - start, "seconds")
        return ret

    return async_wrapper if asyncio.iscoroutinefunction(fun) else sync_wrapper
```

Second, we need to create an async main function so we can use await. Move your existing code that
calls load_model into an async function and add a call to
[asyncio.run](https://docs.python.org/3/library/asyncio-task.html#asyncio.run) to run your async
main function to completion from a non-async context.

``` python
import asyncio

async def async_main():
    winml_content_path = Path.cwd() / "winml_content"

    model_path = winml_content_path / "SqueezeNet.onnx"
    model = load_model(model_path)

asyncio.run(async_main())
```

We haven't added any async code yet, but we should still be able to run the python file and get a
result similar to what we saw in the previous step.

With these helpers now in place, we can write an async python function to load an image and convert
it into a [VideoFrame](https://docs.microsoft.com/uwp/api/Windows.Media.VideoFrame)
for use with the LearningModel we created earlier. Note the use of `await` on methods that have
an "Async" suffix. The "Async" suffix is often (but not always) used to indicate the method call
should be awaited.

``` python
from winrt.windows.graphics.imaging import BitmapDecoder
from winrt.windows.media import VideoFrame
from winrt.windows.storage import StorageFile, FileAccessMode

@timed_op
async def load_image_file(file_path):
    file = await StorageFile.get_file_from_path_async(os.fspath(file_path))
    stream = await file.open_async(FileAccessMode.READ)
    decoder = await BitmapDecoder.create_async(stream)
    software_bitmap = await decoder.get_software_bitmap_async()
    return VideoFrame.create_with_software_bitmap(software_bitmap)
```

We can now add a call to `load_image_file` in our `async_main` function. The `load_image_file` and
`async_main` functions are an async function because they are declared with "async def" instead
of "def". You can only use `await` inside of an async function.

``` python
async def async_main():
    winml_content_path = Path.cwd() / "winml_content"

    model_path = winml_content_path / "SqueezeNet.onnx"
    model = load_model(model_path)

    image_file =  winml_content_path / "kitten_224.png"
    image_frame = await load_image_file(image_file)
```

Running this code from Python now should result in something similar to the following (again,
timings will vary).

``` shell
Starting load_model
load_model took 0.7183248 seconds
Starting load_image_file
load_image_file took 0.17086919999999994 seconds
```

## Bind Input and Output

Now that the model and image to be evaluated are loaded, we create a LearningModelSession to
[bind them together](https://docs.microsoft.com/windows/ai/windows-ml/get-started-desktop#bind-the-input-and-output).

``` python
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
```

A few things to note about the `bind_model` function:

* Instead of using global variables like the C++/WinRT version, this Python code is passing all state
  as parameters and return values. In `bind_model`, we need to return two values - the session and the
  binding - so we group them into a tuple.
* [LearningModelDeviceKind](https://docs.microsoft.com/uwp/api/windows.ai.machinelearning.learningmodeldevicekind)
  is an enum type. Like namespaces and class methods, Python/WinRT projects enum type values using
  Python naming conventions. For enum type values, this is UPPER_SNAKE_CASE.
* [TensorFloat.Create](https://docs.microsoft.com/uwp/api/windows.ai.machinelearning.tensorfloat.create)
  is overloaded - there is a version of Create that takes zero zero parameters and a version that takes
  one parameter. Python/WinRT determines which version of an overloaded method to call based on the
  number of parameters provided
* The single parameter version of
  [TensorFloat.Create](https://docs.microsoft.com/uwp/api/windows.ai.machinelearning.tensorfloat.create#Windows_AI_MachineLearning_TensorFloat_Create_Windows_Foundation_Collections_IIterable_System_Int64__)
  takes a WinRT iterable of 64-bit integers. Python/WinRT enables any Python object implementing the
  [Iterator Protocol](https://docs.python.org/3/c-api/iter.html) to work as  a WinRT iteratble.

We can simply add the `bind_model` call to our existing `async_main` function, using destructuring
assignment to assign the grouped return values into separate variables.

``` python
async def async_main():
    winml_content_path = Path.cwd() / "winml_content"

    model_path = winml_content_path / "SqueezeNet.onnx"
    model = load_model(model_path)

    image_file =  winml_content_path / "kitten_224.png"
    image_frame = await load_image_file(image_file)

    session, binding = bind_model(model, image_frame)
    results = evaluate_model(session, binding)
```

Running this code with Python now should result in something similar to the following.

``` shell
Starting load_model
load_model took 0.7168329999999999 seconds
Starting load_image_file
load_image_file took 0.1684 seconds
Starting bind_model
bind_model took 0.020646799999999965 seconds
```

## Evaluate the Model

We are now ready to [evaluate the model](https://docs.microsoft.com/windows/ai/windows-ml/get-started-desktop#evaluate-the-model)
and find out what the image represents.

``` python
@timed_op
def evaluate_model(session, binding):
    results = session.evaluate(binding, "RunId")
    o = results.outputs["softmaxout_1"]
    result_tensor = winml.TensorFloat._from(o)
    return result_tensor.get_as_vector_view()
```

A few things to note about the `evaluate_model` function:

* The [LearningModelEvaluationResult.Outputs](https://docs.microsoft.com/uwp/api/windows.ai.machinelearning.learningmodelevaluationresult.outputs)
  property returns a WinRT map of string to object. Python/WinRT implements Python's
  [mapping protocol](https://docs.python.org/3/c-api/mapping.html) for WinRT maps. This enables
  us to index into the output property using the square bracket syntax.
* While Python types are  dynamically typed (sometimes know as "duck typing"), WinRT types are
  statically typed. Since the Outputs property is a WinRT map of string to object, we do need to
  convert the softmaxout_1 output object to the correct type in order to use it from Python.
  To convert a WinRT base object to a different static type, all WinRT classes and interfaces
  support [QueryInterface](https://docs.microsoft.com/windows/desktop/api/unknwn/nf-unknwn-iunknown-queryinterface%28refiid_void%29).
  In Python, QueryInterface is projected as a _from static method on the type we want to convert to.
  All WinRT classes and non-parameterized interfaces expose a _from method.

Adding model evaluation to `async_main` is a simple one line addition:

``` python

async def async_main():

    import os.path

    model_path = os.path.abspath("./winml_content/SqueezeNet.onnx")
    model = load_model(model_path)

    image_file = os.path.abspath("./winml_content/kitten_224.png")
    image_frame = await load_image_file(image_file)

    session, binding = bind_model(model, image_frame)
    results = evaluate_model(session, binding)
```

Running this code with Python now should result in something similar to the following.

``` shell
Starting load_model
load_model took 0.7029269 seconds
Starting load_image_file
load_image_file took 0.15494339999999995 seconds
Starting bind_model
bind_model took 0.016178600000000043 seconds
Starting evaluate_model
evaluate_model took 0.022036400000000067 seconds
```

## Print the Results

Finally, we need to print the model evaluation results. The possible model results are stored in a
comma-separated file named Labels.txt. We can load this data in Python without any help from WinRT.

> Note, major thanks to Steve Dower (aka [@zooba](https://twitter.com/zooba)) for rewriting
> load_labels and print_results in a pythonic style.

``` python
def print_results(results, labels):
    for confidence, label in sorted(zip(results, labels), reverse=True)[:3]:
        print(f"{labels[label]} ({confidence * 100:.1f}%)")
```

`load_labels` returns a Python dictionary mapping model results to a text string. All that remains
now is to loop thru the results, looking for the top three probabilities and then print them out.

``` python
def print_results(results, labels):
    for confidence, label in sorted(zip(results, labels), reverse=True)[:3]:
        print(f"{labels[label]} ({confidence * 100:.1f}%)")
```

Just like we saw in `evaluate_model`, WinRT collection can be accessed in a pythonic style. In
`print_results`, the labels WinRT vector is accessed via Python's
[sequence](https://docs.python.org/3/c-api/sequence.html) protocol.

All that remains is calling `load_labels` and `print_results` from `async_main`:

``` python
async def async_main():
    winml_content_path = Path.cwd() / "winml_content"

    model_path = winml_content_path / "SqueezeNet.onnx"
    model = load_model(model_path)

    image_file =  winml_content_path / "kitten_224.png"
    image_frame = await load_image_file(image_file)

    session, binding = bind_model(model, image_frame)
    results = evaluate_model(session, binding)

    labels_path = winml_content_path / "Labels.txt"
    labels = load_labels(labels_path)

    print_results(results, labels)
```

Executing this code with Python one final time should reveal the results of the model evaluation,
predicting that the image is a tabby cat.

``` shell
Starting load_model
load_model took 0.7041156000000001 seconds
Starting load_image_file
load_image_file took 0.1883804 seconds
Starting bind_model
bind_model took 0.01686500000000002 seconds
Starting evaluate_model
evaluate_model took 0.021589099999999917 seconds

tabby,  tabby cat with confidence of 0.931460976600647
Egyptian cat with confidence of 0.06530658155679703
Persian cat with confidence of 0.00019303907174617052
```
