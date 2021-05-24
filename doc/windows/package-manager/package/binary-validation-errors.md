# Binary-Validation-Error

The Windows Package Manager goes to great lengths to create an excellent user experience when installing applications. In order to do this, we must ensure that all applications install on PCs without errors regardless of environment.

To that end, a key test we use for the Windows Package Manager is to ensure that all installers will install without warnings on a variety of popular antivirus configurations. While Windows provides Defender as a built-in antivirus program, many enterprise customers and users employ a wide range of antivirus software.

Therefore, each submission to the Windows Package Manager will be run through several antivirus programs.  These programs all have different virus detection algorithms for identifying [Potentially unwanted application (PUA)](https://docs.microsoft.com/windows/security/threat-protection/intelligence/criteria) and malware.  

## Application failures

If an application fails validation, Microsoft will first attempt to verify that the flagged software is not a false positive with the antivirus vendors.  In many cases, after notification and validation, the antivirus vendor will update their algorithm and the application will pass.

In some cases, however, the code anomaly detected is not able to be determined to be a false positive by the antivirus vendors. In this case the application cannot be added to the Windows Package Manager repository, and the Pull Request will be rejected with a **Binary-Validation-Error** label.

## Responding to Binary-Validation-Error

A previously mentioned, the Windows Package Manager repository is not allowed applications that fail with a **Binary-Validation-Error**.  The next step is for the ISV to update their software to remove the code detected as PUA.

### What if I cannot remove that code?

Occasionally, genuine tools used for debugging and low-level activities, will appear as PUA to the antivirus vendors.  This is because the code necessary to do the debugging will have a similar signature to unwanted software.  Even though this is a legitimate use of that coding practice, unfortunately we are unable to allow those applications into the Windows Package Manager repository.