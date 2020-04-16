---
title: Windows Package Manager
description: This document describes the Windows Package Manager command: install.
ms.date: 5/1/2020
ms.topic: article
keywords:  
ms.localizationpriority: medium
ms.custom:  
---




# install Command
The <b>install</b> command is used to install the specified application.   Use the [search](search.md) command to identify the application you want to install.  

The <b>install</b> command requires that you specify the exact string to install.  If there is any ambiguity, you will be prompted to further filter the  <b>install</b> command to  an exact application.

usage: <code> winget install [[-q] \<query>] [\<options>]</code>

The following arguments are available:
>-q,--query     The query used to search for an app

## install options
The options allow you to customize the install experience to meet your needs.
* **-manifest**   needs to be followed by the path to the manifest (yaml) file.  You can use the manifest to run the install experience from the local yaml file.
* **-id**        id will limit the install to the id of the application.   
* **-name**      name will limit the search to the name of the application.
* **-moniker**    moniker will limit the search to the moniker listed for the application.
* **-version**    version allows you to specify and exact version to install.  If not specified, latest is installed.
* **-tag**        tag will limit the search to the tags listed for the application.
* **-name**       name will limit the search to the name of the application.
* **-channel**    channel specifies that channel for the tool.  This allows you to install flighted apps.
* **-source**     source needs to be followed by the source name, and will restrict the search to the source name provided
* **-exact**       exact will for search to look for the exact string.  It no longer is a substring.  In addition the string will be case sensitive.
* **-interactive** interactive will run the installer in interactive mode.  The default experience is show installer progress.
* **-silent**     silent will run the installer in silent mode.  This will suppress all UI. The default experience is show installer progress.
* **-language**   language allows you to specify the language preference.  The language must be [bcp47](https://www.w3.org/International/articles/language-tags/) compliant.
* **-log**        log allows you to direct the logging to a log file.  You must provide a path to a file that you have the write rights to.
* **-override**   Override is a string that will be passed directly to the installer.   



## Multiple Selections 
If the query provided to winget does not result in a single application.  Then winget will display the results of the search.  This will provide you with the additional data necessary to refine the search for a correct install. 

## Local install
The <b>manifest</b> option will allow you to install an application by passing in a YAML file directly to the client.  The <b>manifest</b> option has the following usage.

usage: <code> winget install --manifest /<file></code>


 * **-m,--manifest** The path to the manifest of the application to install


### install logging files
The install log for the winget unless redirected, will be located in the following folder:
%temp%\AICLI\*.log

 
