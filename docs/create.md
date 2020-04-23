# create Command
The <b>create</b> command offers functionality to enable you to create a manifest file for submission to the repository.  The application manifest format is a YAML file and follows the following [specification](https://github.com/microsoft/winget-pkgs/YamlSpec.md).

## Sub Commands
To assist in creating a manifest file, the following sub commands have been provided:

| sub commands  | description|
| --------------: | :------------- |
| **validate** |  the validated sub command takes a manifest file as an argument, and then verifies its compliance with the YAML specification as well as the repository requirements.   For details on the requirements, see [YamlSpec.md](https://github.com/microsoft/winget-pkgs/YamlSpec.md).
| **hash**         |  the hash sub command takes a binary installer as input and will report the hash values to use in the manifest file.
| **-?, --help** |  get additional help on this command
|<img width=100   />|<img width=500 />  |


## validate

Validates a manifest using a strict set of guidelines. This is intended to enable you to check your manifest before submitting to a repo.

usage: <code> winget create validate [--manifest] \<manifest> </code>

The following arguments are available:

| argument  | description|
| --------------: | :------------- |
| **--manifest** |  the path to the manifest to be validated.
| **-?, --help** |  get additional help on this command
|<img width=100   />|<img width=500 />  |


## hash
The <b>hash</b> sub command, can be used to generate the required SHA256 file hash for all installer types.  In addition,  the hash command also supports generate a SHA256 certificate hash for MSIX files.  

usage: <code> winget create hash [-f] \<file> [\<options>] </code>

The <b>hash</b> sub command, can only run on a local file.  To use the <b>hash</b> sub command, download your installer to a known location.  Then pass in the file path as an argument to the <b>hash</b> sub command.

The following arguments are available:
    

The following options are available:


| argument  | description|
| --------------: | :------------- |
| **-f,--file** |  the path to the file to be hashed
| **-m,--msix**  | the argument indicates that the hash command should also create the SHA 256 SignatureSha256 for use with MSIX installers.
| **-?, --help** |  get additional help on this command 
|<img width=100   />|<img width=500 />  |
 