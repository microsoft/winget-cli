# show Command
The <b>show</b> command is used to display details on the specified application.

The <b>show</b> command will give you the details on the source of the application as well as the metadata associated with the application.

The <b>show</b> command will only show metadata associated with the application.  If the submitted application excludes some metadata, then the data will not be displayed.

usage: <code> winget show [[-q] \<query>] [\<options>]</code>

The following arguments are available:  
| argument  | description|
| --------------: | :------------- |
| **-q,--query** |  the query used to search for an app
| **-?, --help** |  get additional help on this command
|<img width=100   />|<img width=500 />  |


## Options
The following options are available:  
| option  | description|
| --------------: | :------------- |
| **-m,--manifest** | The path to the manifest of the application to install
| **--id**         |  Filter results by id
| **--name**   |      Filter results by name
| **--moniker**   |  Filter results by app moniker
| **-v,--version** |  Use the specified version;default is the latest version
| **-s,--source** |   Find app using the specified source
| **-e,--exact**     | Find app using exact match
| **--versions**    | Show available versions of the app
|<img width=100   />|<img width=500 />  |



## Multiple Selections 
If the query provided to winget does not result in a single application, then winget will display the results of the search.  This will provide you with the additional data necessary to refine the search.

## Results of show
If a single application is detected, the following data will be displayed.

### Meta data
 * **Id**           Id of the application
 * **Name**         Name of the application
 * **Publisher**   Publisher of the application
 * **Version**      Version of the application
 * **Author**           Author of the application
 * **AppMoniker**           AppMoniker of the application
 * **Description**           Description of the application
 * **License**           License of the application
  * **LicenseUrl**           The URL to the license file of the application
  * **Homepage**           Homepage of the application
  * **Tags**     The tags provided to assist in searching     
 * **Command**           The commands supported by the application
* **Channel**       The details on whether the application is preview or release
* **Minimmum OS Version**          
### Installer details
  * **Arch**           The arch of the installer
  * **Language**           The language of the installer
  * **Installer Type**       The type of installer
  * **Download Url**           The Url of the installer
  * **Hash**           The Sha-256 of the installer 
  * **Scope**          Displays whether the installer is per machine or per user
 



