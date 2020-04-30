# install Command
The <b>install</b> command is used to install the specified application.   Use the <b>[search](search.md) </b> command to identify the application you want to install.  

The <b>install</b> command requires that you specify the exact string to install.  If there is any ambiguity, you will be prompted to further filter the  <b>install</b> command to  an exact application.

usage: <code> winget install [[-q] \<query>] [\<options>]</code>

## arguments
The following arguments are available:
| argument      | description|
| -------------: | :-------------|  
| **-q,--query**  |  The query used to search for an app |

## install options
The options allow you to customize the install experience to meet your needs.
| option                       | description|
| --------------: | :------------- 
| **-m, --manifest** |   needs to be followed by the path to the manifest (yaml) file.  You can use the manifest to run the install experience from the local yaml file. |
| **--id**    |     id will limit the install to the id of the application.   |  
| **--name**   |    name will limit the search to the name of the application. |  
| **--moniker**   |  moniker will limit the search to the moniker listed for the application.|  
| **-v, --version**  |   version allows you to specify an exact version to install.  If not specified, latest will install the highest versioned application. |  
| **--tag**   |      tag will limit the search to the tags listed for the application. |  
| **-s, --source**   |   source needs to be followed by the source name and will restrict the search to the source name provided. |  
| **-e, --exact**   |     exact will force to use the exact string in the query.  It will not use the default behavior of a substring.  In addition the string will be case sensitive. |  
| **-i, --interactive**|  interactive will run the installer in interactive mode.  The default experience is show installer progress. |  
| **-h, --silent**    |  silent will run the installer in silent mode.  This will suppress all UI. The default experience is show installer progress. |  
| **-o, --log**      |   log allows you to direct the logging to a log file.  You must provide a path to a file that you have the write rights to.
| **--override** |  Override is a string that will be passed directly to the installer.    | 
| **-l,--location** |      Location to install to (if supported)|

## Multiple Selections 
If the query provided to winget does not result in a single application, then winget will display the results of the search.  This will provide you with the additional data necessary to refine the search for a correct install. 

## Local install
The <b>manifest</b> option will allow you to install an application by passing in a YAML file directly to the client.  The <b>manifest</b> option has the following usage.

usage: <code> winget install --manifest \<file> </code>
| option  | description|
| --------------: | :------------- |
|  **-m,--manifest** | The path to the manifest of the application to install |
|<img width=100   />|<img width=500 />  |

### install log files
The log files for winget unless redirected, will be located in the following folder:  <b> \%temp%\\AICLI\\*.log</b>

 
