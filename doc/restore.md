# Remove the Windows Package Manager preview

Sometimes users trying the preview of the Windows Package Manager would like to restore their machine to a version that did not include the preview.  The following steps will restore your machine.

1) Remove your machine from Windows Insider Flights.
The Windows Package Manager is included in the Windows Insider Flights.  If you are installing a Windows Insider build, you will keep getting updated.
2) Remove yourself from the Windows Package Manager flights.  
If you requested to be added to the Windows Package Manager flights, then you will specifically need to ask to be removed.  You can ask to be removed by filling in the following form:

Note: Make sure you use the correct email address.  It must be the address used to sign up for the Insider build.  If your email address is not the same address that you are using to log into the Windows Store, then removing it will have no impact.

Please allow for 1 week, to be sure the form has been processed, though most likely it is processed much sooner.

3) Install an older build of the [Desktop App Installer](https://www.microsoft.com/en-us/p/app-installer/9nblggh4nns1).  We have placed an earlier released version [here](https://www.github.com/micrsoft/winget-cli/releases).

Steps to install previous version:
1) Install [Desktop App Installer](https://www.github.com/micrsoft/winget-cli/releases).
2) When prompted to downgrade, state 'yes'

Once the above steps have all been completed, you will be back on the build of the [Desktop App Installer](https://www.microsoft.com/en-us/p/app-installer/9nblggh4nns1) that does not include the Windows Package Manager.

