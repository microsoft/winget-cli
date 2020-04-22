# Source Command
The <b>source</b> command is used to manage the repositories accessed by the Windows Package Manager.  With the Source command you can <b>add, remove, list </b> and <b>update</b> the repositories.

A source provides the data for you to discover and install applications. Only add a new source if you trust it as a secure location.

usage: <code> winget source \<sub command> \<options> </code>

## Sub Commands 
Source supports a number of sub-commands for manipulating the sources.  The following sub-commands are supported:

| sub commands  | description|
| --------------: | :------------- |
|  **add** |  adds a new source |
|  **list** | enumerates the list of enabled sources |
|  **update** | updates a source |
|  **remove** | removes a source |
|<img width=100   />|<img width=500 />  |


## Options
The  <b>source</b> command supports the following options:
| options  | description|
| --------------: | :------------- |
|  **-n,--name** | the name to identify the source by |
|  **-a,--arg** | the URL or UNC of the source |
|  **-t,--type** | the type of source |
|<img width=100   />|<img width=500 />  |

## add
<b>add</b> is the sub-command that adds a new source.  The add sub-command requires the <b>--name</b> option and <b> name argument</b> be provided, in order to identify the source.

usage: <code> winget source add [-n, --name] \<name> [-a] \<url> [[-t] \<type>]</code>

example:  <code> winget source add --name Contoso  https://www.contoso.com/cache

<b>add</b> also supports the optional parameter of type.  The <b> type</b> communicates to the client what type of repository it is connecting too.  The following types are supported:
| types  | description|
| --------------: | :------------- |
| **Microsoft.PreIndexed.Package** | the type of source \<default> | 
|<img width=100   />|<img width=500 />  |



## list
<b>list</b> is the sub-command that enumerates the currently enabled  sources.  The list command will also provide details on a specific source.

usage: <code> winget list [-n, --name] \<name> 

### list all
The <b>list</b> sub-command by itself will reveal the complete list of supported sources.
 
For example: 
>  C:\winget list   
>  Current sources:  
> <ul>Contoso ->  https://www.contoso.com/cache 

### list source details
In order to get complete details on the source, pass in the name used to identify the source.  
For example: 
> C:\winget source list --name contoso  
> Name   : contoso  
> Type   : Microsoft.PreIndexed.Package  
> Arg    : https://pkgmgr-int.azureedge.net/cache  
> Data   : AppInstallerSQLiteIndex-int_g4ype1skzj3jy  
> Updated: 2020-4-14 17:45:32.000

<b>Name</b> displays the name to identify the source by  
<b>Type </b> displays the type of repo  
<b>Arg </b> displays the URL or path used by the source  
<b>Data </b> displays the optional package name used if appropriate  
<b>Updated</b> displays the last date and time the source was updated  

## update
<b>update</b> is the sub-command that forces and update to an individual source or all.   
usage: <code> winget update [-n, --name] \<name> </code>

### update all
The <b>update</b> sub-command by itself will request and update to each repo.  
For example: 
> C:\winget update 

### update source 
The <b>update</b> command combined with the <b> --name</b> option can direct and update to an individual source.  
For example: 
<code> C:\winget update --name contoso   </code>

## remove
<b>remove</b> is the sub-command that removes a source.  The <b>remove</b>  sub-command requires the <b>--name</b> option and <b> name argument</b> to be provided, in order to identify the source.

usage: <code> winget source add [-n, --name] \<name>  
For example:  
> winget source remove --name Contoso   

## Default Repository
The Windows Package Manager will ship with a default repository.  You can identify the repository by using the list command.  
For example:  
> winget source list  

## Common Errors
