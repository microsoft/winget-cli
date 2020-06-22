---
author: <kevin> <larkin> <kevinlams/<kevinla@microsoft.com>
created on: <2020-05-28>
last updated: <2020-06-04>
issue id: <#117>
---
# Package Manager Windows Store Support

## Abstract
This spec identifies requirements and implementation for supporting Windows Store apps in the Windows Package Manager.

## Glossary
* **StoreId**:  The StoreID is the 12 character number that is used to reference the application in the store.  This is a Windows Store ID, often referred to as BigId when interacting with the store team.  Example: 9nblggh4nns1.
* **BigId**:  Is the same as StoreId above.  We will not be using that terminology, but adding here for completeness. 

## Inspiration
The Windows Store is a curated list of applications.  The store apps include tools and the best of class applications and must be available to Package Manager users.

## Solution Design
Windows Package Manager will consume a manifest file like it does with all other formats.  The Windows Package Manager will then use a store API to initiate downloads and install the apps.

### Additional Options Considered
#### Store as a Source
We evaluated whether we should set the Windows Store as a source, rather than update the manifests.  After evaluation, we felt the metadata provide in the Store Catalog that we would use for Search was lacking data that we would want in order to provide a great user experience.  

## Phased Approach ##
Supporting the Windows store apps adds a bunch of complexity.  Among other things, developers when submitting their applications to the store agreed to certain terms that did not include the Windows Package Manager.  There we will take a phased approach to enabling store support.

### Phase 1 ###
Phase 1 will be kick the tires.  The Windows Package Manager will only install apps that meet the following critieria:
1) The app is rated for Everyone.
2) The app is Free (no in app purchases)
3) The manifest submitted to the the Winget-pkgs repository was provided by the same publisher that submitted to the Microsoft Store.

Failure to meet the above criteria will result in a manifest rejection.
### Phase 2 ###
Phase 2 we will tackle In App Purchase apps (IAP), and apps support apps that are owned and the complexities of the store metadata.

## Store Apps ##
The Store has multiple app types.  Those that are free for download and free, but with purchase options, as well as paid apps.

* **Free Apps*** 
Apps that are free, will be downloaded directly when requested.  
Free apps can be installed anonymously.

* **Purchased apps**
The Windows Package Manager does not have an commerce experience, therefore apps that require payment will not be supported.  However if the app is 'owned', in phase 2, we should allow these apps to be installed.   If an app from the store requires commerce, to run, we will  allow them in the Windows Package Manager repository.  
These apps will fail to activate and therefore should not pass our approval policy.

* **In App Purchase**
Apps that require in app purchase, can be allowed, as long as the app can correctly handle activation and treat the in-app purchase out of band from the Windows Package Manager.  IAP must comply with the parental controls.

## ESRB and Parental controls ##
### Age Limit ###
The store enables parental controls.  The parent can specify an age limit for their child and winget will not install any application that does not meet the age requirements.  
As stated earlier, in phase 1, we will only support those apps that are available to everyone.

### Purchase Restrictions ###
A parent can specify that purchases through the Microsoft store, require adult approval.  If a user is a child and under this restriction, Winget will not allow a purchase.

> [!NOTE] App installation is based on user logged in to Windows. Parental control is based on user logged in to Store Client. They can be 2 different accounts.  We need a way to detect who actually is trying to “purchase” the app, I think this needs to be done in Store backend server. What makes the logic more complex is: Store client allows multiple accounts to be logged in at the same time..


## Manifest Requirements 
To support store apps, the manifest file will need to be updated to support the new format.  Most fields remain the same, but additional changes are required to support the store.

### YAML Spec updates
**URL** the URL used for the store is not the same as the URL users and the developers expect.  Therefore this field is unnecessary.  

**REQUIRED: NEW: StoreID** the Store ID will be used to identify the app.  
Example: 9nblggh4nns1   

**InstallerType:** A new format, the WindowsStore type will be provided  

**InstallerTypeDefault** The InstallerTypeDefault field will allow the manifest author to specify their preferred app format.  For example: if a manifest includes 3 InstallerTypes, MSI, EXE and WindowsStore.  Setting the InstallerTypeDefault to one of the existing types will cause winget to try and install that InstallerType.

> [!NOTE] The users preference can override the manifest settings.  

**Homepage:** The Homepage will be the URL with the StoreId https://www.microsoft.com/en-us/p/app-installer/9nblggh4nns1   

**NEW: TransactionType**  TransactionType will be used to indicate what type of App pricing model is used.  As stated previously, we will only support "Free" for initial install.  It is acceptable for these apps to support, **trials** or **in app purchases**.  

**NEW: TransactionTerms**  The TransactionTerms URL will be the URL provided by IAP apps in the Windows Store.    

**NEW: PrivacyUrl**  
This is not specific to this feature.  This will include a URL to the apps privacy statement.  
For example: https://support.microsoft.com/en-us/help/4468234/windows-10-desktop-apps-and-privacy  

**Architecture** The architecture will support the architectures supported by the store.  **Neutral** will most likely be the default.  

**-prerelease** To support flighting, the client will need to support only showing apps without the prerelease flag AND enabling the user to override on the command line.

### Flighting
The store provides the ability to flight apps to partners.  But you must be logged in with your MSA to get the update.  

It will be a bit misleading if you are supported in a Flight Ring, you will get a newer version of the app when you install.  In short store flighting will be independent of the manifest version.


#### MSA Credentials
In order to support flighting, the Windows Package Manager would need to pass credentials to the Windows Store.  This is not in scope for this release, therefore only GA apps are enabled.  

Developers wishing to flight store apps will need to create a different app and BigId in order to make it available for flighting.  Developers can use the -prerelease flag in the version to have it treated like a new app.  

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

**StoreID** The StoreId can be used as a unique identifier.  But it is not a friendly string for most users.  Therefore we will promote the Id, Name and appmoniker as the primary fields.

 
### Store App installation experience
Installing the store app will provide the MSIX installation experience.  The Windows Store App will stream install.

![](images\117-Download.png)

> [!NOTE] OPEN ISSUE: We need to determine all additional notifications that needs to inform the user prior to install.

## Manifests
Developers will be able to submit their app by providing meta data that includes the store ID or URL to the store app.

#### Duplicate manifests
Because we have manifests that are based on publisher and appname, there should not be a case where we have multiple manifests with different installers.  If we do, there is something wrong with the metadata.  
 
#### Duplicate entries, one manifest
If a manifest file has multiple formats, MSI, EXE, MSIX.  Unless specified by the user, default will be first to follow the manifest author's preference, then to display and install the MSIX format.

**Example: <appname>**  
```
...  
InstallerType: MSI  
...  
InstallerType: MSIX  
```
If the user has specified a default InstallerType, then the client will try to match.  If not, and the manifest specifies a default InstallerType, then the client will try to match.  If none of the above is true then the app will default to the store as the default InstallerType.

See [Install-Order-Spec.md](.\Install-Order-Spec.md) for more details.

## Manifest Submission validation
Since anyone can submit a manifest file, it is important that we detect that the submitter is  the approved owner.  We will not support community submissions of Store apps, since we want the owner to knowingly agree to expose their app through this channel.

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

## Open Issues from #117
**Policy Question**  Your app may promote or distribute software only through the Microsoft Store.
https://docs.microsoft.com/en-us/windows/uwp/publish/store-policies#101-distinct-function--value-accurate-representation  
***Answer*** We are distributing through the store.  We are using the Store API.  

**Paid Applications**
There are also paid applications in the Microsoft Store.  
***Answer*** See above.  We will not support including the installation of apps for purchase.  

**Multi-User**: And how do you handle some apps installing for the current user only and other apps installing for all users. At the moment, this is a total mess on Windows. Application installation is inconsistent and there seems to be this misguided assumption that a host machine only has one account. In this scenario, then signing in with Microsoft account and install from the store seems ok. But when you have multiple accounts on the same system, which all need access to the same apps, this model simply does not work.
> [!NOTE] OPEN ISSUE:  Need to investigate.

**Sign in**  If winget supports installing apps from the Microsoft Store, do I need to sign in to ***my Microsoft account through the winget cli?  
***Answer***  
Currently the Store does require authentication for content access.  We need the user logged on.  We will need to put this request on the backlog.

**Logged in**  In addition to earlier comments, would like to also throw out there that downloading from the Store should not require initial configuration of credentials or require any authentication. I typically click through quite a few dialogs in the Store client to get our supported download-without-login experience. I'd expect winget to just work.  
***Answer***
Currently the Store does require authentication for content access.  We need the user logged on.  We will need to put this request on the backlog.

**Applications with durables and/or optional packages**
> [!NOTE] OPEN ISSUE:  Need to find a way to promote these.

**Dependencies (e.g. framework packages)**
The Store apps have built in dependencies.  
***Answer*** 
If they are coming from the store, the framework package will resolve all dependencies.

**Flights**
Applications with flight audience groups (can be handled via channel? what about auth?)  
***Answer*** As stated above, store flighting will need to be separate. If your MSA allows for a flight, you will get the latests when you update.

**Store Bug with Durables**
Applications that are labeled as having IAP but don't really (Store bug with durables/optional packages)
> [!NOTE] OPEN ISSUE:  Need to create a new issue on Store for this.

**Discovery, especially around localized product display**
Localized details for a tool will require a localized manifest.
> [!NOTE] OPEN ISSUE:  We should create a best practices base on AppInstaller.

**Windows SKUs without Store support**
Can we have two variants (but same version) of our app in winget to alleviate issues that arise due to SKU limitations?
***Answer***
The manifest will support multiple installers.  This should enable this scenario.  



## History
| Version  | Details  |  Date  |
|--------------|-------------|---------|
.001  |   Draft  | 5/28/2020  
.002  |   Cleaned up some HTML.  Thanks Megamorf. | 6/4/2020
.003  |   Responding to feedback. |6/22/2020