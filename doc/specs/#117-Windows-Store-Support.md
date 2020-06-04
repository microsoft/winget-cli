---
author: <kevin> <larkin> <kevinlams/<kevinla@microsoft.com>
created on: <2020-05-28>
last updated: <2020-05-28>
issue id: <github issue id>
---
# ===== DRAFT ======
# Package Manager Windows Store Support

## Abstract


This spec identifies requirements and implementation for supporting Windows Store apps in the Windows Package Manager.

## Inspiration


The Windows Store is a curated list of applications.  The store apps include tools and the best of class applications and must be available to Package Manager users.

## Solution Design

Windows Package Manager will consume a manifest file like it does with all other formats.  The Windows Package Manager will then use a store API to initiate downloads and install the apps.



**Store Apps**
The Store has multiple app types.  Those that are free for download and free, but with purchase options, as well as paid apps.

***Free Apps***  
Apps that are free, will be downloaded directly when requested.

***Pay apps [NOT SUPPORTED]***
Apps that are for purchase, will not be allowed to be submitted.  If an app from the store requires commerce, to run, we will not allow them in the Windows Package Manager repository.  
These apps will fail to activate and therefore should not pass our approval policy.

***In app purchase***
Apps that require in app purchase, can be allowed, as long as the app can correctly handle activation and treat the in-app purchase out of band from the Windows Package Manager.

## Manifest Requirements 
To support store apps, the manifest file will need to be updated to support the new format.  Most fields remain the same, but additional changes are required to support the store.

### YAML Spec updates
<b>URL</b> the URL used for the store is not the same as the URL users and the developers expect.  Therefore this field is unnecessary.  
<b>REQUIRED: NEW: StoreID</b> the Store ID will be used to identify the app. example: 9nblggh4nns1   
<b>InstallerType</b> A new format, the WindowsStore type will be provided
<b> InstallerType - Default </b> this needs to be defined, but essentially the author can specify if the user does not select anything... which URL to default to.
<b>Homepage</b> The Homepage will be the URL with the bigid https://www.microsoft.com/en-us/p/app-installer/9nblggh4nns1  
<B>NEW: TransactionType</b> TransactionType will be used to indicate what type of 
<b>NEW: Privacy Policy</b>  
<b>NEW: Terms of transaction</b>  
<b>License terms</b>  
<b>Architecture</b> The architecture will support the architectures supported by the store.  neutral will most likely be the default.
<b>-prelease</b> In order to support flighting, the client will need to support only showing apps without the prerelease flag AND enabling the user to override on the command line.

### Flighting
The store provides the ability to flight apps to partners.  But you must be logged in with your MSA to get the update.  By default the Store App support will be for providing the user with the GA version.  

#### MSA Credentials
In order to support flighting, the Windows Package Manager would need to pass credentials to the Windows Store.  This is not in scope for this release, therefore only GA apps are enabled.  

Developers wishing to flight store apps will need to create a different app and BigID in order to make it available for flighting.  Developers will use the -preview flag in the version to restrict access to those that want prerelease builds.

#### GA Only
We are generic and only pull down the latest GA version of the app.  

## UI/UX Design
Each Store app in the repo will be interacted with by the client like any other app by the Windows Package Manager.

```
PS C:\> winget search appinstaller
Name                    Id                                Version
----------------------------------------------------------------------
AppInstallerFileBuilder Microsoft.AppInstallerFileBuilder 1.2020.211.0
```
And Install
### Store App installation experience
Installing the store app will provide the MSIX installation experience.  The Windows Store App will stream install.

![](images\117-Download.png)


## Manifests
Developers will be able to submit their app by providing meta data that includes the store ID or URL to the store app.

### Duplicate app entries
With the addition of the store, we will have an increase in duplicate entries.  If we have 2 manifests with the same application, we should try to eliminate the duplicate entries by default.  Otherwise it will make it difficult to limit the selection to the one file.

#### Duplicate manifests
Because we have manifests that are based on publisher and appname, there should not be a case where we have multiple manifests with different installers.  If we do, there is something wrong with the metadata.  
 
#### Duplicate entries, one manifest
If a manifest file has multiple formats, MSI, EXE, MSIX.  Unless specified by the user, default will be to display and install will be the  MSIX format.


<b>  Example: <appname>  </b>  
```
...  
InstallerType: MSI  
...  
InstallerType: MSIX  
```
When the user specifies install by default the client will choose to install the version of the app that is flagged as default in the manifest.  If no default is specified, then the app will default to the store as the default app.

See [Install-Order-Spec.md](.\Install-Order-Spec.md) for more details.


## Submission Wizard
Much of the metadata to create a manifest file is available from the homepage.  A tool will be created to pull the data and create a manifest file for adding to the Windows Store.


## Capabilities

### Accessibility

N/A - as the store apps should be no different than other apps

### Security

We need to evaluate access to the API and ensure that for purchase apps are not exposed.

### Reliability

N/A - as the store apps should be no different than other apps.

### Compatibility

Changes to support the store will be in addition.  They should not impact compatibility.  However the metadata changes to the manifest spec will mean newer apps may have additional entries to add.

### Performance, Power, and Efficiency

## Potential Issues


## Future considerations

The wizard to ease metadata transfer.

## Resources

[comment]: # Be sure to add links to references, resources, footnotes, etc.

## History
.001     Draft    5/28/2020
