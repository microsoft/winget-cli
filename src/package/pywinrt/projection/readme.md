# Python/WinRT

The Windows Runtime Python Projection (Python/WinRT) enables Python developers to access
[Windows Runtime APIs](https://docs.microsoft.com/uwp/api/) directly from Python in a natural
and familiar way.

## Getting Started

### Prerequisites

* [Windows 10](https://www.microsoft.com/windows), October 2018 Update or later.
* [Python for Windows](https://docs.python.org/3.7/using/windows.html), version 3.7 or later
* [pip](https://pypi.org/project/pip/), version 19 or later

### Installing

Python/WinRT can be installed from the [Python Package Index](https://pypi.org/) via pip. Assuming
pip is on the path, Python/WinRT can be installed from the command line with the following command:

``` shell
> pip install winrt
```

You can test that Python/WinRT is installed correctly by launching Python and running the following
snippet of Python code. It should print "https://github.com/Microsoft/xlang/tree/master/src/tool/python"
to the console.

``` python
import winrt.windows.foundation as wf
u = wf.Uri("https://github.com/")
u2 = u.combine_uri("Microsoft/xlang/tree/master/src/tool/python")
print(str(u2))
```

## Using the Windows Runtime from Python

> For a full end-to-end sample of using Python/WinRT, please see the
> [WinML Tutorial](https://github.com/Microsoft/xlang/tree/master/samples/python/winml_tutorial)
> in the samples folder of the xlang GitHub repo.

The WinRT APIs are documented on [docs.microsoft.com](https://docs.microsoft.com/uwp/api/).
At this time, there is no official documentation for using WinRT from Python. However, this section
will describe how WinRT APIs are projected in Python.

### Namespace Modules

WinRT namespaces are projected in Python as modules. WinRT namespaces are projected in Python as
lower case to conform with standard Python naming conventions. WinRT namespaces are also prefixed
with the `winrt` package name. For example, the
[Windows.Devices.Geolocation](https://docs.microsoft.com/uwp/api/Windows.Devices.Geolocation)
WinRT namespace is projected as `winrt.windows.devices.geolocation` in Python.

Importing a WinRT namespace module will automatically import namespace modules containing dependent
types, but will not automatically import child namespace modules. For example `winrt.windows.devices.geolocation`
will automatically import `winrt.windows.foundation` and `winrt.windows.foundation.collections` but
will not automatically import ``winrt.windows.devices.geolocation.geofencing`.

### Class Members

WinRT type members are projected in Python using snake_case, following standard Python naming conventions.
For example, the [Geolocator.GetGeopositionAsync](https://docs.microsoft.com/uwp/api/windows.devices.geolocation.geolocator.getgeopositionasync)
method is projected as `get_geoposition_async` in Python

### Enum Members

WinRT enums are projected as [`IntEnum`](https://docs.python.org/3/library/enum.html#enum.IntEnum)
in Python. Enum members are projected in UPPER_SNAKE_CASE as per standard Python naming conventions.
For example. the 
[PositionStatus](https://docs.microsoft.com/uwp/api/Windows.Devices.Geolocation.PositionStatus)
WinRT enum is projected in Python like this:

``` python
class PositionStatus(enum.IntEnum):
    READY = 0
    INITIALIZING = 1
    NO_DATA = 2
    DISABLED = 3
    NOT_INITIALIZED = 4
    NOT_AVAILABLE = 5
```

### Static typing

While Python is a dynamic language, WinRT is statically typed. Python/WinRT will automatically
convert between the two type systems automatically. Passing the incorrect type to a WinRT method
will result in a type error. 

### Method Overloading

WinRT supports method overloading - i.e. multiple methods on the same type with the same name.
Overloaded methods are typically differentiated in WinRT by the number of parameters.

When calling a projected method, Python/WinRT will compare the number of provided parameters to the
number of parameters each overload accepts. Providing too many or too few parameters will results in
a type error. Providing parameters of the wrong type will also result in a type error.

### Async Coroutines

WinRT type methods that return an IAsync* interface are projected as coroutines in Python. This means
they can be called using the await keyword from inside a Python
[coroutine function](https://docs.python.org/3/reference/compound_stmts.html#async-def). For example:

``` python
import winrt.windows.devices.geolocation as wdg

async def get_current_latitude():
    locator = wdg.Geolocator()
    pos = await locator.get_geoposition_async()
    return pos.coordinate.latitude
```

### Collection Protocols

WinRT and Python both have standard definitions of iterators, sequences and maps. Python/WinRT automatically
projects WinRT collection types as using the appropriate Python protocol.

* WinRT iterators ([IIterable<T>](https://docs.microsoft.com/uwp/api/Windows.Foundation.Collections.IIterable_T_)/
  [IIterator<T>](https://docs.microsoft.com/uwp/api/windows.foundation.collections.iiterator_t_))
  are projected implementing the Python [iterator protocol](https://docs.python.org/3/c-api/iter.html).
* WinRT vectors ([IVector<T>](https://docs.microsoft.com/uwp/api/windows.foundation.collections.ivector_t_)/
  [IVectorView<T>](https://docs.microsoft.com/uwp/api/windows.foundation.collections.ivectorview_t_))
  are projected implementing the Python [sequence protocol](https://docs.python.org/3/c-api/sequence.html).
  IVectorView supports [sq_length](https://docs.python.org/3/c-api/typeobj.html?highlight=sq_length#c.PySequenceMethods.sq_length)
  and [sq_item](https://docs.python.org/3/c-api/typeobj.html?highlight=sq_length#c.PySequenceMethods.sq_item).
  IVector also support [sq_ass_item](https://docs.python.org/3/c-api/typeobj.html?highlight=sq_length#c.PySequenceMethods.sq_ass_item).
* WinRT maps ([IMap<K,V>](https://docs.microsoft.com/uwp/api/windows.foundation.collections.imap_k_v_)/
  [IMapView<K,V>](https://docs.microsoft.com/uwp/api/windows.foundation.collections.imapview_k_v_))
  are projected implementing the Python [mapping protocol](https://docs.python.org/3/c-api/mapping.html).
  IMapView supports [mp_length](https://docs.python.org/3/c-api/typeobj.html?highlight=sq_length#c.PyMappingMethods.mp_length)
  and [mp_subscript](https://docs.python.org/3/c-api/typeobj.html?highlight=sq_length#c.PyMappingMethods.mp_subscript).
  IMap also support [mp_ass_subscript](https://docs.python.org/3/c-api/typeobj.html?highlight=sq_length#c.PyMappingMethods.mp_ass_subscript).

Additionally, Python/WinRT will convert Python collections to WinRT collections as appropriate. For
example, one of the overloads of
[TensorFlow.Create](https://docs.microsoft.com/uwp/api/windows.ai.machinelearning.tensorfloat.create#Windows_AI_MachineLearning_TensorFloat_Create_Windows_Foundation_Collections_IIterable_System_Int64__)
takes a WinRT iterator of 64 bit integers. Python/WinRT will accept any Python [iterator type](https://docs.python.org/3/library/stdtypes.html#typeiter)
as a parameter.

``` python
shape = winml.TensorFloat.create([1, 1000, 1, 1])
```

### Query Interface

WinRT methods typically return objects with full type information that can be projected into Python.
However, there are some WinRT APIs that don't have static type information. In these cases, the
object must be converted into the correct type by the developer. For example,
[LearningModelEvaluationResult.Outputs](https://docs.microsoft.com/uwp/api/windows.ai.machinelearning.learningmodelevaluationresult.outputs)
property is a map of strings to objects without type information. The expected type of the output in
this example depends on the types of the input. In the
[WinML Python/WinRT Tutorial](https://github.com/Microsoft/xlang/tree/master/samples/python/winml_tutorial),
softmaxout_1 is bound to a [TensorFloat](https://docs.microsoft.com/uwp/api/windows.ai.machinelearning.tensorfloat).
So we need to convert the softmaxout_1 output back to a TensorFloat in order to work with it.

In WinRT terms, this conversion is achieved by calling
[QueryInterface](https://docs.microsoft.com/windows/desktop/api/unknwn/nf-unknwn-iunknown-queryinterface%28refiid_void%29)
to get a correctly typed object. WinRT classes and concrete interfaces project a `_from` static
method in Python\WinRT to support calling Query Interface on arbitrary objects in order to convert
them to the correct type so they can be used in Python.

## Known Issues

* This release of the Python/WinRT does not support WinRT composable types. This includes most of
the classes in the Windows.UI.Composition and Windows.UI.Xaml namespaces. These namespaces are excluded
from Python/WinRT.
  * Note, a few methods in other namespaces reference types in these namespaces. For example,
  [MediaPlayerSurface](https://docs.microsoft.com/uwp/api/windows.media.playback.mediaplayersurface)
  is in the Windows.Media.Playback namespace but has properties that return types in the
  Windows.UI.Composition namespace. While these properties are available to Python, accessing them
  is not supported at this time.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE) file for details.

## Contributing

This project welcomes contributions and suggestions. Please visit our [GitHub repo](https://github.com/Microsoft/xlang/)
to file issues, make suggestions or submit pull requests.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
