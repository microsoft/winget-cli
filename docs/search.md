# search Command
The <b>search</b> command is used to query the sources for available applications that can be installed by the Windows Package Manager.  

The <b>search</b> command can show all applications available, or it can be filtered down to a specific application.  The <b>search</b> command is used typically to identify the string to use to install a specific application.

usage: <code> winget search [[-q] \<query>] [\<options>] </code>

The following arguments are available:  
| argument  | description|
| --------------: | :------------- |
| **-q,--query** |  the query used to search for an app

## Show all
If the search command includes no filters or options, it will display all available applications in the default source.  You can also search for all applications in another source if you pass in just the <b>source</B> option.


## Search strings
Search strings can be filtered with the following options:  

| option  | description|
| --------------: | :------------- |
| **--id**        |   id will limit the search to the id of the application.  id is helpful because the id includes the publisher and the application name. |
| **--name**      |   name will limit the search to the name of the application. |
| **--moniker**  |    moniker will limit the search to the moniker specified. |
|  **--tag**    |  tag will limit the search to the tags listed for the application. |
| **--command**   |   name will limit the search to the name of the application. |

The string will be treated as a substring.  The search by default is also case insensitive.  
For example, "winget search micro" will return the following:
* Microsoft
* microscope
* MyMicro

## Search options
The search commands supports a number of options or filters to help limit the results.

| option  | description|
| --------------: | :------------- |
| **-e, --exact**  |     exact will force to use the exact string in the query.  It will not use the default behavior of a substring.  In addition the string will be case sensitive. |  
| **-n, --count**      |  count will restrict the out put of the display to the count
| **-s, --source**     |  source will restrict the search to the specified source name.  Click here for more information on <b> [source](source.md)</b>.
