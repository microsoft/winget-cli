---
title: Windows Package Manager
description: This document describes the Windows Package Manager command: search.
ms.date: 5/1/2020
ms.topic: article
keywords:  
ms.localizationpriority: medium
ms.custom:  
---


# search Command
The <b>search</b> command is used to query the repositories for available applications that can be installed by the Windows Package Manager.  

The <b>search</b> command can show all applications available, or it can be filtered down to a specific application.  The <b>search</b>  command is used typically to identify the string to use to install a specific application.

usage: <code> winget search [[-q] \<query>] [\<options>] </code>

The following arguments are available:
>-q,--query     The query used to search for an app

## Show all
If the search command includes no filters or options, it will display all available applications in the default repo.  You can also search for all applications in another repo if you pass in just the <b>repo</B> option.


## Search strings
Search strings can be filtered with the following options:  
 * **--id**           Filter results by id
 * **--name**         Filter results by name
 * **--moniker**      Filter results by app moniker
 * **--tag**      Filter results by tag
 * **--command**      Filter results by command


The string will be treated as a substring.  The search by default is also case insensitive.  
For example, "appinst install micro" will return the following:
* Microsoft
* mīcrō
* microsoft
* MyMicro

## Search options
The search commands supports a number of options or filters to help limit the results.
* **-exact**       exact will for search to look for the exact string.  It no longer is a substring.  In addition the string will be case sensitive.
* **-id**         id will limit the search to the id of the application.  id is helpful because the id includes the publisher and the application name.
* **-name**       name will limit the search to the name of the application.
* **-moniker**    moniker will limit the search to the moniker listed for the application.
* **-tag**        tag will limit the search to the tags listed for the application.
* **-name**       name will limit the search to the name of the application.
* **-source**     source needs to be followed by the source name, and will restrict the search to the source name provided 


