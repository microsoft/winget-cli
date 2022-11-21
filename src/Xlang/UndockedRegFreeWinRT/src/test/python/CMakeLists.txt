set(python_module_name winrt)

set(winmd_files
    Windows.Foundation.FoundationContract
    Windows.Networking.Connectivity.WwanContract
    Windows.Foundation.UniversalApiContract
)

set(included_namespaces
    Windows.Data.Json
    Windows.Devices.Geolocation
    Windows.Foundation
    Windows.Graphics.DirectX


    # uncomment out the following to run complete_winml_tutorial.py
    # Windows.AI.MachineLearning
    # Windows.Graphics.Imaging
    # Windows.Media
    # Windows.Storage
)

set(excluded_namespaces
    Windows.Foundation.Diagnostics.
    Windows.Foundation.Metadata.
    # Windows.Media.Audio.
    # Windows.Media.Capture.
    # Windows.Media.Casting.
    # Windows.Media.ClosedCaptioning.
    # Windows.Media.ContentRestrictions.
    # Windows.Media.Control.
    # Windows.Media.Core.
    # Windows.Media.Devices.
    # Windows.Media.DialProtocol.
    # Windows.Media.Editing.
    # Windows.Media.Effects.
    # Windows.Media.FaceAnalysis.
    # Windows.Media.Import.
    # Windows.Media.MediaProperties.
    # Windows.Media.Ocr.
    # Windows.Media.Playback.
    # Windows.Media.Playlists.
    # Windows.Media.PlayTo.
    # Windows.Media.Protection.
    # Windows.Media.Render.
    # Windows.Media.SpeechRecognition.
    # Windows.Media.SpeechSynthesis.
    # Windows.Media.Streaming.
    # Windows.Media.Transcoding.
    # Windows.Storage.AccessCache.
    # Windows.Storage.BulkAccess.
    # Windows.Storage.Compression.
    # Windows.Storage.FileProperties.
    # Windows.Storage.Pickers.
    # Windows.Storage.Provider.
    # Windows.Storage.Search.
    # Windows.Storage.Streams.
)

# set(included_namespaces
#     Windows.
# )

# set(excluded_namespaces
#     Windows.UI.Composition
#     Windows.UI.Xaml
# )

file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/generated" generated_files_folder)
set(pywinrt_cmdline "-output ${generated_files_folder} -module ${python_module_name}")

file(TO_NATIVE_PATH "$ENV{WindowsSdkDir}References/$ENV{WindowsSDKVersion}" winsdk_references_root)
foreach (winmd ${winmd_files})
    file(TO_NATIVE_PATH "${winsdk_references_root}/${winmd}" contract_root)
    file(GLOB_RECURSE winmd_path "${contract_root}/${winmd}.winmd")
    list(LENGTH winmd_path winmd_path_length)
    if(winmd_path_length EQUAL 1)
        string(APPEND pywinrt_cmdline " -input \"${winmd_path}\"")
    endif()
endforeach()

foreach(ns ${included_namespaces})
    string(APPEND pywinrt_cmdline " -include ${ns}")
endforeach()

foreach(ns ${excluded_namespaces})
    string(APPEND pywinrt_cmdline " -exclude ${ns}")
endforeach()

file(TO_NATIVE_PATH "${generated_files_folder}/setup.py" setup_py)
file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/../../tool/python/pywinrt.exe pywinrt_exe)
add_custom_command(
  OUTPUT ${setup_py} 
  DEPENDS ${pywinrt_exe}
  COMMAND ${pywinrt_exe} ${pywinrt_cmdline}
)
add_custom_target(generate_python_test DEPENDS ${setup_py})

file(TO_NATIVE_PATH "${generated_files_folder}/_${python_module_name}.pyd" module_pyd)
add_custom_command(
  OUTPUT ${module_pyd} 
  DEPENDS ${setup_py}
  WORKING_DIRECTORY ${generated_files_folder}
  COMMAND py setup.py build_ext --inplace
)
add_custom_target(build_python_test DEPENDS ${module_pyd})
