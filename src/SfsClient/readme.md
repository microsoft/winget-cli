## SfsClient

Do not change code under the sfs-client directory; it contains sfs-client source code from release 1.1.0 (https://github.com/microsoft/sfs-client/releases/tag/1.1.0).  
It was initially created using git subtree command:
```
    git subtree add --prefix=src/SfsClient/sfs-client https://github.com/microsoft/sfs-client.git be733af9e5c8e9227f2018ff618800bf08a31180 --squash
```
Then updated to release 1.1.0 using:
```
    git subtree pull -P src/SfsClient/sfs-client https://github.com/microsoft/sfs-client 1.1.0 --squash
```


### Update
To update, run the following command, then update the above commit for reference.  'master' can be replaced with the appropriate commit spec as desired.
```
    git subtree pull -P src/SfsClient/sfs-client https://github.com/microsoft/sfs-client master --squash
```
**When committing the PR, DO NOT squash it.  The two commits are needed as is to allow for future subtree pulls.**
